#include "uevent_queue.h"

using namespace rednet;

void uevent_queue::push(uevent *event, eq_ptr ptr)
{
    assert(event);
    uspinlocking lk(&k__);

    eqs__[tail__] = *event;
    if (++tail__ >= cap__)
        tail__ = 0;

    if (head__ == tail__)
        local_expand();

    if (inglobal__ == 0)
    {
        inglobal__ = 1;
        INST(uglobal_queue, push, ptr);
    }
}
