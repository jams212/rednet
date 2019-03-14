#include "utimer.h"
#include "uevent.h"
#include "umemory.h"
#include <string.h>

using namespace rednet;

utimeSlot::~utimeSlot()
{
}

void utimeSlot::local_push(struct timeHandle *lnode)
{
    lst__.push_back(lnode);
}

struct timeHandle *utimeSlot::local_pop()
{
    struct timeHandle *result = NULL;
    if (lst__.empty())
        return NULL;
    result = lst__.front();
    lst__.pop_front();
    return result;
}

void utimeSlot::local_dispatch(struct timeHandle *lcurrent)
{
    struct timeEvent *levt = (struct timeEvent *)&lcurrent->data[0];
    uevent event;
    event.source = 0;
    event.data = NULL;
    event.type = EP_RESONE;
    event.sz = 0;
    ucall::sendEvent(levt->id, &event);
    umemory::free(lcurrent);
}

utimer::utimer() : time__(0),
                   starttime__(0),
                   current__(0),
                   current_point__(0)
{
    uint32_t tmp_cur = 0;
    timestamp::getSystemTime(&starttime__, &tmp_cur);
    current__ = tmp_cur;
    current_point__ = timestamp::getTime();
}

utimer::~utimer()
{
}

void utimer::tick()
{
    uint64_t ct = timestamp::getTime();
    if (ct < current_point__)
    {
        ucontext::error(0, "time diff error: change from %llu to %llu", ct, current_point__);
        current_point__ = ct;
    }
    else if (ct != current_point__)
    {
        uint32_t diff = (uint32_t)(ct - current_point__);
        current_point__ = ct;
        current__ += diff;
        for (uint32_t i = 0; i < diff; i++)
            update();
    }
}

int utimer::timeOut(uint32_t id, int time, int session)
{
    if (time <= 0)
    {
        uevent event;
        event.source = 0;
        event.data = NULL;
        event.session = session;
        event.type = EP_RESONE;
        event.sz = 0;
        ucall::sendEvent(id, &event);
    }
    else
    {
        struct timeEvent evt;
        evt.id = id;
        evt.session = session;

        size_t sz = sizeof(evt);
        struct timeHandle *lnde = (struct timeHandle *)umemory::malloc(sizeof(*lnde) - 1 + sz);

        memcpy(&lnde->data, &evt, sz);

        lock__.lock();
        lnde->expire = time + time__;
        local_append(lnde);
        lock__.unlock();
    }

    return session;
}
