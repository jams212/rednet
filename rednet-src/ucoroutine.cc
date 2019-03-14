
#include "ucoroutine.h"
#include "umemory.h"


namespace rednet
{
    uschedule::uschedule(int32_t stacksize)
    {
        stackSize__ = stacksize;
        stack__ = umemory::malloc(stacksize);
        nco__ = 0;
        cap__ = 16;
    }

    uschedule::~uschedule()
    {
        umemory::free(stack__);
        stack__ = NULL;
    }

    struct coroutine* uschedule::create(coroutine_func *f, void *parm)
    {
        struct coroutine *co = (struct coroutine*)umemory::malloc(sizeof(*co));
        co->fun__ = f;
        co->funParm__ = parm;
        co->sch__ = this;
        co->status__ = COROUTINE_READY;
        co->stack__ = NULL;

        co->cap__ = 0;
        co->size__ = 0;
        return co;
    }

}