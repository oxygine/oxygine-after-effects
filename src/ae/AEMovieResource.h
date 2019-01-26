#pragma once
#include "AEConf.h"
#include "movie/movie.hpp"
#include "ox/Actor.hpp"
#include "ox/Resources.hpp"
using namespace oxygine;


extern "C"
{
    struct aeMovieSubComposition;
}



DECLARE_SMART(AEMovieResource, spAEMovieResource);

DECLARE_SMART(AEMovieResourceWeak, spAEMovieResourceWeak);
class AEMovieResourceWeak : public Object
{
public:
    bool deleted = false;
};

class AEMovieResource : public Object
{
public:
    static const aeMovieInstance* instance;
    static void initLibrary(const char *AE_HASH);
    static void freeLibrary();
    static void updateLibrary();

    AEMovieResource();
    ~AEMovieResource();

    void clear();
    bool load(const std::string& path, error_policy ep = ep_show_error, bool loadAtlasses = true);
    bool hasComposition(const std::string& name);

    void setFallback(AEMovieResource* f) { fallback = f; }
    bool isEmpty() const { return movieData == 0; }

    const Resources&    getResources() const { return _res; }
    const std::string&  getFolder() const { return _folder; }
    const aeMovieData*  getMovieData() const { return movieData; }
    const ResAnim*      getResAnim(const std::string& id, error_policy ep = ep_show_error) const;

    
    std::tuple<const aeMovieCompositionData*, const AEMovieResource*> getComposition(const std::string& id) const;

    aeMovieData* movieData = 0;

protected:
    friend class AEMovie;
    friend class AEMovieWork;

    static ae_bool_t resource_provider(const aeMovieResource * _resource, ae_voidptrptr_t _rd, ae_voidptr_t _ud);
    static void resource_provider_deleter(aeMovieResourceTypeEnum _type, ae_voidptr_t _data, ae_voidptr_t _ud);


    spAEMovieResourceWeak _weak;

    Resources _res;

    std::string _folder;
    AEMovieResource* fallback = 0;
};


class AEMovieResourcesGroup
{
public:
    AEMovieResource * current;
    AEMovieResourcesGroup* fallback;
};


#define AE_SHOULD_NOT_BE_DELETED() if (_weak->deleted)\
{\
    logs::error("ae isntance already deleted %s", getName().c_str());\
    OX_ASSERT(!"AE_SHOULD_NOT_BE_DELETED");\
    return;\
}
