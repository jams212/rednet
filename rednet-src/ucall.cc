#include "ucall.h"
#include "ucontext.h"
#include "uenv.h"
#include "uframework.h"
#include "utimer.h"

using namespace rednet;

void ucall::error(ucontext *ctx, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    if (ctx == NULL)
        ucontext::error(0, msg, ap);
    else
        ucontext::error(ctx->id, msg, ap);
    va_end(ap);
}

uint64_t ucall::now()
{
    return INST(utimer, now);
}

uint32_t ucall::addrTo(const char *addr)
{
    uint32_t dest = 0;
    if (addr[0] == ':')
        dest = strtoul(addr + 1, NULL, 16);
    else if (addr[0] == '.')
    {
        dest = INST(uctxMgr, convertId, addr + 1);
        if (dest == 0)
        {
            fprintf(stderr, "fail:%s\n", addr + 1);
        }
    }
    return dest;
}

bool ucall::sendEvent(uint32_t id, uevent *e)
{
    ctx_ptr ptr = INST(uctxMgr, grab, id);
    if (ptr == nullptr)
        return false;
    ptr->eq->push(e, ptr->eq);
    return true;
}

int ucall::sendEvent(uint32_t source, uint32_t dest, int type, int session, void *data, size_t sz, ucontext *ctx)
{
    type &= 0xff;
    if (ctx)
    {
        assert(session == 0);
        session = ctx->session();
    }

    if (dest == 0)
        return session;
    uevent e;
    e.source = source;
    e.data = data;
    e.session = session;
    e.type = type;
    e.sz = sz;

    if (!sendEvent(dest, &e))
    {
        goto _failed;
    }

    return session;
_failed:
    if (data)
        umemory::free(data);
    return -1;
}

void ucall::exit(ucontext *ctx, uint32_t id)
{
    if (id == 0)
    {
        id = ctx->id;
        ucontext::error(id, "KILL self");
    }
    else
    {
        ucontext::error(ctx->id, "KILL :%x", id);
    }

    INST(uctxMgr, unRegister, id);
}

const char *ucall::luaTimeOut(ucontext *ctx, const char *param)
{
    char *session_ptr = NULL;
    int ti = strtol(param, &session_ptr, 10);
    int session = ctx->session();
    INST(utimer, timeOut, ctx->id, ti, session);
    sprintf(ctx->result, "%d", session);
    return ctx->result;
}

const char *ucall::luaName(ucontext *ctx, const char *param)
{
    int size = strlen(param);
    char name[size + 1];
    char handle[size + 1];
    sscanf(param, "%s %s", name, handle);
    if (handle[0] != ':')
    {
        return NULL;
    }
    uint32_t id = strtoul(handle + 1, NULL, 16);
    if (id == 0)
    {
        return NULL;
    }
    if (name[0] == '.')
    {
        return INST(uctxMgr, registerName, id, name + 1);
    }
    else
    {
        ucontext::error(ctx->id, "Can't set global name %s in CXX", name);
    }
    return NULL;
}

const char *ucall::luaQuery(ucontext *ctx, const char *param)
{
    if (param[0] == '.')
    {
        uint32_t id = INST(uctxMgr, convertId, param + 1);
        if (id)
        {
            sprintf(ctx->result, ":%x", id);
            return ctx->result;
        }
    }
    return NULL;
}

const char *ucall::luaLaunch(ucontext *ctx, const char *param)
{
    size_t sz = strlen(param);
    char tmp[sz + 1];
    strcpy(tmp, param);
    char *args = tmp;
    char *mod = strsep(&args, " \t\r\n");
    args = strsep(&args, "\r\n");
    ucontext *inst = new ucontext();
    if (inst->init(mod, args) == 0)
    {
        return NULL;
    }
    else
    {
        toAddr(inst->result, inst->id);
        return inst->result;
    }
}

const char *ucall::luaGetEnv(ucontext *ctx, const char *param)
{
    return INST(uenv, getEnv, param);
}

const char *ucall::luaSetEnv(ucontext *ctx, const char *param)
{
    size_t sz = strlen(param);
    char key[sz + 1];
    int i;
    for (i = 0; param[i] != ' ' && param[i]; i++)
    {
        key[i] = param[i];
    }
    if (param[i] == '\0')
        return NULL;

    key[i] = '\0';
    param += i + 1;

    INST(uenv, setEnv, key, param);
    return NULL;
}

const char *ucall::luaStartTime(ucontext *ctx, const char *param)
{
    uint32_t sec = INST(utimer, startTime);
    sprintf(ctx->result, "%u", sec);
    return NULL;
}

const char *ucall::luaAbort(ucontext *ctx, const char *param)
{
    INST(uctxMgr, retireAll);
    return NULL;
}

const char *ucall::luaSignal(ucontext *ctx, const char *param)
{
    uint32_t handle = addrTo(param);
    if (handle == 0)
        return NULL;
    ctx_ptr obj_ent = INST(uctxMgr, grab, handle);
    if (obj_ent == nullptr)
        return NULL;
    param = strchr(param, ' ');
    int sig = 0;
    if (param)
    {
        sig = strtol(param, NULL, 0);
    }

    if (obj_ent->cp__)
        obj_ent->cp__->signal(sig);

    return NULL;
}