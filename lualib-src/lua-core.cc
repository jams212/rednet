
#include <lua.hpp>
#include <uextern.h>
#include <inttypes.h>

#define CORE_MAX_LEVEL 3

using namespace rednet;

class luaCoreApi
{
    struct sourceInfo
    {
        const char *source;
        int line;
    };

    enum commandCode
    {
        CC_TIMEOUT = 1,
        CC_NAME = 2,
        CC_QUERY = 3,
        CC_EXIT = 4,
        CC_KILL = 5,
        CC_LAUNCH = 6,
        CC_GETENV = 7,
        CC_SETENT = 8,
        CC_STARTTIME = 9,
        CC_ABORT = 10,
        CC_SIGNAL = 11,
    };
private:
    static int __sendEvent(lua_State *l, int source, int idx_type);
    static const char *__getDestString(lua_State *l, int index);
    static const char *__commandCall(ucontext *ctx, int cmd, const char *param);
public:
    static int 
    lsend(lua_State *l)
    {
        return __sendEvent(l, 0, 2);
    }

    static int 
    lself(lua_State *l)
    {
        ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
        const char *result = ctx->self();
        if (result && result[0] == ':')
        {
            int i;
            uint32_t addr = 0;
            for (i = 1; result[i]; i++)
            {
                int c = result[i];
                if (c >= '0' && c <= '9')
                {
                    c = c - '0';
                }
                else if (c >= 'a' && c <= 'f')
                {
                    c = c - 'a' + 10;
                }
                else if (c >= 'A' && c <= 'F')
                {
                    c = c - 'A' + 10;
                }
                else
                {
                    return 0;
                }
                addr = addr * 16 + c;
            }
            lua_pushinteger(l, addr);
            return 1;
        }
        return 0;
    }

    static int 
    lerror(lua_State *l)
    {
        ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
        int n = lua_gettop(l);
        if (n <= 1)
        {
            lua_settop(l, 1);
            const char *s = luaL_tolstring(l, 1, NULL);
            ucall::error(ctx, "%s", s);
            return 0;
        }
        luaL_Buffer b;
        luaL_buffinit(l, &b);
        int i;
        for (i = 1; i <= n; i++)
        {
            luaL_tolstring(l, i, NULL);
            luaL_addvalue(&b);
            if (i < n)
            {
                luaL_addchar(&b, ' ');
            }
        }
        luaL_pushresult(&b);
        ucall::error(ctx, "%s", lua_tostring(l, -1));
        return 0;
    }

    static int 
    ltrace(lua_State *l)
    {
        ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
        const char *tag = luaL_checkstring(l, 1);
        const char *user = luaL_checkstring(l, 2);
        if (!lua_isnoneornil(l, 3))
        {
            lua_State *co = l;
            int level;
            if (lua_isthread(l, 3))
            {
                co = lua_tothread(l, 3);
                level = luaL_optinteger(l, 4, 1);
            }
            else
            {
                level = luaL_optinteger(l, 3, 1);
            }
            struct sourceInfo si[CORE_MAX_LEVEL];
            lua_Debug d;
            int index = 0;
            do
            {
                if (!lua_getstack(co, level, &d))
                    break;
                lua_getinfo(co, "Sl", &d);
                level++;
                si[index].source = d.source;
                si[index].line = d.currentline;
                if (d.currentline >= 0)
                    ++index;
            } while (index < CORE_MAX_LEVEL);

            switch (index)
            {
            case 1:
                ucall::error(ctx, "<TRACE %s> %" PRId64 " %s : %s:%d", tag, 0, user, si[0].source, si[0].line);
                break;
            case 2:
                ucall::error(ctx, "<TRACE %s> %" PRId64 " %s : %s:%d %s:%d",
                              tag, timestamp::getTimeSec(), user,
                              si[0].source, si[0].line,
                              si[1].source, si[1].line);
                break;
            case 3:
                ucall::error(ctx, "<TRACE %s> %" PRId64 " %s : %s:%d %s:%d %s:%d",
                              tag, timestamp::getTimeSec(), user,
                              si[0].source, si[0].line,
                              si[1].source, si[1].line,
                              si[2].source, si[2].line);
                break;
            default:
                ucall::error(ctx, "<TRACE %s> %" PRId64 " %s", tag, timestamp::getTimeSec(), user);
                break;
            }
            return 0;
        }
        ucall::error(ctx, "<TRACE %s> %" PRId64 " %s", tag, timestamp::getTimeSec(), user);
        return 0;
    }

    static int 
    ltrash(lua_State *l)
    {
        int t = lua_type(l, 1);
        switch (t)
        {
        case LUA_TSTRING:
        {
            break;
        }
        case LUA_TLIGHTUSERDATA:
        {
            void *msg = lua_touserdata(l, 1);
            luaL_checkinteger(l, 2);
            umemory::free(msg);
            break;
        }
        default:
            luaL_error(l, "rednet.trash invalid param %s", lua_typename(l, t));
        }

        return 0;
    }

    static int 
    ltostring(lua_State *l)
    {
        if (lua_isnoneornil(l, 1))
        {
            return 0;
        }
        char *msg = (char *)lua_touserdata(l, 1);
        int sz = luaL_checkinteger(l, 2);
        lua_pushlstring(l, msg, sz);
        return 1;
    }

    static int 
    lnow(lua_State *l)
    {
        uint64_t ti = ucall::now();
        lua_pushinteger(l, ti);
        return 1;
    }

    static int
    lcommand(lua_State *l)
    {
        ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
        const int cmd = luaL_checkinteger(l, 1);
        const char *result;
        const char *parm = NULL;
        if (lua_gettop(l) == 2)
        {
            parm = luaL_checkstring(l, 2);
        }

        result = __commandCall(ctx, cmd, parm);

        if (result)
        {
            lua_pushstring(l, result);
            return 1;
        }

        return 0;
    }

    static int
    lintcommand(lua_State *l)
    {
        ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
        const int cmd = luaL_checkinteger(l, 1);
        const char *result;
        const char *parm = NULL;
        char tmp[64]; // for integer parm
        if (lua_gettop(l) == 2)
        {
            if (lua_isnumber(l, 2))
            {
                int32_t n = (int32_t)luaL_checkinteger(l, 2);
                sprintf(tmp, "%d", n);
                parm = tmp;
            }
            else
            {
                parm = luaL_checkstring(l, 2);
            }
        }

        result = __commandCall(ctx, cmd, parm);
        if (result)
        {
            char *endptr = NULL;
            lua_Integer r = strtoll(result, &endptr, 0);
            if (endptr == NULL || *endptr != '\0')
            {
                // may be real number
                double n = strtod(result, &endptr);
                if (endptr == NULL || *endptr != '\0')
                {
                    return luaL_error(l, "Invalid result %s", result);
                }
                else
                {
                    lua_pushnumber(l, n);
                }
            }
            else
            {
                lua_pushinteger(l, r);
            }
            return 1;
        }
        return 0;
    }
};

int 
luaCoreApi::__sendEvent(lua_State *l, int source, int idx_type)
{
    ucontext *ctx = (ucontext *)lua_touserdata(l, lua_upvalueindex(1));
    uint32_t dest = (uint32_t)lua_tointeger(l, 1);
    const char *dest_string = NULL;
    if (dest == 0)
    {
        if (lua_type(l, 1) == LUA_TNUMBER)
        {
            return luaL_error(l, "Invalid service address 0");
        }
        dest_string = __getDestString(l, 1);
    }

    int type = luaL_checkinteger(l, idx_type + 0);
    int session = 0;
    ucontext *session_ctx = NULL;
    if (lua_isnil(l, idx_type + 1))
    {
        session_ctx = ctx;
    }
    else
    {
        session = luaL_checkinteger(l, idx_type + 1);
    }

    int mtype = lua_type(l, idx_type + 2);

    switch (mtype)
    {
    case LUA_TSTRING:
    {
        size_t len = 0;
        void *msg = (void *)lua_tolstring(l, idx_type + 2, &len);
        if (len == 0)
        {
            msg = NULL;
        }
        if (dest_string)
        {
            session = ucall::sendString(source == 0 ? ctx->id : source, dest_string, type, session, (const char*)msg, session_ctx);
        }
        else
        {
            session = ucall::sendString(source == 0 ? ctx->id : source, dest, type, session, (const char *)msg, session_ctx);
        }
        break;
    }
    case LUA_TLIGHTUSERDATA:
    {
        void *msg = lua_touserdata(l, idx_type + 2);
        int size = luaL_checkinteger(l, idx_type + 3);
        if (dest_string)
        {
            session = ucall::sendEvent(source == 0 ? ctx->id : source, dest_string, type, session, msg, size, session_ctx);
        }
        else
        {
            session = ucall::sendEvent(source == 0 ? ctx->id : source, dest, type, session, msg, size, session_ctx);
        }
        break;
    }
    default:
        luaL_error(l, "invalid param %s", lua_typename(l, lua_type(l, idx_type + 2)));
    }

    if (session < 0)
    {
        return 0;
    }

    lua_pushinteger(l, session);
    return 1;
}

const char *
luaCoreApi::__getDestString(lua_State *l, int index)
{
    const char *dest_string = lua_tostring(l, index);
    if (dest_string == NULL)
    {
        luaL_error(l, "dest address type (%s) must be a string or number.", lua_typename(l, lua_type(l, index)));
    }
    return dest_string;
}

const char * 
luaCoreApi::__commandCall(ucontext *ctx, int cmd, const char *param)
{
    const char *result;
    switch (cmd)
    {
    case CC_TIMEOUT:
        result = ucall::luaTimeOut(ctx, param);
        break;
    case CC_NAME:
        result = ucall::luaName(ctx, param);
        break;
    case CC_QUERY:
        result = ucall::luaQuery(ctx, param);
        break;
    case CC_EXIT:
        result = ucall::luaExit(ctx, param);
        break;
    case CC_KILL:
        result = ucall::luaKill(ctx, param);
        break;
    case CC_LAUNCH:
        result = ucall::luaLaunch(ctx, param);
        break;
    case CC_GETENV:
        result = ucall::luaGetEnv(ctx, param);
        break;
    case CC_SETENT:
        result = ucall::luaSetEnv(ctx, param);
        break;
    case CC_STARTTIME:
        result = ucall::luaStartTime(ctx, param);
        break;
    case CC_ABORT:
        result = ucall::luaAbort(ctx, param);
        break;
    case CC_SIGNAL:
        result = ucall::luaSignal(ctx, param);
        break;
    default:
        result = NULL;
        break;
    }
    return result;
}

extern "C" int 
luaopen_rednet_core(lua_State *L)
{
    luaL_checkversion(L);
    luaL_Reg l[] = {
        {"send", luaCoreApi::lsend},
        {"self", luaCoreApi::lself},
        {"error", luaCoreApi::lerror},
        {"trace", luaCoreApi::ltrace},
        {"command", luaCoreApi::lcommand},
        {"intcommand", luaCoreApi::lintcommand},
        {NULL, NULL},
    };

     luaL_Reg l2[] = {
        {"tostring", luaCoreApi::ltostring},
        {"trash", luaCoreApi::ltrash},
        {"now", luaCoreApi::lnow},
        {NULL, NULL},
    };

    lua_createtable(L, 0, sizeof(l) / sizeof(l[0]) + sizeof(l2) / sizeof(l2[0]) - 2);
    lua_getfield(L, LUA_REGISTRYINDEX, "UContext");
    ucontext *ctx = (rednet::ucontext*)lua_touserdata(L, -1);
    if (ctx == NULL)
    {
        return luaL_error(L, "Init UContext first");
    }

    luaL_setfuncs(L, l, 1);

    luaL_setfuncs(L, l2, 0);

    return 1;
}