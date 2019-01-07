#include "AERenderer.h"
//#include "dev/tests.h"
#include "ox/ResAnim.hpp"
#include "ox/UberShaderProgram.hpp"
#include "ox/key.hpp"
#include "oxygine/core/gl/oxgl.h"

Mask::Mask()
{
    setName("mask");
}

Mask::~Mask()
{

}

vector<unsigned short> AERenderer::_indices;
vector<vertexPCT2> AERenderer::_vertices;

AERenderer::AERenderer(IVideoDriver* driver, getShader gs, const Matrix& wvp, const fnValidate &validate) : _driver(driver), _batches(0), _gs(gs), _wvp(wvp), _addColor(0), _validate(validate)
{
    _decl = driver->getVertexDeclaration(VERTEX_PCT2);
	_indices.clear();
	_vertices.clear();
}

void AERenderer::setColor(const Color& c)
{
    _color = c;
    _applyColor = _color.rgba() != 0xffffffff;
}

void AERenderer::setAddColor(const Color& c)
{
    _addColor = c;
}

void AERenderer::setTexture(spNativeTexture texture, spNativeTexture alpha, int flags)
{
    if (texture != _texture)
    {
        flush();
        
        _texture = texture;
        _alpha = alpha;
        _driver->setTexture(UberShaderProgram::SAMPLER_BASE, _texture);
        _driver->setTexture(UberShaderProgram::SAMPLER_ALPHA, _alpha);        
    }
    
    if (_textureFlags != flags)
    {
        _textureFlags = flags;
        validate();
    }
}

void AERenderer::flush()
{
    if (_indices.empty())
        return;
    
    {
        _driver->draw(IVideoDriver::PT_TRIANGLES, _decl,
                      &_vertices.front(), static_cast<unsigned int>(_vertices.size() * sizeof(vertexPCT2)),
                      &_indices.front(), static_cast<unsigned int>(_indices.size()));

#ifdef __WIN32__

        if (_batches == _wire)
        {
            _driver->setTexture(UberShaderProgram::SAMPLER_BASE, STDRenderer::white);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

            ShaderProgram* sp = _driver->getShaderProgram();
            _driver->setShaderProgram(_gs(0));

            for (vertexPCT2& v : _vertices)
                v.color = Color(Color::Green).rgba();


            _driver->draw(IVideoDriver::PT_TRIANGLES, _decl,
                          &_vertices.front(), static_cast<unsigned int>(_vertices.size() * sizeof(vertexPCT2)),
                          &_indices.front(), static_cast<unsigned int>(_indices.size()));

            _driver->setTexture(UberShaderProgram::SAMPLER_BASE, _texture);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            _driver->setShaderProgram(sp);
            _driver->setUniform("mat", _wvp);
        }
#endif
    }

    _vertices.clear();
    _indices.clear();
    _batches++;
}


void AERenderer::reset()
{
    _texture = 0;
    _alpha = 0;
    _textureFlags = 0;
    _blendSrc = IVideoDriver::BT_ZERO;
    _blendDest = IVideoDriver::BT_ZERO;

    _driver->setState(IVideoDriver::STATE_BLEND, 1);

    _mask = 0;

    validate();
}

void AERenderer::setBlendMode(IVideoDriver::BLEND_TYPE src, IVideoDriver::BLEND_TYPE dest)
{
    if (src != _blendSrc || dest != _blendDest)
    {
        flush();
        _driver->setBlendFunc(src, dest);
    }

    _blendSrc = src;
    _blendDest = dest;
}

void AERenderer::applyCustom(const getShader& gs)
{
    ShaderProgram* shader = gs(0);
    _driver->setShaderProgram(shader);

#if OXYGINE_RENDERER >= 5
    _driver->setUniform("mat", _wvp);
#else
    _driver->setUniform("mat", &_wvp);
#endif
}

void AERenderer::setMask(const Mask* mask)
{
    if (mask != _mask)
    {
        _mask = mask;
        validate();
    }
}

void AERenderer::validate()
{
    flush();

	int F =  _addColor.rgba() ? UberShaderProgram::ADD_COLOR : 0;
    F |= _textureFlags;    

    if (_mask)
    {
        if (_textureFlags & UberShaderProgram::SEPARATE_ALPHA)
            F |= UberShaderProgram::MASK_R_CHANNEL;

        ShaderProgram* maskShader = _gs(UberShaderProgram::MASK | F);
        _driver->setShaderProgram(maskShader);

        _driver->setTexture(UberShaderProgram::SAMPLER_MASK, _mask->texture);

        _driver->setUniform("mat", _wvp);


        _driver->setUniform("clip_mask", &_mask->clip, 1);
        _driver->setUniform("msk", _mask->msk, 4);

    }
    else
    {
        _driver->setShaderProgram(_gs(F));
        _driver->setUniform("mat", _wvp);
    }

    if (F)
    {
        Vector4 c = _addColor.toVector();
        _driver->setUniform("add_color", &c, 1);
    }

	if (_validate)
		_validate();
}

vertexPCT2* AERenderer::addVertices(int num, int& offset)
{
    size_t s = _vertices.size();

    offset = (int)s;
    _vertices.resize(s + num);
    return &_vertices[s];
}

unsigned short* AERenderer::addIndices(int num)
{
    size_t s = _indices.size();
    _indices.resize(s + num);
    return &_indices[s];
}


void buildVertices(vertexPCT2* pv, const aeMovieRenderMesh& mesh, const Vector2& mulUV, const Vector2& addUV, const Color& color)
{
    for (uint32_t index = 0; index != mesh.vertexCount; ++index)
    {
        vertexPCT2& v = pv[index];

        const ae_vector3_t& p = *(mesh.position + index);
        v.x = p[0];
        v.y = p[1];
        v.z = 0;
        //v.z = p[2];

        const ae_vector2_t& uv = *(mesh.uv + index);
        v.u = uv[0] * mulUV.x + addUV.x;
        v.v = uv[1] * mulUV.y + addUV.y;


        Color vc(
            (unsigned char)(mesh.color.r * 255),
            (unsigned char)(mesh.color.g * 255),
            (unsigned char)(mesh.color.b * 255),
            (unsigned char)(mesh.opacity * 255));
        vc = vc * color;

        v.color = vc.premultiplied().rgba();
    }
}

void buildIndices(unsigned short* pi, const aeMovieRenderMesh& mesh, int indicesOffset)
{
    for (unsigned int i = 0; i < mesh.indexCount; ++i)
    {
        *pi = mesh.indices[i] + indicesOffset;
        ++pi;
    }
}


void buildVD(AERenderer& renderer, const aeMovieRenderMesh& mesh, const RectF& src, const Color& color)
{
    if (!mesh.vertexCount)
        return;

    int indicesOffset;
    vertexPCT2* pv = renderer.addVertices(mesh.vertexCount, indicesOffset);
    buildVertices(pv, mesh, src.size, src.pos, color);

    unsigned short* pi = renderer.addIndices(mesh.indexCount);
    buildIndices(pi, mesh, indicesOffset);
}
