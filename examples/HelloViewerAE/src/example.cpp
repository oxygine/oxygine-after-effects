#include "oxygine-framework.h"

#include "test.h"
#include "ae/AEMovie.h"
#include "AEMovieWork.h"
#include "ox/ZipFileSystem.hpp"
//#include "MovieSprite.h"

using namespace oxygine;



//#define MULTIWINDOW 1

#if MULTIWINDOW
spStage stage2;
#endif

AEMovieResource movieRes;

extern std::string aeProject;
extern std::string aeCurrent;
extern Vector2 aeWorkArea;

DECLARE_SMART(Preview, spPreview);

namespace test
{
	bool AeMasksDisabled = false;
	bool AeDontDraw = false;
	int AeHalfFillrate = 0;
}


class Preview : public Test
{
public:
	static spPreview instance;

	spAEMovieWork movie;
    spColorRectSprite _cr;
    spColorRectSprite _cr2;
	bool _looped = false;
    static bool _bb;
    static bool _show_all;
    float _allscale = 1.0f;

	Preview(const string &id)
	{
		addButton("play_loop", "play loop");
        addButton("play_once", "play once");
        addButton("interrupt", "interrupt");
        addButton("pause", "pause");
        addButton("resume", "resume");
        addButton("wa", "ae work area");
        addButton("wireframe", "wireframe");

		//addButton("int_off", "interpolation off");
		//addButton("int_on", "interpolation on");

        addButton("show_all", _show_all ? "show all: on" : "show all: off");

        addButton("bounding_box", _bb ? "bounding box: on" : "bounding box: off");

        addButton("overdraw", "show overdraw");
        addButton("mask", "disable mask");

		_color = Color::Red;
		_txtColor = Color::White;
		//()
		

		Test::toggle t[2] = { {"use work area", 1}, {"no work area", 2} };
		//addToggle("wa", t, 2);

		addClickListener([=](Event*) {

		});

        addEventListener(TouchEvent::WHEEL_DIR, [=] (Event* ev) {
            TouchEvent *te = safeCast<TouchEvent*>(ev);
            
            int b = movie->getWireframeMode();
            if (te->wheelDirection.y < 0)
            {
                b += 1;
            }

            if (te->wheelDirection.y > 0)
            {
                b -= 1;
            }

            if (b < -1)
                b = movie->getLastBatches();

            if (b > 0)
            {
                b = b % (movie->getLastBatches() + 1 );
            }

            movie->setWireframeMode(b);

        });

		set(id);

	}

	void toggleClicked(string id, const toggle* data)
	{
	}

	void clicked(string id)
	{
		size_t p = id.find(':');
		if (p != id.npos)
		{
			string ev = id.substr(p + 1);
			movie->playEvent(ev.c_str(), true);
		}

		if (id == "wa")
		{
			movie->playArea(aeWorkArea.x * 1000.0f, aeWorkArea.y * 1000.0f, true);
		}

        if (id == "play_loop")
        {
            movie->play(true);
        }

        if (id == "play_once")
        {
            movie->play(false);
        }

        if (id == "pause")
        {
            movie->pause();
        }

        if (id == "resume")
        {
            movie->resume();
        }

        if (id == "wireframe")
        {

        }

        if (id == "mask")
        {
            test::AeMasksDisabled = !test::AeMasksDisabled;
            updateText("mask", test::AeMasksDisabled ? "enable mask" : "disable mask");
        }

		if (id == "interrupt")
		{
			movie->interrupt();
		}

		if (id == "int_on")
		{
//			movie->setInterpolation(true);
		}

		if (id == "int_off")
		{
	//		movie->setInterpolation(false);
		}

        if (id == "bounding_box") {
            _bb = !_bb;
            updateText("bounding_box", _bb ? "bounding box: on" : "bounding box: off");
        }

        if (id == "overdraw") 
        {
            movie->setShowOverdraw(!movie->getShowOverdraw());
            updateText("overdraw", !movie->getShowOverdraw() ? "show overdraw" : "hide overdraw");
        }

        if (id == "show_all")
        {
            _show_all = !_show_all;
            updateText("show_all", _show_all ? "show all: on" : "show all: off");
        }
    }

	void update(const UpdateState& us)
	{
		Test::update(us);
		if (!movie)
			return;

        char str[255];
        safe_sprintf(str, "wireframe: %d / %d", movie->getWireframeMode(), movie->getLastBatches());
        updateText("wireframe", str);

		if (key::wasPressed(SDL_SCANCODE_SPACE))
		{
			if (!movie->isPaused())
			{
				movie->pause();
				notify("pause");
			}
			else
			{
				movie->resume();
				notify("resume");
			}
		}


        float sx, sy;
        float scale = 1.0f;
        if (_bb)
        {
            RectF rc;
            if (movie->computeAllTimelineBounds(rc))
            {
                Vector2 off = movie->getSize() * 0.5f;

                Matrix m = movie->getParent()->computeGlobalTransform().toMatrix();
                Vector4 p0 = m.transformVec4(Vector4(rc.getLeft() - off.x, rc.getTop() - off.y, 0.0f, 1.0f));
                Vector4 p1 = m.transformVec4(Vector4(rc.getRight() - off.x, rc.getBottom() - off.y, 0.0f, 1.0f));

                sx = std::max(fabs(p0.x), fabs(p1.x))*1.1f;
                sy = std::max(fabs(p0.y), fabs(p1.y))*1.1f;

                sx = (0.5f * getStage()->getWidth()) / sx;
                sy = (0.5f * getStage()->getHeight()) / sy;
                scale = std::min(sx, sy);

                if (_show_all)
                {
                    scale = std::min(scale, _allscale);
                }
                else
                {
                    sx = getStage()->getWidth() / movie->getWidth();
                    sy = getStage()->getHeight() / movie->getHeight();
                    scale = std::min(scale, std::min(sx, sy));
                }

                movie->setScale(std::min(scale, 1.0f));
                movie->setPosition(getStage()->getSize() / 2 - movie->getScaledSize() / 2);

                m = movie->computeGlobalTransform().toMatrix();
                Vector4 pp0 = m.transformVec4(Vector4(rc.getLeft(), rc.getTop(), 0.0f, 1.0f));
                Vector4 pp1 = m.transformVec4(Vector4(rc.getRight(), rc.getBottom(), 0.0f, 1.0f));
                _cr2->setPosition(Vector2(pp0.x - 2.0f, pp0.y - 2.0f));
                _cr2->setSize(Vector2(pp1.x - pp0.x + 4.0f, pp1.y - pp0.y + 4.0f));
                _cr2->setVisible(true);

                _cr->setPosition(Vector2(pp0.x, pp0.y));
                _cr->setSize(Vector2(pp1.x - pp0.x, pp1.y - pp0.y));
                _cr->setVisible(true);
            }
        }
        else
        {
            if (_show_all)
            {
                movie->setScale(_allscale);
            }
            else
            {
                sx = getStage()->getWidth() / movie->getWidth();
                sy = getStage()->getHeight() / movie->getHeight();
                movie->setScale(std::min(1.0f, std::min(sx, sy)) * 0.9f);
            }

            movie->setPosition(getStage()->getSize() / 2 - movie->getScaledSize() / 2);

            _cr->setVisible(false);
            _cr2->setVisible(false);
        }
	}

	void set(const string &id)
	{
		if (movie)
			movie->detach();

        _cr2 = new ColorRectSprite;
        _cr2->setColor(Color(255, 0, 0, 255));
        _cr2->attachTo(this);
        _cr2->setPriority(-3);
        _cr2->setVisible(false);

        _cr = new ColorRectSprite;
        _cr->setColor(Color(0,0,0,255));
        _cr->attachTo(this);
        _cr->setPriority(-2);
        _cr->setVisible(false);

		movie = AEMovieWork::createWork(movieRes, id);
		if (!movie)
			return;
		movie->attachTo(this);
		movie->setPriority(-1);
		movie->setAnchor(0.0f, 0.0f);
		movie->play(true);





        float sx, sy, scale;
        RectF rc;
        if (movie->calcMinMaxForAllFrames(rc))
        {
            Vector2 off = movie->getSize() * 0.5f;

            Matrix m = movie->getParent()->computeGlobalTransform().toMatrix();
            Vector4 p0 = m.transformVec4(Vector4(rc.getLeft() - off.x, rc.getTop() - off.y, 0.0f, 1.0f));
            Vector4 p1 = m.transformVec4(Vector4(rc.getRight() - off.x, rc.getBottom() - off.y, 0.0f, 1.0f));

            sx = std::max(fabs(p0.x), fabs(p1.x))*1.1f;
            sy = std::max(fabs(p0.y), fabs(p1.y))*1.1f;

            sx = (0.5f * getStage()->getWidth()) / sx;
            sy = (0.5f * getStage()->getHeight()) / sy;
            scale = std::min(sx, sy);

            sx = getStage()->getWidth() / movie->getWidth();
            sy = getStage()->getHeight() / movie->getHeight();
            scale = std::min(scale, std::min(sx, sy));

            _allscale = std::min(scale, 1.0f);
        }

        if (_show_all)
        {
            movie->setScale(_allscale);
        }
        else
        {
            sx = getStage()->getWidth() / movie->getWidth();
            sy = getStage()->getHeight() / movie->getHeight();
            movie->setScale(std::min(1.0f, std::min(sx, sy)) * 0.9f);
        }
        movie->setPosition(getStage()->getSize() / 2 - movie->getScaledSize() / 2);


		movie->addEventListener(Event::COMPLETE, [=](Event*) {
			notify("COMPLETE");
		});



		int events = ae_get_movie_composition_data_event_count(movie->getAEData());
		for (int i = 0; i < events; ++i)
		{
			const char *name = ae_get_movie_composition_data_event_name(movie->getAEData(), i);

			addButton("event:" + string(name), name);
		}
	}
};

bool Preview::_bb = false;
bool Preview::_show_all = false;

spPreview Preview::instance;


class TestActor: public Test
{
public:

    TestActor()
    {
        _x = 90;//getStage()->getWidth()/2.0f;
        _y = 80;

		if (!movieRes.movieData)
			return;

		int num = ae_get_movie_composition_data_count(movieRes.movieData);
		for (int i = 0; i < num; ++i)
		{
			const aeMovieCompositionData *comp = ae_get_movie_composition_data_by_index(movieRes.movieData, i);
            bool main = ae_is_movie_composition_data_master(comp);
            _color = main ? Color(Color::White) : Color(Color::Gray);
            _txtColor = main ? Color(Color::Blue) : Color(Color::Gray);
            const char *name = ae_get_movie_composition_data_name(comp);
			addButton(name, name);
		}

		if (!aeCurrent.empty())
		{
			getStage()->addTween(TweenDummy(), 1)->addDoneCallback([=](Event*) {
				clicked(aeCurrent);
			});			
		}
    }

    void showTest(spActor actor)
    {
        spStage stage = getStage();
#if MULTIWINDOW
        stage = stage2;
#else
        setVisible(false);
#endif
        stage->addChild(actor);
    }


    void clicked(string id)
    {
		Preview::instance = new Preview(id);
		setVisible(false);
		Preview::instance->attachTo(getStage());
    }
};

void example_preinit()
{
}




oxygine::file::ZipFileSystem zfs;

void example_init()
{
    Test::init();
	key::init();

    const char* AE_HASH = "";
    AEMovieResource::initLibrary(AE_HASH);


    bool isZip = path::extractFileExt(aeProject) == "zip";

    if (isZip)
    {
        if (file::exists(aeProject))
            zfs.add(aeProject.c_str());
        file::fs().mount(&zfs);
        movieRes.load("project", ep_show_warning);
    }
    else
    {
        movieRes.load(aeProject, ep_show_warning);
    }
	


    Test::instance = new TestActor;
    getStage()->addChild(Test::instance);

	//Preview::instance = new Preview;

    //Initialize http requests
    HttpRequestTask::init();

#if MULTIWINDOW
    SDL_Window* window2 = SDL_CreateWindow("Second Oxygine Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, getStage()->getWidth(), getStage()->getHeight(), SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    stage2 = new Stage(false);
    stage2->setSize(getStage()->getSize());
    stage2->associateWithWindow(window2);
#endif
}

void example_update()
{
    AEMovieResource::updateLibrary();
#if MULTIWINDOW
    stage2->update();
    SDL_Window* wnd = stage2->getAssociatedWindow();
    if (core::beginRendering(wnd))
    {
        Color clearColor(32, 32, 32, 255);
        Rect viewport(Point(0, 0), core::getDisplaySize());
        //render all actors. Actor::render would be called also for all children
        stage2->render(clearColor, viewport);

        core::swapDisplayBuffers(wnd);
    }
#endif
}

void example_destroy()
{
	key::release();
	if (Preview::instance)
		Preview::instance->detach();
	Preview::instance = 0;
	movieRes.clear();
    Test::free();
    HttpRequestTask::release();
    AEMovieResource::freeLibrary();
}