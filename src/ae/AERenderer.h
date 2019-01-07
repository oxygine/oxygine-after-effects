#pragma once
#include "AEConf.h"
#include "movie/movie.hpp"
#include "ox/MaskedRenderer.hpp"
#include "ox/VideoDriver.hpp"
#include <vector>

using namespace oxygine;
using namespace std;



class Mask: public Object
{
public:
    Mask();
    virtual ~Mask();

    aeMovieRenderMesh _mask;
    const ResAnim* rs;

    spNativeTexture texture;
    Vector4 clip;
    Vector3 msk[4];
};

class AERenderer
{
public:
    typedef std::function< ShaderProgram*(int) > getShader;
	typedef std::function< void() > fnValidate;

    AERenderer(IVideoDriver* driver, getShader sh, const Matrix& wvp, const fnValidate &validate);

    void setColor(const Color& c);
    void setAddColor(const Color& c);

    void setTexture(spNativeTexture texture, spNativeTexture alpha, int flags);

    void setWireMode(int n) { _wire = n; }

    void flush();

    void reset();

    void setBlendMode(IVideoDriver::BLEND_TYPE src, IVideoDriver::BLEND_TYPE dest);

    void setMask(const Mask*);
    void validate();


    vertexPCT2* addVertices(int num, int& offset);

    unsigned short* addIndices(int num);

    int _batches;


    getShader getCurrentShader() const { return _gs; }
    void setShader(const getShader& s) { _gs = s; }
    void applyCustom(const getShader& gs);

protected:
    bool _applyColor = false;
    Color _color;
    Color _addColor;
    int _wire = -1;

    int _textureFlags = 0;

    Matrix _wvp;
    getShader _gs;
	fnValidate _validate;

    static vector<unsigned short> _indices;
	static vector<vertexPCT2> _vertices;
    IVideoDriver* _driver;

    IVideoDriver::BLEND_TYPE _blendSrc;
    IVideoDriver::BLEND_TYPE _blendDest;
    const VertexDeclaration* _decl;

    const Mask* _mask = 0;

    spNativeTexture _texture;
    spNativeTexture _alpha;
};

void buildVertices(vertexPCT2* pv, const aeMovieRenderMesh& mesh, const Vector2& mulUV, const Vector2& addUV, const Color& color);
void buildIndices(unsigned short* pi, const aeMovieRenderMesh& mesh, int indicesOffset);
void buildVD(AERenderer& renderer, const aeMovieRenderMesh& mesh, const RectF& src, const Color& color);