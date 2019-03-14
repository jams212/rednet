#include "umodule.h"
#include "umemory.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace rednet;

bool umodule::open(const char *name)
{
    create_cb__ = (ucomponent_dl_create)get_api(name, "_create");
    release_cb__ = (ucomponent_dl_release)get_api(name, "_release");
    return create_cb__ != NULL;
}

void *umodule::get_api(const char *name, const char *api_name)
{
    size_t name_size = strlen(name);
    size_t api_size = strlen(api_name);
    char tmp[name_size + api_size + 1];
    memcpy(tmp, name, name_size);
    memcpy(tmp + name_size, api_name, api_size + 1);
    char *ptr = strrchr(tmp, '.');
    if (ptr == NULL)
    {
        ptr = tmp;
    }
    else
    {
        ptr = ptr + 1;
    }
    return dlsym(lib__, ptr);
}

umodMgr::umodMgr() : modp__(NULL)
{
}

umodMgr::~umodMgr()
{
    local_clear();
    if (modp__)
    {
        umemory::free((void *)modp__);
        modp__ = NULL;
    }
}

void umodMgr::init(const char *cpath)
{
    if (!cpath)
    {
        fprintf(stderr, "Need setting module path\n");
        exit(1);
    }
    modp__ = umemory::strdup(cpath);
}

umodule *umodMgr::query(const char *name)
{
    umodule *result = local_query(name);
    if (result)
        return result;
    result = local_register(name);
    return result;
}

umodule *umodMgr::local_query(const char *name)
{
    rlocking rl(&modk__);
    if (mods__.empty())
        return NULL;
    unmaps::iterator it = mods__.find(name);
    if (it == mods__.end())
        return NULL;
    return it->second;
}

umodule *umodMgr::local_register(const char *name)
{
    wlocking wl(&modk__);
    if (!mods__.empty())
    {
        unmaps::iterator pt = mods__.find(name);
        if (pt != mods__.end())
            return pt->second;
    }
    umodule *result = NULL;
    void *dl = local_opendl(name);
    if (dl)
    {
        result = new umodule(dl);
        assert(result);
        if (!result->open(name))
        {
            delete result;
            result = NULL;
        }
        else
        {
            const char *tmpname = umemory::strdup(name);
            assert(tmpname);
            mods__.insert(std::pair<const char *, umodule *>(tmpname, result));
        }
    }

    return result;
}

void umodMgr::local_clear()
{
    wlocking wl(&modk__);
    if (mods__.empty())
        return;
    unmaps::iterator pt = mods__.begin();
    while (pt != mods__.end())
    {
        const char *tname = pt->first;
        umodule *tmodule = pt->second;
        mods__.erase(pt++);

        umemory::free((void *)tname);
        delete tmodule;
    }
}

void *umodMgr::local_opendl(const char *name)
{
    const char *l;
    const char *path = modp__;
    size_t path_size = strlen(path);
    size_t name_size = strlen(name);

    int sz = path_size + name_size;
    //search path
    void *dl = NULL;
    char tmp[sz];
    do
    {
        memset(tmp, 0, sz);
        while (*path == ';')
            path++;
        if (*path == '\0')
            break;
        l = strchr(path, ';');
        if (l == NULL)
            l = path + strlen(path);
        int len = l - path;
        int i;
        for (i = 0; path[i] != '?' && i < len; i++)
        {
            tmp[i] = path[i];
        }
        memcpy(tmp + i, name, name_size);
        if (path[i] == '?')
        {
            strncpy(tmp + i + name_size, path + i + 1, len - i - 1);
        }
        else
        {
            fprintf(stderr, "Invalid CXX service path\n");
            exit(1);
        }

        dl = dlopen(tmp, RTLD_NOW | RTLD_GLOBAL);
        path = l;
    } while (dl == NULL);

    if (dl == NULL)
    {
        fprintf(stderr, "try open %s failed : %s\n", name, dlerror());
    }

    return dl;
}
