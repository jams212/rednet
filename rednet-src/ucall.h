#ifndef REDNET_UCALL_H
#define REDNET_UCALL_H

#include "uevent.h"
#include "umemory.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

namespace rednet
{

class ucontext;
class ucall
{
  public:
    static void error(ucontext *ctx, const char *msg, ...);

    static uint64_t now();

    static inline void toAddr(char *str, uint32_t id)
    {
        int i;
        static char hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
        str[0] = ':';
        for (i = 0; i < 8; i++)
        {
            str[i + 1] = hex[(id >> ((7 - i) * 4)) & 0xf];
        }
        str[9] = '\0';
    }

    static uint32_t addrTo(const char *addr);

    static bool sendEvent(uint32_t id, uevent *e);

    static int sendEvent(uint32_t source, uint32_t dest, int type, int session, void *data, size_t sz, ucontext *ctx = NULL);

    static inline int sendEvent(uint32_t source, const char *addr, int type, int session, void *data, size_t sz, ucontext *ctx = NULL)
    {
        uint32_t dest = 0;
        if (source == 0)
        {
            goto _failed;
        }

        dest = addrTo(addr);
        if (dest == 0)
        {
            goto _failed;
        }

        return sendEvent(source, dest, type, session, data, sz, ctx);
    _failed:
        if (data)
            umemory::free(data);
        return -1;
    }

    static inline char *strDup(const char *str, size_t *str_sz)
    {
        *str_sz = 0;
        char *str_m = NULL;
        if (str)
        {
            str_m = umemory::strdup(str);
            *str_sz = strlen(str_m);
        }
        return str_m;
    }

    static inline int sendString(uint32_t source, uint32_t dest, int type, int session, const char *str, ucontext *ctx = NULL)
    {
        size_t str_z = 0;
        char *str_m = strDup(str, &str_z);
        return sendEvent(source, dest, type, session, str_m, str_z, ctx);
    }

    static inline int sendString(uint32_t source, const char *addr, int type, int session, const char *str, ucontext *ctx = NULL)
    {
        size_t str_z = 0;
        char *str_m = strDup(str, &str_z);
        return sendEvent(source, addr, type, session, str_m, str_z, ctx);
    }

    static void exit(ucontext *ctx, uint32_t id);

    static const char *luaTimeOut(ucontext *ctx, const char *param);

    static const char *luaName(ucontext *ctx, const char *param);

    static const char *luaQuery(ucontext *ctx, const char *param);

    static const char *luaExit(ucontext *ctx, const char *param)
    {
        exit(ctx, 0);
        return NULL;
    }

    static inline const char *luaKill(ucontext *ctx, const char *param)
    {
        uint32_t id = addrTo(param);
        if (id)
        {
            exit(ctx, id);
        }
        return NULL;
    }

    static const char *luaLaunch(ucontext *ctx, const char *param);

    static const char *luaGetEnv(ucontext *ctx, const char *param);

    static const char *luaSetEnv(ucontext *ctx, const char *param);

    static const char *luaStartTime(ucontext *ctx, const char *param);

    static const char *luaAbort(ucontext *ctx, const char *param);

    static const char *luaSignal(ucontext *ctx, const char *param);
};

} // namespace rednet

#endif
