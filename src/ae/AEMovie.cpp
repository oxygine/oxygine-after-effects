#include "AEMovie.h"
#include "AERenderer.h"
//#include "MovieSprite.h"
#include "ox/stringUtils.hpp"
//#include "utils/DebugTimer.h"
#include "ox/RenderState.hpp"
#include <sstream>
#include "ox/UberShaderProgram.hpp"
#include "ox/DebugActor.hpp"
#include "ox/TextField.hpp"
#include "ox/ResAnim.hpp"
//#include "online/preloader.h"

#ifdef HAVE_MP
#include "mp/mp_init.h"
#include "mp/mp.h"
#endif


//#define AE_TEST_PERF

void ae_update_movie_subcompositions(const void*, float)
{
    //logs::warning("NOT IMPLEMENTED!!!");
}

void ae_update_movie_composition_main(const aeMovieComposition * _composition, ae_time_t _timing)
{
    ae_update_movie_composition(_composition, _timing);
}


Matrix fromMatrix34(const float *ptr)
{
    Matrix m;
    m.m11 = ptr[0];
    m.m12 = ptr[1];
    m.m13 = ptr[2];
    m.m14 = 0.0f;

    m.m21 = ptr[3];
    m.m22 = ptr[4];
    m.m23 = ptr[5];
    m.m24 = 0.0f;


    m.m31 = ptr[6];
    m.m32 = ptr[7];
    m.m33 = ptr[8];
    m.m34 = 0.0f;

    m.m41 = ptr[9];
    m.m42 = ptr[10];
    m.m43 = ptr[11];
    m.m44 = 1.0f;

    return m;
}

AEMovie::AEMovie(const AEMovieOptions& opt): _addColor(0), _opt(opt)
{
	_uberShader = &STDRenderer::uberShader;
}

AEMovie::~AEMovie()
{
    AE_SHOULD_NOT_BE_DELETED();

    if (_composition)
        ae_delete_movie_composition(_composition);

    for (auto m : _masks)
        delete m;
}


spAEMovie AEMovie::create(const AEMovieResource& res_, const std::string& comp, error_policy ep, bool interpolation)
{
    return create(res_, comp, AEMovieOptions().withInterpolation(interpolation), ep);
}



spAEMovie AEMovie::create(const AEMovieResource& res_, const std::string& comp, const AEMovieOptions& opt, error_policy ep /*= ep_show_error*/)
{
    auto p = res_.getComposition(comp);
    const aeMovieCompositionData* compositionData = std::get<0>(p);
    const AEMovieResource* res = std::get<1>(p);

    if (!compositionData)
    {
        handleErrorPolicy(ep, "can't find composition %s:%s", res->getFolder().c_str(), comp.c_str());
        return 0;
    }

    spAEMovie movie = new AEMovie(opt);
    movie->init2(*res, compositionData);
    movie->setName(res->getFolder() + "/" + comp);
    return movie;
}

ae_bool_t ae_movie_composition_node_camera(const aeMovieCameraProviderCallbackData* _callbackData, ae_voidptrptr_t _cd, ae_voidptr_t _ud)
{
    return true;
}


void AEMovie::interrupt(bool skip2end)
{
    if (!_playing)
        return;

    float v1, v2;
    ae_get_movie_composition_data_loop_segment(_compositionData, &v1, &v2);

    if (getTime() < v2)
    {
        ae_interrupt_movie_composition(_composition, false);

        if (skip2end)
            ae_set_movie_composition_time(_composition, v2 + 0.01f);
    }


    InterruptEvent ev;
    ev.skip2end = skip2end;
    dispatchEvent(&ev);
}



void AEMovie::setColor(const Color& c)
{
    _color = c;
}

void AEMovie::setAddColor(const Color& c)
{
    _addColor = c;
}

void AEMovie::detachWhenDone()
{
    _detachWhenDone = true;
}

void AEMovie::hideWhenDone()
{
    _hideWhenDone = true;
}

bool AEMovie::getEvent(const char* name, Vector2& p) const
{
    bool ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &p.x, &p.y) ? true : false;
    return ok;
}

spActor AEMovie::getRootSlot(const std::string& name, error_policy ep)
{
    //return getMultiSlot(name, ep);
    return getSlotInternal(name, ep);
}


spAESlot AEMovie::getSlotInternal(const std::string& name, error_policy ep)
{
    spAESlot actor = _slots[name];
    if (!actor)
    {
        const char *cn = ae_get_movie_composition_data_name(_compositionData);
        handleErrorPolicy(ep, "can't find slot '%s' in '%s/%s'", name.c_str(), _res->_folder.c_str(), cn);
    }

    return actor;
}

spActor  AEMovie::getSlotNormalized(const std::string& name, error_policy ep)
{
    spAESlot slot = getSlotInternal(name, ep);
    if (!slot)
        return 0;

    if (!slot->normalized)
    {
        spActor normalized = new AESlot;

        normalized->setName(slot->proxy->getName());//get original name without slot:
        normalized->attachTo(slot);
        normalized->setSize(slot->getScaledSize());
        normalized->setScale(1.0f / slot->getScaleX(), 1.0f / slot->getScaleY());

        slot->normalized = normalized;
    }

    return slot->normalized;
}

spActor AEMovie::getMultiSlot(const std::string& name, error_policy ep /*= ep_show_error*/)
{
    spAESlot slot = _slots[name];
    if (!slot)
    {
        const char *cn = ae_get_movie_composition_data_name(_compositionData);
        handleErrorPolicy(ep, "can't find slot '%s' in '%s/%s'", name.c_str(), _res->_folder.c_str(), cn);

        if (ep == ep_ignore_error)
            return 0;
        return new Actor;
    }

    return slot->proxy;
}

oxygine::timeMS AEMovie::getDuration() const
{
    return (timeMS)ae_get_movie_composition_duration(_composition);
}

bool AEMovie::getShowOverdraw() const
{
#ifdef AE_DEV_MODE
    return _showOverdraw;
#endif
    return false;
}

void AEMovie::resetMarkers()
{
    _markerA = AEMarker();
    _markerB = AEMarker();
    _markerC = AEMarker();
}

const Resources& AEMovie::getResources() const
{
    return _res->_res;
}

const ResAnim* AEMovie::getResAnim(const std::string& id, error_policy ep) const
{
    return _res->_res.getResAnim(id, ep);
}


void ae_movie_composition_node_destroyer(const aeMovieNodeDeleterCallbackData* _callbackData, void* _data)
{
    aeMovieLayerTypeEnum type = ae_get_movie_layer_data_type(_callbackData->layer);

    if (type == AE_MOVIE_LAYER_TYPE_SLOT ||
        type == AE_MOVIE_LAYER_TYPE_VIDEO ||
        type == AE_MOVIE_LAYER_TYPE_SOUND ||
        type == AE_MOVIE_LAYER_TYPE_PARTICLE)
    {
        Object* actor = (Object*)_callbackData->element_userdata;
        if (actor)
            actor->releaseRef();
    }
}


void AEMovie::ae_movie_composition_node_update_t(const aeMovieNodeUpdateCallbackData* _callbackData, void* _data)
{
    AEMovie *This = (AEMovie*)_data;

    int state = _callbackData->state;
    void* element = _callbackData->element_userdata;
    Matrix matrix = fromMatrix34(_callbackData->matrix);

    switch (ae_get_movie_layer_data_type(_callbackData->layer))
    {
        case AE_MOVIE_LAYER_TYPE_VIDEO:
        {
            /*
            MovieSprite* ins = (MovieSprite*)element;

            ins->setLooped(_callbackData->loop ? true : false);

            if (state == AE_MOVIE_STATE_UPDATE_BEGIN)
            {
                ins->play();
            }

            if (state == AE_MOVIE_STATE_UPDATE_END)
            {
                ins->stop();
            }
            */

        } break;

        case AE_MOVIE_LAYER_TYPE_SLOT:
        {
            AESlot* slot = (AESlot*)element;

            slot->world = matrix;
            Transform tr(matrix);
            
            setDecomposedTransform(slot, tr);

            slot->setTransform(tr);
            slot->setAlpha((unsigned char)(_callbackData->opacity * 255));
        } break;

        case AE_MOVIE_LAYER_TYPE_PARTICLE:
        {
            if (state == AE_MOVIE_STATE_UPDATE_PROCESS)
            {
                Actor* actor = (Actor*)element;

                Transform tr(matrix);
                actor->setTransform(tr);
                actor->setAlpha((unsigned char)(_callbackData->opacity * 255));
            }
        } break;


        default:
            break;
    }

}


void updateMask(Mask* mask, const aeMovieRenderMesh* mesh, const Transform *t = 0)
{
    mask->_mask = *mesh;

    const ResAnim* resAnim = (ResAnim*)ae_get_movie_resource_userdata(mesh->resource);
    if (!resAnim)
        return;
    mask->rs = resAnim;

    const AnimationFrame& frame = resAnim->getFrame(0);
    const RectF& srcX = frame.getSrcRect();



    Vector2 srcLeftTop     = Vector2(mesh->uv[0][0], mesh->uv[0][1]);
    Vector2 srcRightBottom = Vector2(mesh->uv[2][0], mesh->uv[2][1]);

    mask->texture = frame.getDiffuse().base;

    spNativeTexture alpha = frame.getDiffuse().alpha;
    if (alpha)
        mask->texture = alpha;


    Vector2 addUV = srcX.pos;
    Vector2 mulUV = srcX.size;

    srcLeftTop = srcLeftTop.mult(mulUV) + addUV;
    srcRightBottom = srcRightBottom.mult(mulUV) + addUV;


    ClipUV _clipUV;
	if (t)
	{
		_clipUV = ClipUV(
			Vector2(t->transform(Vector2(mesh->position[0][0], mesh->position[0][1]))),
			Vector2(t->transform(Vector2(mesh->position[1][0], mesh->position[1][1]))),
			Vector2(t->transform(Vector2(mesh->position[3][0], mesh->position[3][1]))),
			srcLeftTop,
			Vector2(srcRightBottom.x, srcLeftTop.y),
			Vector2(srcLeftTop.x, srcRightBottom.y));
	}
	else
	{
		_clipUV = ClipUV(
			Vector2(mesh->position[0][0], mesh->position[0][1]),
			Vector2(mesh->position[1][0], mesh->position[1][1]),
			Vector2(mesh->position[3][0], mesh->position[3][1]),
			srcLeftTop,
			Vector2(srcRightBottom.x, srcLeftTop.y),
			Vector2(srcLeftTop.x, srcRightBottom.y));
	}

    Vector2 v(2.0f / mask->texture->getWidth(), 2.0f / mask->texture->getHeight());

    //srcLeftTop -= v;
    //srcRightBottom += v;

    mask->clip = Vector4(srcLeftTop.x, srcLeftTop.y, srcRightBottom.x, srcRightBottom.y);
    _clipUV.get(mask->msk);
}

ae_bool_t AEMovie::ae_movie_composition_track_matte_create(const aeMovieTrackMatteProviderCallbackData * _callbackData, ae_voidptrptr_t user_data, ae_voidptr_t _data)
{
    Mask* mask = new Mask;
    updateMask(mask, _callbackData->mesh);

    AEMovie* This = (AEMovie*)_data;
    This->_masks.push_back(mask);

	*user_data = mask;
	return true;
    //return mask;
}

void AEMovie::ae_movie_composition_track_matte_delete(const aeMovieTrackMatteDeleterCallbackData * _callbackData, ae_voidptr_t _data)
{
    AEMovie* This = (AEMovie*)_data;
    //OX_ASSERT(0); 
    Mask* mask = (Mask*)_callbackData->track_matte_userdata;
    delete mask;

    if (mask)
    {
        auto it = std::find(This->_masks.begin(), This->_masks.end(), mask);
        OX_ASSERT(it != This->_masks.end());
        if (it != This->_masks.end())
            This->_masks.erase(it);        
    }
}


void AEMovie::ae_movie_composition_track_matte_update(const aeMovieTrackMatteUpdateCallbackData * _callbackData, ae_voidptr_t _data)
{
    AEMovie* This = (AEMovie*)_data;
    switch (_callbackData->state)
    {
        case AE_MOVIE_STATE_UPDATE_BEGIN:
        case AE_MOVIE_STATE_UPDATE_PROCESS:
        {
            Mask* mask = (Mask*)_callbackData->track_matte_userdata;
            if (mask)
                updateMask(mask, _callbackData->mesh);
        } break;

        default:
            break;
    }
}

ae_bool_t AEMovie::ae_movie_composition_node_provider(const aeMovieNodeProviderCallbackData* _callbackData, ae_voidptrptr_t userData, ae_voidptr_t _data)
{
    AEMovie* me = (AEMovie*)_data;


    switch (ae_get_movie_layer_data_type(_callbackData->layer))
    {
	    case AE_MOVIE_LAYER_TYPE_IMAGE:
	    {
		    int q = 0;
	    } break;

        case AE_MOVIE_LAYER_TYPE_VIDEO:
        {
#if 0
            const aeMovieResource *mr =  ae_get_movie_layer_data_resource(_callbackData->layer);
            const aeMovieResourceVideo* _resource = (const aeMovieResourceVideo*)mr;

            AEMovie* me = (AEMovie*)_data;
            spMovieSprite movie = MovieSprite::create();
            movie->setSkipFrames(false);

            std::string str = me->_res->getFolder() + std::string(_resource->path);
            str = path::extractBaseFileName(str) + ".ogv";
            if (file::exists(str.c_str()))
            {
                //movie->setMovie(str, _resource->alpha ? true : false);
                movie->setMovie(str, true);
                me->addChild(movie);

                movie->addRef();
				*userData = movie.get();
            }
#endif
        } break;

        case AE_MOVIE_LAYER_TYPE_PARTICLE:
        {
#ifdef HAVE_MP
            const aeMovieResourceParticle* _resource = (const aeMovieResourceParticle*)resource;
            HM_FILE hm = (HM_FILE)(size_t)_resource->data;

            MAGIC_FIND_DATA fd;
            const char* name = Magic_FindFirst(hm, &fd, MAGIC_EMITTER);
            HM_EMITTER em = Magic_LoadEmitter(hm, name);
            spMagicEmitter emitter = mp::createEmitter(em);
            me->addChild(emitter);
            emitter->addRef();

			*userData = emitter.get();
#endif
        } break;

        case AE_MOVIE_LAYER_TYPE_SOUND:
        {
            /*
            const aeMovieResourceSound * _resource = (const aeMovieResourceSound *)resource;
            AEMovie *me = (AEMovie*)_data;
            ResSound* snd = (ResSound*)_resource->data;

            spSoundInstance ins = me->_splayer.play(snd, PlayOptions().pause());
            ins->addRef();

            return ins.get();
            */
        } break;

        case AE_MOVIE_LAYER_TYPE_SLOT:
        {
            spAESlot slot = new AESlot;

            const aeMovieLayerData* layer = _callbackData->layer;

            const char *name_ = ae_get_movie_layer_data_name(layer);
            std::string name = name_;

            slot->setName("slot:" + name);
            //slot->setVisible(false);
            slot->addRef();

			*userData = slot.get();


            bool easy = name[0] == '*';
            if (easy)
				return true;

            spAESlot existingSlot = me->_slots[name];
           
            me->_slots[name] = slot;

            const aeMovieResource *mr = ae_get_movie_layer_data_resource(_callbackData->layer);
            const aeMovieResourceSlot* resource = (const aeMovieResourceSlot*)mr;

            slot->setSize(resource->width, resource->height);

            slot->setAlpha((unsigned char)(_callbackData->opacity * 255));                

            me->addChild(slot);

            Matrix m = fromMatrix34 (_callbackData->matrix);
            slot->world = m;
            Transform tr(m);
            //actor->setTransform(tr);
            setDecomposedTransform(slot.get(), tr);



            spActor proxy;
            if (existingSlot)
                proxy = existingSlot->proxy;
            else
            {
                proxy = new Actor;
                proxy->setName(name);
                proxy->attachTo(slot);
                proxy->setSize(slot->getSize());
                proxy->setHasOwnBounds();

#ifdef AE_DEBUG_SHOW_SLOTS
                spColorRectSprite clr = new ColorRectSprite;
                clr->attachTo(proxy);
                clr->setSize(proxy->getSize());
                clr->setAlpha(64);
#endif

                //proxy->setScale(1.0f / slot->getScaleX(), 1.0f / slot->getScaleY());
            }

            slot->proxy = proxy;
        } break;

        default:
            break;
    }

    return true;
}

void AEMovie::_completed()
{
    if (_timeLineFrom)
        return;
    OX_ASSERT(_playing);
    _playing = false;


    if (_hideWhenDone)
        setVisible(false);

    if (_detachWhenDone)
        detach();


    AutoRefHolder ar(this);

    Event ev(Event::COMPLETE);
    dispatchEvent(&ev);
}

void AEMovie::ae_movie_composition_state(const aeMovieCompositionStateCallbackData* _callbackData, void* _data)
{
    AEMovie* me = (AEMovie*)_data;
    switch (_callbackData->state)
    {
        case AE_MOVIE_COMPOSITION_END:
        {
            me->_completed();
        } break;

        default:
            break;
    }
}

void AEMovie::ae_movie_composition_event(const aeMovieCompositionEventCallbackData* _callbackData, void* _data)
{
    AEMovie* me = (AEMovie*)_data;

    UserEvent ev(_callbackData->name, _callbackData->begin ? true : false);
    me->dispatchEvent(&ev);

    UserEventBeginOrEnd ev2(_callbackData->begin ? UserEventBeginOrEnd::EVENT_BEGIN : UserEventBeginOrEnd::EVENT_END, _callbackData->name);
    me->dispatchEvent(&ev2);
}

void AEMovie::ae_movie_composition_callback_subcomposition_state(const aeMovieSubCompositionStateCallbackData * _callbackData, ae_voidptr_t _ud)
{

}

/*
void AEMovie::init(const AEMovieResource& res, const std::string& clip)
{
    const aeMovieCompositionData* compositionData = ae_get_movie_composition_data(res.movieData, clip.c_str());

    if (!compositionData)
    {
        logs::error("can't load composition %s:%s", res._folder.c_str(), clip.c_str());
    }
    OX_ASSERT(compositionData);

    if (!compositionData)
        return;

    init2(res, compositionData);
}*/

void AEMovie::init2(const AEMovieResource& res, const aeMovieCompositionData* compositionData)
{
    _weak = res._weak;
    _res = &res;
    _compositionData = compositionData;

    aeMovieCompositionProviders provider;
    ae_initialize_movie_composition_providers(&provider);

    provider.camera_provider = ae_movie_composition_node_camera;
  

    provider.track_matte_update = ae_movie_composition_track_matte_update;
    provider.track_matte_provider = ae_movie_composition_track_matte_create;
    provider.track_matte_deleter = ae_movie_composition_track_matte_delete;

    provider.node_provider = ae_movie_composition_node_provider;
    provider.node_deleter = ae_movie_composition_node_destroyer;
    provider.node_update = ae_movie_composition_node_update_t;

    provider.composition_state = ae_movie_composition_state;
    provider.composition_event = ae_movie_composition_event;

    provider.subcomposition_state = ae_movie_composition_callback_subcomposition_state;


    _composition = ae_create_movie_composition(res.getMovieData(), compositionData, _opt._interpolation, &provider, this);

    OX_ASSERT(_composition);

    if (!_composition)
        return;

    ae_vector3_t anchorPoint;
    if (ae_get_movie_composition_anchor_point(_composition, anchorPoint) == AE_TRUE)
    {
        setAnchorInPixels(anchorPoint[0], anchorPoint[1]);
    }

    float w = ae_get_movie_composition_data_width(_compositionData);
    float h = ae_get_movie_composition_data_height(_compositionData);

    setSize(w, h);
    
    spActor boundsSlot = getRootSlot("bounds", ep_ignore_error);
    if (boundsSlot)
    {
        setTouchEnabled(false);
        boundsSlot->setClickableWithZeroAlpha(true);
    }        
}

void AEMovie::handleEvent(Event* event)
{
    Actor::handleEvent(event);
}

void AEMovie::complete()
{
    if (!_playing)
        return;

    _completed();
}


bool AEMovie::checkMarker(AEMarker &marker, float tm0, float tm1)
{
    if (marker.act == AEMarker::action::nothing)
        return false;

    if (tm0 < marker.right  && marker.right <= tm1 )
    {
#ifdef OX_DEBUG
        //logs::messageln("ae marker hit in '%s' event '%s' with action %d (%.2f - %.2f)", getName().c_str(), marker.fromEvent.c_str(), marker.act, marker.left, marker.right);
#endif

        switch (marker.act) 
        {
        case AEMarker::action::completed:
            _completed();
            return true;
        case AEMarker::action::rewind2left:
            ae_set_movie_composition_time(_composition, marker.left);

            //ae_update_movie_composition(_composition, 1.0);
            return true;
        default:
            break;
        }
    }

    return false;
}

void AEMovie::doUpdate(const UpdateState& us)
{
    if (!_composition)
        return;

    AE_SHOULD_NOT_BE_DELETED();

    float dt = us.dt * _speed;

    ae_update_movie_subcompositions(_composition, dt);

    if (!_playing)
        return;

    if (_paused)
        return;

    if (_timeLineFrom)
        return;


//    DTAuto ae_upd("ae-upd");


    //float dt = us.dt * _speed;

    AutoRefHolder ar(this);

    float tm0 = ae_get_movie_composition_time(_composition);

    if (dt < 0)
    {
        float tm = tm0;
        tm += dt;
        if (tm < 0)
            tm += ae_get_movie_composition_duration(_composition);
        ae_set_movie_composition_time(_composition, tm);
    }
    else
        ae_update_movie_composition_main(_composition, dt);

#ifndef WORK_AREA_API
    if (_end != 0.0f)
    {
        if (ae_get_movie_composition_time(_composition) >= _end)
        {
            if (_workAreaLooped)
            {
                ae_set_movie_composition_time(_composition, _start);
                ae_update_movie_composition_main(_composition, 1);
            }
            else
            {
                ae_set_movie_composition_time(_composition, _end);
                ae_update_movie_composition_main(_composition, 1);
                _completed();
            }
        }
    }
#endif

    float tm1 = ae_get_movie_composition_time(_composition);

    auto markers = {&_markerA, &_markerB, &_markerC};
    for (AEMarker *m : markers)
    {
        if (checkMarker(*m, tm0, tm1))
            break;
    }
}

void AEMovie::render(const RenderState& parentRS)
{
    RenderState rs;
    if (!internalRender(rs, parentRS))
        return;
}

const aeMovieComposition* AEMovie::getAE()
{
    return _composition;
}

const aeMovieCompositionData* AEMovie::getAEData() const
{
    return _compositionData;
}

std::string AEMovie::dump(const dumpOptions& options) const 
{
    stringstream stream;
    stream << "{AEMovie}\n";
    /*

    stream << _vstyle.dump() << " ";
    std::string tname = "null";
    if (_frame.getDiffuse().base)
        tname = _frame.getDiffuse().base->getName();
    stream << "texture='" << tname << "' ";
    //if (_frame.getResAnim())
    //    stream << "resanim='" << _frame.getResAnim()->getName() << "' ";
    if (_flags & flag_manageResAnim)
        stream << "manageResAnim=true";

    if (_localScale != Vector2(1.0f, 1.0f))
        stream << " localScale=(" << _localScale.x << "," << _localScale.y << ")";
        */

    stream << Actor::dump(options);
    return stream.str();
}

Vector2 AEMovie::getEventPosition(const char* name) const
{
    Vector2 p(0,0);
    bool ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &p.x, &p.y) ? true : false;
    OX_ASSERT(ok);
    return p;
}


int AEMovie::getEventDuration(const char* name) const
{
    Vector2 v = getEventPosition(name);
    return (int)(v.y - v.x);
}

oxygine::Vector2 AEMovie::getLoopPosition() const
{
    float v1, v2;
    ae_get_movie_composition_data_loop_segment(_compositionData, &v1, &v2);
    return Vector2(v1, v2);
}

void AEMovie::play(bool looped, float from)
{
    _start = 0;
    _end = 0;

    playFrom(looped, from);
}

void AEMovie::playFrom(bool looped, float from)
{
    if (!_composition)
        return;

    _looped = looped;
    _playing = true;
    _paused = false;

    ae_stop_movie_composition(_composition);
    ae_play_movie_composition(_composition, from);
    ae_set_movie_composition_loop(_composition, looped);
    ae_update_movie_composition(_composition, 1);
}

void AEMovie::justPlay()
{
    _playing = true;
    _paused = false;
    ae_play_movie_composition(_composition, -1.0f);
}

void AEMovie::pause()
{
    _paused = true;
}

void AEMovie::stop()
{
    _playing = false;
    ae_stop_movie_composition(_composition);
}

void AEMovie::resume()
{
    _paused = false;
}


oxygine::timeMS AEMovie::getTime() const
{
    return (timeMS)ae_get_movie_composition_time(_composition);
}

void AEMovie::setTime(timeMS tm)
{
    //OX_ASSERT(0);
    ae_set_movie_composition_time(_composition, (float)tm);
    //while (_composition->time < tm)
    //  ae_update_movie_composition(_composition, 50);
}

void AEMovie::setSpeed(float s)
{
    _speed = s;
}

const oxygine::Color& AEMovie::getColor() const
{
    return _color;
}

void AEMovie::resetWorkArea()
{
    _workAreaLooped = false;
    _start = _end = 0.0;
}

void AEMovie::setWorkArea(const char* name, bool looped)
{
    float a, b;
    ae_bool_t ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &a, &b);
    OX_ASSERT(ok);
    _start = a;
    _end = b;
    _workAreaLooped = looped;

    if (ae_get_movie_composition_time(_composition) >= _end)
    {
        ae_set_movie_composition_time(_composition, _start);
    }
}


void AEMovie::setUseTimelineFrom(AEMovie* tm)
{
    _timeLineFrom = tm;
    _lastTimeLineTime = ae_get_movie_composition_time(tm->_composition);
    ae_set_movie_composition_time(_composition, _lastTimeLineTime);
    ae_play_movie_composition(_composition, 1);
}

void AEMovie::playArea(float a, float b, bool loop)
{
    if (_playing && a == _start && b == _end)
    {
        _workAreaLooped = loop;
        return;
    }

#ifdef WORK_AREA_API
    ae_set_movie_composition_work_area(_composition, _start, _end);
#else

    play(false);
    ae_set_movie_composition_time(_composition, a);
#endif
    _workAreaLooped = loop;
    _start = a;
    _end = b;
}

void AEMovie::playEvent(const char* name, bool loop)
{
    float a, b;
    ae_bool_t ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &a, &b);
    OX_ASSERT(ok);
    if (!ok)
        return;

    playArea(a, b, loop);
}

void AEMovie::setTimeFromEventStart(const char* name)
{
    float a, b;
    ae_bool_t ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &a, &b);
    OX_ASSERT(ok);

    ae_set_movie_composition_time(_composition, a);
    //_workAreaLooped = false;
    //_start = 0;
    //_end = 0;
}

void AEMovie::setTimeFromEventEnd(const char* name)
{
    float a, b;
    ae_bool_t ok = ae_get_movie_composition_node_in_out_time(_composition, name, AE_MOVIE_LAYER_TYPE_EVENT, &a, &b);
    OX_ASSERT(ok);

    ae_set_movie_composition_time(_composition, b);
    //_workAreaLooped = false;
    //_start = 0;
    //_end = 0;
}

void AEMovie::setTime(float tm)
{
    ae_set_movie_composition_time(_composition, tm);

}

void AEMovie::setShowHideSlots(const std::string& name)
{
    _showHideSlots = name;
}

int AEMovie::getWireframeMode() const
{
#ifdef AE_DEV_MODE
    return _wireframe;
#else
    return -1;
#endif
}

int AEMovie::getLastBatches() const
{
#ifdef AE_DEV_MODE
    return _batches;
#else
    return -1;
#endif
}

void AEMovie::setWireframeMode(int layer)
{
#ifdef AE_DEV_MODE
    _wireframe = layer;
#endif

}

void AEMovie::setShowOverdraw(bool en)
{
#ifdef AE_DEV_MODE
    _showOverdraw = en;
#endif
}

const aeMovieSubComposition* AEMovie::getSubMovie(const std::string& name, error_policy ep)
{
	const aeMovieSubComposition* sm = ae_get_movie_sub_composition(_composition, name.c_str());
	if (!sm)
		handleErrorPolicy(ep, "can't find submovie %s in %s", name.c_str(), getName().c_str());
	return sm;
}

void AEMovie::submoviePlay(const aeMovieSubComposition* sub, bool loop, float tm)
{
    ae_set_movie_sub_composition_loop(sub, loop);
    ae_play_movie_sub_composition(_composition, sub, tm);
}

void AEMovie::submovieStop(const aeMovieSubComposition* sub)
{
    ae_stop_movie_sub_composition(_composition, sub);
}

bool AEMovie::submovieIsPlaying(const aeMovieSubComposition* sub)
{
    return ae_is_play_movie_sub_composition(sub) ? true : false;
}

void AEMovie::submovieInterrupt(const aeMovieSubComposition* sub, bool skip2end)
{
    ae_interrupt_movie_sub_composition(_composition, sub, skip2end);
}

float AEMovie::submovieGetTime(const aeMovieSubComposition* sub)
{
    return ae_get_movie_sub_composition_time(sub);
}

Vector2 AEMovie::submovieGetLoopPosition(const aeMovieSubComposition* sub) const
{
    float a, b;
    ae_get_movie_sub_composition_in_out_loop(sub, &a, &b);
    return Vector2(a, b);
}

void AEMovie::smPlay(const std::string& name, bool loop)
{
    const aeMovieSubComposition* sm = getSubMovie(name);
    if (!sm)
        return;

    submovieStop(sm);
    submoviePlay(sm, loop);
}

bool AEMovie::isPlaying() const
{
    return _playing;
}

bool AEMovie::isPaused() const
{
    return _paused;
}

bool AEMovie::isInLoop() const
{
    Vector2 loop = getLoopPosition();
    if (getTime() > loop.x && getTime() < loop.y)
        return true;
    return false;
}

void AEMovie::doRender(const RenderState& rs)
{
    AE_SHOULD_NOT_BE_DELETED();

#ifdef AE_DEV_MODE
    _batches = 0;
#endif

    if (!_composition)
        return;

//    DTAuto ae_ren("ae-ren-o");

    if (_timeLineFrom)
    {
        float maintm = ae_get_movie_composition_time(_timeLineFrom->_composition);
        float loctm = ae_get_movie_composition_time(_composition);
		float dt = maintm - loctm;
		//logs::messageln("dt %f", dt);
        if (dt >= 0)		
            ae_update_movie_composition(_composition, dt);
        else
            ae_set_movie_composition_time(_composition, maintm);
    }


    Material::null->apply();

#if OXYGINE_RENDERER >= 5
    STDRenderer* stdr = STDRenderer::getCurrent();
#else
    STDRenderer* stdr = ((STDMaterial*)rs.material)->getRenderer();
#endif


    IVideoDriver* driver = stdr->getDriver();
    //stdr->resetSettings();

    const VertexDeclaration* decl = driver->getVertexDeclaration(VERTEX_PCT2);


    Matrix vp = stdr->getViewProjection();
    Matrix wvp = rs.transform.toMatrix() * vp;

    
    AERenderer::getShader gs = [=](int f)
    {
		return _uberShader->getShaderProgram(f);
    };

    AERenderer renderer(driver, gs, wvp, _validate);

    Color clr = _color.withAlpha(rs.alpha);// .premultiplied();
    renderer.setColor(clr);
    renderer.setAddColor(_addColor);
    renderer.reset();


#ifdef AE_DEV_MODE
    renderer.setWireMode(_wireframe);
#endif


    bool hidden = false;


    aeMovieRenderMesh mesh;
    uint32_t mesh_iterator = 0;


#ifdef AE_TEST_PERF
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
#endif

    int n = 0;

    string show_hide;
    if (!_showHideSlots.empty())
    {
        show_hide = "slot:" + _showHideSlots;
    }

    while (ae_compute_movie_mesh(_composition, &mesh_iterator, &mesh) == AE_TRUE)
    {
        ++n;
//        DTManual ae_ren_i("ae-ren-i");

        if (hidden)
        {
            if (mesh.layer_type == AE_MOVIE_LAYER_TYPE_SLOT)
            {
                Actor* actor = (Actor*)mesh.element_userdata;
                if (!show_hide.empty() && actor->getName().rfind(show_hide, 0) == 0)
                    hidden = false;
            }
            continue;
        }

        const Mask* msk = (const Mask*)mesh.track_matte_userdata;

#ifdef AE_TEST_PERF
        if (test::AeMasksDisabled)
            msk = 0;
#endif

        if (msk)
        {
#ifdef ONLINE
            preloader::preload(msk->rs);
#endif

            //mesh = msk->_mask;
            //  mesh.layer_type = AE_MOVIE_LAYER_TYPE_IMAGE;

            /*
            if (0)
            {
                renderer.setMask(0);
                renderer.flush();
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
                renderer.setTexture(frame.getDiffuse().base, 0);
                driver->setState(IVideoDriver::STATE_BLEND, 0);
                //buildVD(renderer, mesh, src);
                renderer.flush();
                driver->setState(IVideoDriver::STATE_BLEND, 1);
            }
            */
        }
#ifdef AE_DEV_MODE
        if (_showOverdraw)
            renderer.setBlendMode(IVideoDriver::BT_ONE, IVideoDriver::BT_ONE);
        else
#endif
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
                default:
                    break;
            };

#ifdef AE_DEV_MODE
        if (!_showOverdraw)
            renderer.setMask(msk);
#else
        renderer.setMask(msk);
#endif



        switch (mesh.layer_type)
        {
            case AE_MOVIE_LAYER_TYPE_SLOT:
            {
                AESlot * slot = (AESlot*)mesh.element_userdata;
                if (slot)
                {
                    renderer.flush();

                    rsCache().reset();

                    slot->setAlpha(std::min(255, int(mesh.opacity * 255)));
                    //slot->setVisible(true);


					if (slot->proxy && slot->proxy->getParent() != slot)
						slot->proxy->attachTo(slot);

					
					if (msk)
					{

						Mask m = *msk;
						updateMask(&m, &msk->_mask, &rs.transform);

						ShaderProgramChangedHook hook;
						hook.hook = [&]()
						{
							IVideoDriver::instance->setUniform("clip_mask", m.clip);
							IVideoDriver::instance->setUniform("msk", m.msk, 4);
						};


						rsCache().setTexture(UberShaderProgram::SAMPLER_MASK, m.texture);

						STDRenderer *stdr = STDRenderer::getCurrent();

						int sflags = stdr->getBaseShaderFlags();
						int baseShaderFlags = sflags;

						baseShaderFlags |= UberShaderProgram::MASK;

						stdr->pushShaderSetHook(&hook);
						stdr->setBaseShaderFlags(baseShaderFlags);

						slot->render(rs);
						Material::null->apply();

						stdr->popShaderSetHook();
						stdr->setBaseShaderFlags(sflags);
					}
					else
					{
						slot->render(rs);
						Material::null->apply();
					}

                    renderer.reset();

                    if (!show_hide.empty() && slot->getName().rfind(show_hide, 0) == 0)
                        hidden = true;
                }
            }
            break;

            case AE_MOVIE_LAYER_TYPE_PARTICLE:
            {
                renderer.flush();

                Actor* actor = (Actor*)mesh.element_userdata;
                actor->render(rs);

                renderer.reset();
            } break;

            case AE_MOVIE_LAYER_TYPE_SOLID:
            {
                renderer.setTexture(STDRenderer::white, STDRenderer::white, 0);
                RectF src(0, 0, 1, 1);
                buildVD(renderer, mesh, src, clr);
            }
            break;

            case AE_MOVIE_LAYER_TYPE_IMAGE:
            case AE_MOVIE_LAYER_TYPE_SEQUENCE:
            {
                ResAnim* rs = (ResAnim*)ae_get_movie_resource_userdata(mesh.resource);
#ifdef ONLINE
                preloader::preload(rs);
#endif

                const AnimationFrame& frame = rs->getFrame(0);
                const RectF& src = frame.getSrcRect();
#ifdef AE_DEV_MODE
                if (_showOverdraw)
                {
                    renderer.setTexture(STDRenderer::white, STDRenderer::white, 0);
                    Color clr = _color;
                    if (clr.a >= 64)
                        clr.a /= msk ? 4 : 8;
                    buildVD(renderer, mesh, src, clr);
                }
                else
#endif
                {
                    const Diffuse &df = frame.getDiffuse();
                    renderer.setTexture(df.base, df.alpha, df.flags);
                    buildVD(renderer, mesh, src, clr);
                }

            } break;

            case AE_MOVIE_LAYER_TYPE_VIDEO:
            {
#if 0
                MovieSprite* movie = (MovieSprite*)mesh.element_data;
                if (movie)
                {
                    renderer.flush();

                    const AnimationFrame& frame = movie->getAnimFrame();
                    const RectF& src = frame.getSrcRect();

                    movie->convert();

                    renderer.setShader([](int f)
                    {
                        return MovieSprite::_shader->getShaderProgram(f);
                    });


                    renderer.validate();

                    const Diffuse& d = frame.getDiffuse();

                    movie->_shader->apply(driver, d.base, d.alpha);
                    Vector2 yaScale(1, 1);
                    driver->setUniform("yaScale", &yaScale, 1);

                    buildVD(renderer, mesh, src, clr);

                    renderer.flush();
                    renderer.reset();

                    renderer.setShader(gs);
                    renderer.validate();

                }
#endif

            } break;
            default:
                break;
        }

    }

    renderer.flush();

#ifdef AE_TEST_PERF
    if (test::AeHalfFillrate)
    {
        driver->setScissorRect(0);
    }
#endif


    DebugActor::addDebugString("AE batches: %d", renderer._batches);

    rsCache().reset();

#ifdef AE_DEV_MODE
    _batches = renderer._batches;
#endif
    ;
}

AESlot::AESlot()
{
    _flags |= flag_actorHasBounds;
}

AEMarker::AEMarker(spAEMovie movie, const char *name, bool loop):AEMarker(movie.get(), name, loop)
{
}

AEMarker::AEMarker(AEMovie *movie, const char *name, bool loop)
{
    Vector2 pos = movie->getEventPosition(name);
    right = pos.y;
    left = pos.x;
    act = loop ? action::rewind2left : action::completed;

#ifdef OX_DEBUG
    fromEvent = name;
#endif
}

AEMarker AEMarker::withRight(float pos)
{
    AEMarker m = *this;
    m.right = pos;
    return m;
}

AEMarker AEMarker::withLeft(float pos)
{
    AEMarker m = *this;
    m.left = pos;
    return m;
}

AEMarker AEMarker::withAction(action act)
{
    AEMarker m = *this;
    m.act = act;
    return m;
}
