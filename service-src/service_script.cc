#include "uextern.h"
#include <functional>
#include <lua.hpp>

using namespace rednet;
using namespace std;

#define MEMORY_WARNING_REPORT 1024 * 1024 * 32

void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize);

int traceback(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    if (msg)
        luaL_traceback(L, L, msg, 1);
    else
        lua_pushliteral(L, "(no error message)");
    return 1;
}

void report_launcher_error(ucontext *ctx)
{
    ucall::sendString(ctx->id, ".launch", EP_TEXT, 0, (const char *)"ERROR");
}

class script : public ucomponent
{
    friend void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize);

  public:
    script() : mem__(0),
               mem_report__(MEMORY_WARNING_REPORT),
               mem_limit__(0)
    {
        l__ = lua_newstate(lalloc, this);
    }

    ~script()
    {
        lua_close(l__);
    }

    int init(ucontext *ctx, const char *parm) override
    {
        parent__ = ctx;
        int sz = strlen(parm);
        char *tmp = (char *)umemory::malloc(sz);
        memcpy(tmp, parm, sz);
        uint32_t ctxid = ctx->id;
        callback__ = EVBIND(&script::launch, this);
        ud__ = NULL;
        ucall::sendEvent(ctxid, ctxid, EP_DONT, 0, tmp, sz);
        return 0;
    }

  private:
    int launch(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        assert(type == EP_DONT);
        callback__ = nullptr;
        ud__ = NULL;
        int err = loadlua((const char *)data, sz);
        if (err)
        {
            ucall::exit(parent__, parent__->id);
        }

        callback__ = EVBIND(&script::start, this);

        ucall::sendEvent(parent__->id, parent__->id, EP_DONT, 0, NULL, 0);
        return 0;
    }

    int start(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        lua_State *L = l__;
        lua_getglobal(L, "start");

        int r = lua_pcall(L, 0, 0, 1);
        if (r != LUA_OK)
        {
            ucall::error(parent__, "lua script start fail :%s", lua_tostring(L, -1));
            report_launcher_error(parent__);
            return 0;
        }

        lua_pop(L, 1);
        return 0;
    }

  private:
    int loadlua(const char *args, size_t sz)
    {
        lua_State *L = l__;
        lua_gc(L, LUA_GCSTOP, 0);
        lua_pushboolean(L, 1);
        lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
        luaL_openlibs(L);
        lua_pushlightuserdata(L, parent__);
        lua_setfield(L, LUA_REGISTRYINDEX, "UContext");

        assert(INST(uframework, getDeploy)->luaService);

        lua_pushstring(L, INST(uframework, getDeploy)->luaPath);
        lua_setglobal(L, "LUA_PATH");

        lua_pushstring(L, INST(uframework, getDeploy)->luaCPath);
        lua_setglobal(L, "LUA_CPATH");

        lua_pushstring(L, INST(uframework, getDeploy)->luaService);
        lua_setglobal(L, "LUA_SERVICE");

        lua_pushcfunction(L, traceback);
        assert(lua_gettop(L) == 1);

        char loader[64] = "./lualib/loader.lua";
        int r = luaL_loadfile(L, loader);
        if (r != LUA_OK)
        {
            ucall::error(parent__, "Can't load %s : %s", loader, lua_tostring(L, -1));
            report_launcher_error(parent__);
            return 1;
        }

        lua_pushlstring(L, args, sz);
        r = lua_pcall(L, 1, 0, 1);
        if (r != LUA_OK)
        {
            ucall::error(parent__, "lua loader error :%s", lua_tostring(L, -1));
            report_launcher_error(parent__);
            return 1;
        }

        lua_settop(L, 0);
        if (lua_getfield(L, LUA_REGISTRYINDEX, "memlimit") == LUA_TNUMBER)
        {
            size_t limit = lua_tointeger(L, -1);
            mem_limit__ = limit;
            ucall::error(parent__, "Set memory limit to %.2f M", (float)limit / (1024 * 1024));
            lua_pushnil(L);
            lua_setfield(L, LUA_REGISTRYINDEX, "memlimit");
        }

        lua_pop(L, 1);
        lua_gc(L, LUA_GCRESTART, 0);

        return 0;
    }

  private:
    lua_State *l__;
    size_t mem__;
    size_t mem_report__;
    size_t mem_limit__;
};

void *lalloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    script *l = (script *)ud;
    size_t mem = l->mem__;
    l->mem__ += nsize;
    if (ptr)
        l->mem__ -= osize;
    if (l->mem_limit__ != 0 && l->mem__ > l->mem_limit__)
    {
        if (ptr == NULL || nsize > osize)
        {
            l->mem__ = mem;
            return NULL;
        }
    }

    if (l->mem__ > l->mem_report__)
    {
        l->mem_report__ *= 2;
        ucall::error(l->parent__, "Memory warning %.2f M", (float)l->mem__ / (1024 * 1024));
    }

    return umemory::lalloc(ptr, osize, nsize);
}

extern "C" script *script_create(void)
{
    script *inst = new script();
    return inst;
}

extern "C" void script_release(script *inst)
{
    delete inst;
    inst = NULL;
}