#include "AEMovieWork.h"
#include <functional>
#include "ox/Material.hpp"
#include "movie/movie.hpp"
//#include "ox/STDMaterial.hpp"
#include <map>
#include "ox/stringUtils.hpp"
//#include "MovieSprite.h"
#include "ox/UberShaderProgram.hpp"
//#include "shared.h"
#include "ae/AERenderer.h"
//#include "packs/preloader.h"
#include "ox/stringUtils.hpp"
#include "ox/Font.hpp"
#include <atomic>
//#include "utils/DebugTimer.h"
#include "oxygine/winnie_alloc/winnie_alloc.h"
#include "ox/RenderState.hpp"
#include <stdarg.h>
#include "ox/DebugActor.hpp"

#ifndef AEVIEWER
#include "ResSound.h"
#include "SoundInstance.h"
#endif

#ifdef HAVE_MP
#include "mp/mp_init.h"
#include "mp/mp.h"
#endif

//#define WORK_AREA_API 1
using namespace oxygine;

spAEMovieWork AEMovieWork::createWork(AEMovieResource& res, const string& comp, error_policy ep)
{
    const aeMovieCompositionData* compositionData = ae_get_movie_composition_data(res.movieData, comp.c_str());

    if (!compositionData)
    {
        handleErrorPolicy(ep, "can't find composition %s:%s", res._folder.c_str(), comp.c_str());
        return 0;
    }

    spAEMovieWork movie = new AEMovieWork;
    movie->init2(res, compositionData);
    movie->setName(res._folder + "/" + comp);
    return movie;
}

bool AEMovieWork::computeAllTimelineBounds(RectF & out)
{
    if (!_composition) return false;

    Vector2 umin(0.0f, 0.0f);
    Vector2 umax(0.0f, 0.0f);

    bool uset = false;

    bool hidden = false;

    aeMovieRenderMesh mesh;
    uint32_t mesh_iterator = 0;

    while (ae_compute_movie_mesh(_composition, &mesh_iterator, &mesh) == AE_TRUE)
    {
//        DTManual ae_ren_i("ae-ren-i");

        if (hidden)
        {
            if (mesh.layer_type == AE_MOVIE_LAYER_TYPE_SLOT)
            {
                Actor* actor = (Actor*)mesh.element_userdata;
                if (actor->isName(_showHideSlots))
                    hidden = false;
            }
            continue;
        }

        for (int i = 0; i < (int)mesh.vertexCount; i++) {
            const ae_vector3_t& p = *(mesh.position + i);

            if (!uset)
            {
                uset = true;
                umin.x = umax.x = p[0];
                umin.y = umax.y = p[1];
            }
            else
            {
                umin.x = std::min(umin.x, p[0]);
                umin.y = std::min(umin.y, p[1]);
                umax.x = std::max(umax.x, p[0]);
                umax.y = std::max(umax.y, p[1]);
            }
        }
    }

    out = RectF(umin.x, umin.y, umax.x - umin.x, umax.y - umin.y);
    return uset;
}

bool AEMovieWork::calcMinMaxForAllFrames(RectF & out)
{
    if (!_composition) return false;

    float d = ae_get_movie_composition_duration(_composition);
    float step = std::min(10.0f, d * 0.1f);

    bool ret = false;

    float oldt = ae_get_movie_composition_time(_composition);

    float t = 0;
    ae_set_movie_composition_time(_composition, t);
    while (true)
    {
        RectF r;
        if (computeAllTimelineBounds(r)) {
            if (!ret)
                out = r;
            else
                out.unite(r);
            ret = true;
        }
        if (t >= d) break;

        t += step;
        ae_update_movie_composition(_composition, step);
    }
    
    ae_set_movie_composition_time(_composition, oldt);

    return ret;
}

/*
void AEMovieWork::doRender(const RenderState& rs)
{
//    AEMovie::doRender(rs);

    if (!_composition)
        return;

    //if (_render2cache)
    //{
    //    int q = 0;
    //}

    DTAuto ae_ren("ae-ren-o");

    if (_timeLineFrom)
    {
        float maintm = ae_get_movie_composition_time(_timeLineFrom->ae());
        float loctm = ae_get_movie_composition_time(_composition);
        if (maintm > loctm)
            ae_update_movie_composition(_composition, maintm - loctm);
        else
            ae_set_movie_composition_time(_composition, maintm);
    }

    Material::setCurrent(0);

    STDRenderer* stdr = ((STDMaterial*)rs.material)->getRenderer();
    IVideoDriver* driver = stdr->getDriver();
    stdr->resetSettings();

    const VertexDeclaration* decl = driver->getVertexDeclaration(VERTEX_PCT2);


    Matrix vp = stdr->getViewProjection();
    Matrix wvp = rs.transform.toMatrix() * vp;


    //_res->g
    int shaderFlags = _res->_shaderFlags;
    AERenderer renderer(driver, [=](int f)
    {
        return STDRenderer::uberShader.getShaderProgram(shaderFlags | f);
    }, wvp);


    renderer.setColor(_color);
    renderer.reset();

    bool hidden = false;


    aeMovieRenderMesh mesh;
    uint32_t mesh_iterator = 0;


    if (test::AeHalfFillrate)
    {
        Rect r;

        if (test::AeHalfFillrate == 1)
            r = Rect(0, 0, (int)(getWidth() / 2), (int)getHeight());
        if (test::AeHalfFillrate == 2)
            r = Rect(0, 0, (int)(getWidth() / 2), (int)(getHeight() / 2));
        if (test::AeHalfFillrate == 3)
            r = Rect(0, 0, 0, 0);

        driver->setScissorRect(&r);
    }

    while (ae_compute_movie_mesh(_composition, &mesh_iterator, &mesh) == AE_TRUE)
    {
        DTManual ae_ren_i("ae-ren-i");

        if (hidden)
        {
            if (mesh.layer_type == AE_MOVIE_LAYER_TYPE_SLOT)
            {
                Actor* actor = (Actor*)mesh.element_data;
                if (actor->isName(_showHideSlots))
                    hidden = false;
            }
            continue;
        }

        const Mask* msk = (const Mask*)mesh.track_matte_data;
        if (test::AeMasksDisabled)
            msk = 0;
        if (msk)
        {
            //mesh = msk->_mask;
            //  mesh.layer_type = AE_MOVIE_LAYER_TYPE_IMAGE;

            if (0)
            {
                renderer.setMask(0);
                renderer.drawBatch();
                aeMovieRenderMesh meshMask = msk->_mask;
                ResAnim* rs = (ResAnim*)meshMask.resource_data;
#ifdef ONLINE
                preloader::preload(rs);
#endif
                meshMask.r = 1.0f;
                meshMask.g = 1.0f;
                meshMask.b = 1.0f;
                meshMask.a = 1.0f;

                const AnimationFrame& frame = rs->getFrame(0);
                const RectF& src = frame.getSrcRect();
                if(_show_texture)
                    renderer.setTexture(frame.getDiffuse().base, 0);
                else
                    renderer.setTexture(STDRenderer::white, 0);
                driver->setState(IVideoDriver::STATE_BLEND, 0);
                //buildVD(renderer, mesh, src);
                renderer.drawBatch();
                driver->setState(IVideoDriver::STATE_BLEND, 1);
            }
        }

        if (!_show_texture)
            renderer.setBlendMode(IVideoDriver::BT_ONE, IVideoDriver::BT_ONE);
        else
        switch (mesh.blend_mode)
        {
        case AE_MOVIE_BLEND_SCREEN:
            renderer.setBlendMode(IVideoDriver::BT_ONE, IVideoDriver::BT_ONE_MINUS_SRC_COLOR);
            break;
        case AE_MOVIE_BLEND_ADD:
            renderer.setBlendMode(IVideoDriver::BT_ONE, IVideoDriver::BT_ONE);
            break;
        case AE_MOVIE_BLEND_NORMAL:
            renderer.setBlendMode(IVideoDriver::BT_ONE, IVideoDriver::BT_ONE_MINUS_SRC_ALPHA);
            break;
        case AE_MOVIE_BLEND_MULTIPLY:
            renderer.setBlendMode(IVideoDriver::BT_DST_COLOR, IVideoDriver::BT_ONE_MINUS_SRC_ALPHA);
            break;
        };

        if(_show_texture)
            renderer.setMask(msk);

        switch (mesh.layer_type)
        {
        case AE_MOVIE_LAYER_TYPE_SLOT:
        {
            Actor* actor = (Actor*)mesh.element_data;
            if (actor)
            {
                renderer.drawBatch();

                Material::setCurrent(0);
                actor->render(rs);
                Material::setCurrent(0);

                renderer.reset();

                if (actor->isName(_showHideSlots))
                    hidden = true;
            }
        }
        break;

        case AE_MOVIE_LAYER_TYPE_PARTICLE:
        {
            renderer.drawBatch();

            Actor* actor = (Actor*)mesh.element_data;
            actor->render(rs);
            Material::setCurrent(0);

            renderer.reset();
        } break;
        case AE_MOVIE_LAYER_TYPE_SOLID:
        {
            renderer.setTexture(STDRenderer::white, STDRenderer::white);
            RectF src(0, 0, 1, 1);
            buildVD(renderer, mesh, src, _color);
        }
        break;

        case AE_MOVIE_LAYER_TYPE_IMAGE:
        case AE_MOVIE_LAYER_TYPE_SEQUENCE:
        {
            ResAnim* rs = (ResAnim*)mesh.resource_data;
#ifdef ONLINE
            preloader::preload(rs);
#endif

            const AnimationFrame& frame = rs->getFrame(0);
            const RectF& src = frame.getSrcRect();
            if (_show_texture)
            {
                renderer.setTexture(frame.getDiffuse().base, frame.getDiffuse().alpha);
                buildVD(renderer, mesh, src, _color);
            }
            else
            {
                renderer.setTexture(STDRenderer::white, STDRenderer::white);
                Color clr = _color;
                if (clr.a >= 64)
                    clr.a /= msk ? 4 : 8;
                buildVD(renderer, mesh, src, clr);
            }

        } break;

        case AE_MOVIE_LAYER_TYPE_VIDEO:
        {
            MovieSprite* movie = (MovieSprite*)mesh.element_data;
            if (movie)
            {
                renderer.drawBatch();

                const AnimationFrame& frame = movie->getAnimFrame();
                const RectF& src = frame.getSrcRect();

                movie->convert();

                if (msk)
                    renderer.applyMask(*movie->_shader, msk);
                else
                    renderer.apply(*movie->_shader);


                const Diffuse& d = frame.getDiffuse();

                movie->_shader->apply(driver, d.base, d.alpha);
                Vector2 yaScale(1, 1);
                driver->setUniform("yaScale", &yaScale, 1);

                buildVD(renderer, mesh, src, _color);

                renderer.drawBatch();
                renderer.reset();
            }

        } break;
        }
    }

    renderer.drawBatch();

    if (test::AeHalfFillrate)
    {
        driver->setScissorRect(0);
    }


    DebugActor::addDebugString("AE batches: %d", renderer._batches);

    Material::setCurrent(0);
}
*/