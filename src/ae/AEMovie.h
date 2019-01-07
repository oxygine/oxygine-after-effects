#pragma once
#include "AEMovieResource.h"

using namespace oxygine;

DECLARE_SMART(AESlot, spAESlot);
class AESlot : public Actor
{
public:
    AESlot();

    Matrix world;
    spActor proxy;
    spActor normalized;
};

class AEMovieOptions
{
public:
    AEMovieOptions withInterpolation(bool enabled = true) { AEMovieOptions c = *this;  c._interpolation = enabled; return c; }
    AEMovieOptions withoutInterpolation(bool disabled = true) { AEMovieOptions c = *this;  c._interpolation = !disabled; return c; }

    bool _interpolation = true;
};

DECLARE_SMART(AEMovie, spAEMovie);
class AEMarker
{
public:
    enum class action {
        rewind2left,
        completed,
        nothing,
    };

    action act = action::nothing;
    float right = 0.0f;
    float left = 0.0f;

    AEMarker() {}
    AEMarker(AEMovie *movie, const char *eventName, bool loop = true);
    AEMarker(spAEMovie movie, const char *eventName, bool loop = true);

    AEMarker withPosition(float pos) { return withRight(pos); }
    AEMarker withRight(float pos);
    AEMarker withValue(float pos) { return withLeft(pos); }
    AEMarker withLeft(float pos);
    AEMarker withAction(action act);
    AEMarker withActionCompleted() { return withAction(action::completed); }

    Vector2 asVector2() const { return Vector2(left, right); }

    void off() { act = action::nothing; }

#ifdef OX_DEBUG
    std::string fromEvent;
#endif
};


class AEMovie : public Actor
{
public:
    static spAEMovie create(const AEMovieResource& res, const std::string& comp, error_policy ep = ep_show_error, bool interpolation = true);
    static spAEMovie create(const AEMovieResource& res, const std::string& comp, const AEMovieOptions& opt, error_policy ep = ep_show_error);

   
    ~AEMovie();

    class InterruptEvent: public Event
    {
    public:
        enum { EVENT = EventID("AEIn") };
        InterruptEvent(): Event(EVENT), skip2end(false) {}
        bool skip2end;
    };


    class UserEvent : public Event
    {
    public:
        enum
        {
            EVENT = EventID("AEUs")
        };
        UserEvent(const std::string& Id, bool Begin) : Event(EVENT), begin(Begin), id(Id) {}
        bool begin;
        std::string id;
    };

    class UserEventBeginOrEnd : public Event
    {
    public:
        enum
        {
            EVENT_BEGIN = EventID("AEUB"),
            EVENT_END = EventID("AEUE")
        };
        UserEventBeginOrEnd(int eventID, const std::string& Id) : Event(eventID), id(Id) {}
        std::string id;
    };

    void init2(const AEMovieResource& res, const aeMovieCompositionData* comp);

    void play(bool looped, float from = 0);
    void playFrom(bool looped, float from);
    void justPlay();
    void pause();
    void stop();
    void resume();
    void interrupt(bool skip2end = false);    
    void smPlay(const std::string& name, bool loop);

    void resetWorkArea();
    void resetMarkers();

    void submoviePlay(const aeMovieSubComposition* sub, bool loop, float tm = 0);
    void submovieStop(const aeMovieSubComposition* sub);
    bool submovieIsPlaying(const aeMovieSubComposition* sub);
    void submovieInterrupt(const aeMovieSubComposition* sub, bool skip2end = false);
    float submovieGetTime(const aeMovieSubComposition* sub);
    Vector2 submovieGetLoopPosition(const aeMovieSubComposition* sub) const;

    bool isPlaying() const;
    bool isPaused() const;
    bool isInLoop() const;

    bool                          getInterpolation() const { return _opt._interpolation; }
    timeMS                        getTime() const;
    float                         getSpeed() const { return _speed; }
    const Color&                  getColor() const;
    const Color&                  getAddColor() const { return _addColor; }
    bool                          getEvent(const char* name, Vector2& out) const;
                                  
    spActor                       getRootSlot(const std::string& name, error_policy ep = ep_show_error);
    spActor                       getSlotNormalized(const std::string& name, error_policy ep = ep_show_error);
    spActor                       getMultiSlot(const std::string& name, error_policy ep = ep_show_error);
    bool                          getShowOverdraw() const;
    int                           getWireframeMode() const;
    int                           getLastBatches() const;
    timeMS                        getDuration() const;
    Vector2                       getEventPosition(const char* name) const;
    int                           getEventDuration(const char* name) const;
    Vector2                       getLoopPosition() const;

    const aeMovieComposition*     getAE();
    const aeMovieCompositionData* getAEData() const;
    const Resources&              getResources() const;
    const ResAnim*                getResAnim(const std::string& id, error_policy ep = ep_show_error) const;
    const aeMovieSubComposition*  getSubMovie(const std::string& name, error_policy ep = ep_show_error);

    
    void detachWhenDone();
    void hideWhenDone();

    

    void playEvent(const char* name, bool loop);
    void playArea(float start, float end, bool loop);

    void setTimeFromEventStart(const char* name);
    void setTimeFromEventEnd(const char* name);

    void setWorkAreaLoop(bool en) { _workAreaLooped = false; }
	void setUberShader(UberShaderProgram *us) { _uberShader = us; }
	void setCustomShaderValidate(const std::function< void() > &validate) { _validate = validate; }
    void setShowHideSlots(const std::string& name);    
    void setWorkArea(const char* name, bool looped = true);

    void setUseTimelineFrom(AEMovie*);
    void setTime(float);
    void setWireframeMode(int layer);
    void setShowOverdraw(bool en);
    void setTime(timeMS tm);
    void setSpeed(float s);
    void setColor(const Color& c);
    void setAddColor(const Color& c);

    void complete();
    
    AEMarker& markerA() { return _markerA; }
    AEMarker& markerB() { return _markerB; }
    AEMarker& markerC() { return _markerC; }

    
    void doRender(const RenderState& rs) override;
    void render(const RenderState& rs) override;
    
    typedef Property<Color, const Color&, AEMovie, &AEMovie::getColor, &AEMovie::setColor>       TweenColor;
    typedef Property<Color, const Color&, AEMovie, &AEMovie::getAddColor, &AEMovie::setAddColor> TweenAddColor;

    spObject userObject;

    std::string dump(const dumpOptions& options) const override;

protected:

    spAESlot getSlotInternal(const std::string& name, error_policy ep);

    AEMovie(const AEMovieOptions& opt = AEMovieOptions());

    void _completed();

    AEMovieOptions _opt;
    Color _color;
    Color _addColor;


    AEMarker _markerA;
    AEMarker _markerB;
    AEMarker _markerC;

	UberShaderProgram *_uberShader = 0;
	std::function< void() > _validate;

    spAEMovieResourceWeak _weak;

    bool _looped = false;
    bool _playing = false;
    bool _paused = false;
    const AEMovieResource* _res;

#ifdef AE_DEV_MODE

    int  _wireframe = -1;
    int _batches = 0;
    bool _showOverdraw = false;

#endif

    float _start = 0;
    float _end = 0;
    float _lastTimeLineTime = 0;
    float _speed = 1.0f;
    bool  _workAreaLooped = false;

    std::vector<class Mask*> _masks;

    AEMovie(const AEMovie&);


    static ae_bool_t ae_movie_composition_node_provider(const aeMovieNodeProviderCallbackData* _callbackData, ae_voidptrptr_t userData, ae_voidptr_t _data);

    static void  ae_movie_composition_track_matte_update(const aeMovieTrackMatteUpdateCallbackData* _callbackData, void* _data);
    static ae_bool_t ae_movie_composition_track_matte_create(const aeMovieTrackMatteProviderCallbackData * _callbackData, ae_voidptrptr_t user_data, ae_voidptr_t _data);
    static void  ae_movie_composition_track_matte_delete(const aeMovieTrackMatteDeleterCallbackData* _callbackData, void* _data);

    static void ae_movie_composition_node_update_t(const aeMovieNodeUpdateCallbackData* _callbackData, void* _data);
    static void ae_movie_composition_state(const aeMovieCompositionStateCallbackData* _callbackData, void* _data);
    static void ae_movie_composition_event(const aeMovieCompositionEventCallbackData* _callbackData, void* _data);
    static void ae_movie_composition_callback_subcomposition_state(const aeMovieSubCompositionStateCallbackData * _callbackData, ae_voidptr_t _ud);

    void doUpdate(const UpdateState& us) override;
    void handleEvent(Event* event) override;

    bool checkMarker(AEMarker &m, float tm0, float tm1);

    std::unordered_map<std::string, spAESlot> _slots;

    bool _detachWhenDone = false;
    bool _hideWhenDone = false;
    
    const aeMovieComposition*       _composition = 0;
    const aeMovieCompositionData*   _compositionData = 0;    
    
    AEMovie* _timeLineFrom = 0;

    std::string _showHideSlots;
};