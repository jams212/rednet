#include "uenv.h"
#include <assert.h>
#include <string.h>

using namespace rednet;

uenv::uenv()
{
    l__ = luaL_newstate();
}

uenv::~uenv()
{
    lua_close(l__);
    l__ = NULL;
}

const char *uenv::getEnv(const char *key)
{
    lk__.lock();

    lua_getglobal(l__, key);
    const char *result = lua_tostring(l__, -1);
    lua_pop(l__, 1);

    lk__.unlock();
    return result;
}

void uenv::setEnv(const char *key, const char *value)
{
    lk__.lock();
    lua_getglobal(l__, key);
    assert(lua_isnil(l__, -1));
    lua_pop(l__, 1);
    lua_pushstring(l__, value);
    lua_setglobal(l__, key);
    lk__.unlock();
}

int uenv::__getInt(const char *key, int opt)
{
    const char *str = INST(uenv, getEnv, key);
    if (str == NULL)
    {
        char tmp[20];
        sprintf(tmp, "%d", opt);
        INST(uenv, setEnv, key, tmp);
        return opt;
    }
    return strtol(str, NULL, 10);
}

bool uenv::__getBoolean(const char *key, bool opt)
{
    const char *str = INST(uenv, getEnv, key);
    if (str == NULL)
    {
        INST(uenv, setEnv, key, opt ? "true" : "false");
        return opt;
    }
    return strcmp(str, "true") == 0;
}

const char *uenv::__getString(const char *key, const char *opt)
{
    const char *str = INST(uenv, getEnv, key);
    if (str == NULL)
    {
        if (opt)
        {
            INST(uenv, setEnv, key, opt);
            opt = INST(uenv, getEnv, key);
        }
        return opt;
    }
    return str;
}
