#pragma once
#include "ae/AEMovie.h"
#include "ae/AEMovieResource.h"

using namespace std;

DECLARE_SMART(AEMovieWork, spAEMovieWork);
class AEMovieWork : public AEMovie
{
public:
    static spAEMovieWork createWork(AEMovieResource& res, const string& comp, error_policy ep = ep_show_error);

    AEMovieWork() : _show_texture(true) {}

    //void doRender(const RenderState& rs) override;

    bool computeAllTimelineBounds(RectF & out);
    bool calcMinMaxForAllFrames(RectF & out);

    bool _show_texture;
};