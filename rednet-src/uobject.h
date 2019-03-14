#ifndef REDNET_OBJECT_H
#define REDNET_OBJECT_H

#include "macro.h"
#include "umemory.h"
#include <stddef.h>
#include <stdint.h>

namespace rednet
{
class uobject
{
  public:
    R_CONSTRUCTOR(uobject)

    /*void *operator new(size_t size)
    {
        return umemory::malloc(size);
    }

    void *operator new[](size_t size)
    {
        return umemory::malloc(size);
    }

    void operator delete(void *p)
    {
        umemory::free(p);
    }

    void operator delete[](void *p)
    {
        umemory::free(p);
    }*/
};

} // namespace rednet

#endif