#include "ucontext.h"
#include "uglobal.h"
#include "utimestamp.h"
#include <stdarg.h>

using namespace rednet;

ucontext::ucontext() : eq(nullptr),
                       cp__(NULL),
                       md__(NULL),
                       session_id__(0),
                       cpu_cost__(0),
                       cpu_start__(0),
                       event_count__(0),
                       profile__(false),
                       inited__(false)
{
}

ucontext::~ucontext()
{
    eq = nullptr;
    RELEASE(cp__);
    INST(uglobal, contextDec);
}

uint32_t ucontext::init(const char *name, const char *parm)
{
    umodule *dl = INST(umodMgr, query, name);
    if (!dl)
        return 0;
    ucomponent *hcomp = dl->create_cb__();
    if (!hcomp)
        return 0;
    id = INST(uctxMgr, registerId, this);
    if (id == 0)
        return 0;
    md__ = dl;
    cp__ = hcomp;
    uevent_queue *q = new uevent_queue(id);
    eq = eq_ptr(q);
    int r = cp__->init(this, parm);
    if (r == 0)
    {
        INST(uctxMgr, registerName, id, name);
        INST(uglobal, contextInc);
        inited__ = true;
        INST(uglobal_queue, push, eq);
        ucontext::error(id, "STARTED %s %s", name, parm ? parm : "");
        return id;
    }
    else
    {
        ucontext::error(id, "FAILED started %s", name);
        INST(uctxMgr, unRegister, id);
        return 0;
    }
}

void ucontext::execute(uevent *e)
{
    if (cp__ == NULL || cp__->callback__ == nullptr)
    {
        umemory::free(e->data);
    }
    else
    {
        assert(inited__);
        int type = e->type;
        size_t sz = e->sz;

        int result;
        if (profile__)
        {
            cpu_start__ = UTIMESTAMP(getThreadTime);
            result = cp__->callback__(cp__->ud__, type, e->source, e->session, e->data, sz);
            uint64_t cost_ts = UTIMESTAMP(getThreadTime);
            cpu_cost__ += (cost_ts - cpu_start__);
        }
        else
            result = cp__->callback__(cp__->ud__, type, e->source, e->session, e->data, sz);

        if (!result)
        {
            umemory::free(e->data);
        }
    }
}

void ucontext::disponseAll()
{
    uevent ev;
    eq_ptr eqMsg = eq;
    if (eqMsg == nullptr)
        return;
    while (eqMsg->pop(&ev))
    {
        execute(&ev);
    }
}

void ucontext::error(uint32_t source, const char *msg, ...)
{
    static uint32_t logger = INST(uctxMgr, convertId, "logger");
    if (logger == 0)
        return;

    static constexpr int log_message_size = 256;
    char tmp[log_message_size];
    char *data = NULL;

    va_list ap;

    va_start(ap, msg);
    int len = vsnprintf(tmp, log_message_size, msg, ap);
    va_end(ap);

    if (len >= 0 && len < log_message_size)
        data = (char *)umemory::strdup(tmp);
    else
    {
        int tmp_max = log_message_size;
        for (;;)
        {
            tmp_max *= 2;
            data = (char *)umemory::malloc(tmp_max);
            va_start(ap, msg);
            len = vsnprintf(data, tmp_max, msg, ap);
            va_end(ap);
            if (len < tmp_max)
                break;
            umemory::free(data);
        }
    }

    if (len < 0)
    {
        umemory::free(data);
        perror("vsnprintf error :");
        return;
    }

    uevent event;
    event.source = source;
    event.session = 0;
    event.data = data;
    event.type = EP_TEXT;
    event.sz = len;

    ctx_ptr logger_ptr = INST(uctxMgr, grab, logger);
    logger_ptr->eq->push(&event, logger_ptr->eq);
}

int32_t ucontext::session()
{
    int session = ++session_id__;
    if (session <= 0)
    {
        session_id__ = 1;
        return 1;
    }
    return session;
}

const char *ucontext::self()
{
    sprintf(result, ":%x", id);
    return result;
}

uctxMgr::uctxMgr() : ctxsSer__(1),
                     ctxsCap__(4)
{
    ctxs__.resize(ctxsCap__, nullptr);
}

uctxMgr::~uctxMgr()
{
    retireAll();
}

void uctxMgr::init(int serverGroupID)
{
    serverGroupID__ = (uint32_t)(serverGroupID & 0xff) << ID_REMOTE_SHIFT;
}

uint32_t uctxMgr::registerId(ucontext *ctx)
{
    ctxsLck__.wlock();
    for (;;)
    {
        int i;
        for (i = 0; i < ctxsCap__; i++)
        {
            uint32_t id = (i + ctxsSer__) & ID_MARK;
            int hash = id & (ctxsCap__ - 1);
            if (ctxs__[hash] == nullptr)
            {
                ctxs__[hash] = ctx_ptr(ctx);
                ctxsSer__ = id + 1;
                ctxsLck__.unlock();
                id |= serverGroupID__;
                return id;
            }
        }

        assert((ctxsCap__ * 2 - 1) <= ID_MARK);
        ctxs__.resize(ctxsCap__ * 2, nullptr);
        int nwcap = ctxsCap__ * 2;
        for (i = 0; i < nwcap; i++)
        {
            if (ctxs__[i] == nullptr)
                continue;
            int nwpos = ctxs__[i]->id & (nwcap - 1);
            if (nwpos == i)
                continue;
            ctxs__[nwpos] = ctxs__[i];
            ctxsCap__ = nwcap;
        }
    }
}

const char *uctxMgr::registerName(uint32_t id, const char *name)
{
    wlocking wk(&ctxsLck__);
    if (!ctxsName__.empty())
    {
        umaps::iterator it = ctxsName__.find(name);
        if (it != ctxsName__.end())
        {
            it->second = id;
            return it->first;
        }
    }

    const char *tmp = (const char *)umemory::strdup(name);
    ctxsName__.insert(pair<const char *, uint32_t>(tmp, id));
    return tmp;
}

bool uctxMgr::unRegister(uint32_t id)
{
    bool bret = false;
    wlocking wk(&ctxsLck__);
    uint32_t hash = local_addr(id);
    if (ctxs__[hash] != nullptr && ctxs__[hash]->id == id)
    {
        bret = true;
        local_erase_name(id);
        ctxs__[hash] = nullptr;
    }

    return bret;
}

ctx_ptr uctxMgr::grab(uint32_t id)
{
    ctx_ptr result = nullptr;
    rlocking rk(&ctxsLck__);
    uint32_t hash = local_addr(id);
    if (ctxs__[hash] && ctxs__[hash]->id == id)
    {
        result = ctxs__[hash];
    }
    return result;
}

uint32_t uctxMgr::convertId(const char *name)
{
    rlocking rk(&ctxsLck__);
    if (ctxsName__.empty())
        return 0;
    umaps::iterator it = ctxsName__.find(name);
    if (it == ctxsName__.end())
    {
        return 0;
    }
    return it->second;
}

const char *uctxMgr::convertName(uint32_t id)
{
    rlocking rk(&ctxsLck__);
    if (ctxsName__.empty())
        return NULL;
    umaps::iterator it = ctxsName__.begin();
    while (it != ctxsName__.end())
    {
        if (it->second != id)
        {
            ++it;
            continue;
        }
        return it->first;
    }
    return NULL;
}

void uctxMgr::retireAll()
{
    for (;;)
    {
        int i;
        for (i = 0; i < ctxsCap__; i++)
        {
            ctxsLck__.rlock();
            ctx_ptr ptr = ctxs__[i];
            uint32_t id = 0;
            if (ptr != nullptr)
                id = ptr->id;
            ctxsLck__.unlock();
            if (id == 0)
                continue;
            unRegister(id);
        }
    }
}

void uctxMgr::local_erase_name(uint32_t id)
{
    if (ctxsName__.empty())
        return;
    umaps::iterator it = ctxsName__.begin();
    while (it != ctxsName__.end())
    {
        if (it->second != id)
        {
            ++it;
            continue;
        }

        const char *tmp = it->first;
        ctxsName__.erase(it);
        umemory::free((void *)tmp);
        break;
    }
}
