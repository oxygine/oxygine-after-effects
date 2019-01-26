#include "AEMovieResource.h"
#include "ox/stringUtils.hpp"
#include <atomic>
#include "ox/ResAnim.hpp"
#include "ox/STDRenderer.hpp"
//#include "utils/DebugTimer.h"
#include "ox/file.hpp"
#include "ox/ResFont.hpp"
#include "ox/Font.hpp"

//#define AE_DEBUG_SHOW_SLOTS


//#define WORK_AREA_API 1
using namespace oxygine;

std::atomic<uint32_t> _aeTotalMem;

const aeMovieInstance* AEMovieResource::instance = 0;

#if 0
#define mmalloc(size) (Winnie::Alloc(size))
#define mfree(p) (Winnie::Free(p))
#else
#define mmalloc malloc
#define mfree free
#endif



//////////////////////////////////////////////////////////////////////////
static void* stdlib_movie_alloc(void* _data, size_t _size)
{
    uint32_t size = (uint32_t)_size;
    (void)_data;
    uint32_t* ptr = (uint32_t*)mmalloc(_size + 4);
    *ptr = size;
    _aeTotalMem += size;
    return ptr + 1;
}
//////////////////////////////////////////////////////////////////////////
static void* stdlib_movie_alloc_n(void* _data, size_t _size, size_t _count)
{
    (void)_data;

    size_t total = _size * _count;

    return stdlib_movie_alloc(_data, total);
}
//////////////////////////////////////////////////////////////////////////
static void stdlib_movie_free(void* _data, const void* _ptr)
{
    if (!_ptr)
        return;

    (void)_data;
    uint32_t* p = (uint32_t*)((char*)_ptr - 4);
    _aeTotalMem -= *p;
    mfree(p);
}
//////////////////////////////////////////////////////////////////////////
static void stdlib_movie_free_n(void* _data, const void* _ptr)
{
    (void)_data;
    stdlib_movie_free(_data, _ptr);
}

int32_t ae_movie_strncmp(void* _data, const char* _src, const char* _dst, size_t _count)
{
    return strcmp_cns(_src, _dst);
}



void  ae_movie_logerror(void* _data, aeMovieErrorCode _code, const ae_char_t* _message, ...)
{
#ifndef BUILD_PROD
    //logs::error("ae error: %s:%s %s", _compositionName, _layerName, _message);

    va_list argList;

    va_start(argList, _message);
    logs::message_va(_message, argList);// logs::messageln();
    va_end(argList);
#endif
}


ResAnim _missingResAnim;

void AEMovieResource::initLibrary(const char *AE_HASH)
{
    logs::messageln("AEMovieResource::initLibrary");
    _aeTotalMem = 0;
    instance = ae_create_movie_instance(AE_HASH, &stdlib_movie_alloc, &stdlib_movie_alloc_n, &stdlib_movie_free, &stdlib_movie_free_n, ae_movie_strncmp, ae_movie_logerror, 0);
    

    _missingResAnim.init(STDRenderer::white, Point(32, 32));
}

void AEMovieResource::freeLibrary()
{
    _missingResAnim.removeFrames();
    if (instance)
        ae_delete_movie_instance(instance);
    instance = 0;
    uint32_t mem = _aeTotalMem;
    logs::messageln("AEMovieResource::freeLibrary mem: %d", mem);
}

void AEMovieResource::updateLibrary()
{
//    DebugTimer::add("ae-mem", _aeTotalMem / 1024);
}

//std::map<std::std::string, ResAnim*> anims;
//static std::map<std::std::string, ResSound*> _sounds;


std::tuple<const aeMovieCompositionData*, const AEMovieResource*> AEMovieResource::getComposition(const std::string& comp)const
{
    if (movieData)
    {
        const aeMovieCompositionData* compositionData = ae_get_movie_composition_data(movieData, comp.c_str());
        if (compositionData)
            return std::make_pair(compositionData, this);
    }
    if (fallback)
        return fallback->getComposition(comp);
    return std::make_pair((const aeMovieCompositionData*)0, this);
}

const oxygine::ResAnim* AEMovieResource::getResAnim(const std::string& id, error_policy ep /*= ep_show_error*/) const
{
    ResAnim* res = _res.getResAnim(id, ep_ignore_error);
    if (res)
        return res;
    if (fallback)
        return fallback->getResAnim(id, ep);
    handleErrorPolicy(ep, "can't find resanim %s", id.c_str());
    return 0;
}

void AEMovieResource::resource_provider_deleter(aeMovieResourceTypeEnum _type, ae_voidptr_t _data, ae_voidptr_t _ud)
{

}

//typedef ae_bool_t(*ae_movie_data_resource_provider_t)(const aeMovieResource * _resource, ae_voidptrptr_t _rd, ae_voidptr_t _ud);
ae_bool_t AEMovieResource::resource_provider(const aeMovieResource * _resource, ae_voidptrptr_t _rd, ae_voidptr_t _data)
{
    AEMovieResource* me = (AEMovieResource*)_data;

    uint8_t resource_type = _resource->type;

    switch (resource_type)
    {
    case AE_MOVIE_RESOURCE_IMAGE:
    {
        const aeMovieResourceImage* resource = (const aeMovieResourceImage*)_resource;
        std::string id = path::extractBaseFileName(path::extractFileName(resource->path));
        ResAnim* rs = me->_res.getResAnim(id, ep_show_warning);
        if (!rs)
            rs = &_missingResAnim;

        *_rd = rs;
    } break;

    case AE_MOVIE_RESOURCE_VIDEO:
    {
        const aeMovieResourceVideo* resource = (const aeMovieResourceVideo*)_resource;
    } break;

    case AE_MOVIE_RESOURCE_SOUND:
    {
        /*
        const aeMovieResourceSound * resource = (const aeMovieResourceSound *)_resource;
        std::string str = me->_folder + "sounds/" + std::string(resource->path);
        ResSound *snd = ResSound::create(str, false);
        _sounds[resource->path] = snd;
        return snd;
        */
    } break;

    case AE_MOVIE_RESOURCE_PARTICLE:
    {
        //#ifdef EMSCRIPTEN
#if 1
        return 0;
#else
        const aeMovieResourceParticle* resource = (const aeMovieResourceParticle*)_resource;

        std::string str = me->_folder + std::string(resource->path);
        HM_FILE file = MP_Manager::GetInstance().LoadEmittersFromFile(str.c_str());

        if (isMainThread())
            MP_Manager::GetInstance().RefreshAtlas(path::extractFolder(str));
        else
            core::getMainThreadDispatcher().postCallback([=]()
        {
            MP_Manager::GetInstance().RefreshAtlas(path::extractFolder(str));
        });

        return (void*)file;
#endif
    } break;

    }


    return true;
}



AEMovieResource::AEMovieResource()
{

}

AEMovieResource::~AEMovieResource()
{

}

void AEMovieResource::clear()
{
    if (_weak)
        _weak->deleted = true;
    _weak = 0;

    _res.free();
    if (movieData)
        ae_delete_movie_data(movieData);
    movieData = 0;
    _folder = "";
}


struct fbpos
{
    file::buffer data;
    int pos = 0;
};

static size_t read_file2(void* _data, void* _buff, ae_size_t _carriage, size_t _size)
{
    fbpos* data = (fbpos*)_data;
    _size = std::min(_size, data->data.size() - data->pos);
    memcpy(_buff, &data->data.data[0] + data->pos, _size);
    data->pos += _size;
    return _size;
}

static void memory_copy(void*  _data, const void*  src, void * dst, size_t _size)
{
    (void)_data;

    memcpy(dst, src, _size);
}



ResAnim* getGlyph(Resources& res, const std::string& prefix, char s)
{
    char name[32];
    char gl[2];
    gl[0] = s;
    gl[1] = 0;
    safe_sprintf(name, "%s.%s", prefix.c_str(), gl);
    ResAnim* rs = res.getResAnim(name);

    return rs;
}

class ResFontFixed : public ResFont
{
public:
    ResFontFixed(int size) { _size = size; }
    const Font* getFont(const char* name = 0, int size = 0) const override
    {
        return &font;
    }

    void _load(LoadResourcesContext* context /* = 0 */) override
    {

    }
    void _unload() override
    {

    }
    Font font;

protected:

};


bool splitStrSepAE(const std::string& str, char sep, std::string& partA, std::string& partB, error_policy ep)
{
    std::string copy = str;
    partA = str;

    size_t pos = str.find_first_of(sep);
    if (pos == str.npos)
    {
        handleErrorPolicy(ep, "can't split std::string: %s", str.c_str());
        return false;
    }

    partA = copy.substr(0, pos);
    partB = copy.substr(pos + 1, str.size() - pos - 1);
    return true;
}

bool AEMovieResource::load(const std::string& movie, error_policy ep, bool loadAtlasses)
{
    clear();



    _folder = movie + "/";

    std::string fq = _folder + path::extractFileName(movie) + ".aem";

    if (!file::exists(fq))
    {
        handleErrorPolicy(ep, "can't load AE project %s", fq.c_str());
        return false;
    }

    _weak = new AEMovieResourceWeak();

    _res.loadXML(_folder + "res.xml", ResourcesLoadOptions().dontLoadAll().shortenIDS().prebuiltFolder(movie).prebuiltImagesFolder(_folder));
    if (loadAtlasses)
        _res.load();

    //_shaderFlags = 0;// _res._getResourcesMap().begin;


    auto rs = _res._getResourcesMap();
    for (auto item : rs)
    {
        int q = 0;
        const std::string& name = item.first;
        if (name[0] == '#')
        {
            std::string fontName, charName;
            splitStrSepAE(name, '.', fontName, charName, ep_show_error);
            auto it = _res._getResourcesMap().find(fontName);

            ResFontFixed* ff = 0;
            if (it == _res._getResourcesMap().end())
            {
                ResAnim* m = getGlyph(_res, fontName, '0');
                int size = (int)m->getHeight();

                ff = new ResFontFixed(size);

                ff->font.init(fontName.c_str(), size, size, size);
                ff->setName(fontName);
                //ff->
                _res._getResourcesMap()[fontName] = ff;
            }
            else
            {
                ff = safeCast<ResFontFixed*>(_res._getResourcesMap()[fontName].get());
            }


            char s = charName[0];
            ResAnim* rs = getGlyph(_res, fontName, s);
            const AnimationFrame& frame = rs->getFrame(0);
            glyph g;
            g.ch = s;
            g.opt = 0;

            g.advance_x = (int)frame.getWidth();
            g.advance_y = 0;
            g.offset_x = 0;
            g.offset_y = -(int)frame.getHeight();
            g.sw = (int)frame.getWidth();
            g.sh = (int)frame.getHeight();
            g.texture = frame.getDiffuse().base;
            g.src = frame.getSrcRect();

            g.offset_x = 0;
            ff->font.addGlyph(g);
        }
    }

    //clr = tf;



    aeMovieDataProviders prov;
    ae_clear_movie_data_providers(&prov);

    prov.resource_provider = resource_provider;
    prov.resource_deleter = resource_provider_deleter;

    //movieData = ae_create_movie_data(instance, &resource_provider, resource_provider_deleter, this);
    movieData = ae_create_movie_data(instance, &prov, this);


    fbpos aeData;
    file::read(fq.c_str(), aeData.data);


    //aeMovieStream* stream = ae_create_movie_stream(instance, read_file2, memory_copy, &aeData);
    aeMovieStream* stream = ae_create_movie_stream_memory(instance, &aeData.data.front(), memory_copy, 0);
    OX_ASSERT(stream);
    if (!stream)
        return false;

    ae_uint32_t major_version;
    ae_uint32_t minor_version;
    ae_result_t res = ae_load_movie_data(movieData, stream, &major_version, &minor_version);

    //ae_result_t res = ae_load_movie_data(movieData, stream, &ver);

    ae_delete_movie_stream(stream);
    OX_ASSERT(res == AE_RESULT_SUCCESSFUL);

    return res == AE_RESULT_SUCCESSFUL;
}

bool AEMovieResource::hasComposition(const std::string& name)
{
    auto p = getComposition(name);
    return std::get<0>(p) != 0;
}
