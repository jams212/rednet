#include "uglobal.h"
#include "atomic.h"

namespace rednet
{

uglobal::uglobal() : contextTotal__(0),
                     systemInited__(false)
{
}

uglobal::~uglobal()
{
}

void uglobal::init()
{
    systemInited__ = true;
    if (pthread_key_create(&contextKey__, NULL))
    {
        fprintf(stderr, "pthread_key_create failed");
        exit();
    }
    bindWorker(1);
}

void uglobal::exit()
{
    pthread_key_delete(contextKey__);
}

void uglobal::bindWorker(int m)
{
    uintptr_t v = (uint32_t)(-m);
    pthread_setspecific(contextKey__, (void *)v);
}

uint32_t uglobal::currentContext()
{
    if (systemInited__)
    {
        void *id_data = pthread_getspecific(contextKey__);
        return (uint32_t)(uintptr_t)id_data;
    }
    else
    {
        return (uint32_t)(-1);
    }
}

void uglobal::contextInc()
{
    ATOM_INC(&contextTotal__);
}

void uglobal::contextDec()
{
    ATOM_DEC(&contextTotal__);
}

bool uglobal::contextEmpty()
{
    if (ATOM_CAS(&contextTotal__, 0, 0))
        return true;
    return false;
}

} // namespace rednet