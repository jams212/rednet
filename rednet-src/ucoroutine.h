#ifndef REDNET_UCOROUTINE_H
#define REDNET_UCOROUTINE_H


#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>

#if __APPLE__ && __MACH__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#define DEFAULT_COROUTINE 16

using namespace std;
namespace rednet
{

typedef void (*coroutine_func)(void *param);

enum coroutineStatus
{
    COROUTINE_DEAT = 0,
    COROUTINE_READY,
    COROUTINE_RUNNING,
    COROUTINE_SUSPEND,
}

struct coroutine
{
    coroutine_func fun__;
    void *funParm__;
    schedule *sch__;
    ucontext_t ctx__;
    ptrdiff_t cap__;
    ptrdiff_t size__;
    int32_i status__;
    char *stack__;
};

class uschedule
{
    public:
      uschedule(int32_t stacksize = 1024 * 1024);
      ~uschedule();

      struct coroutine* create(coroutine_func *f, void *parm);

      void resume(struct coroutine *co);

      void yield();

    private:
      char *stack__;
      int32_t stackSize__;
      struct coroutine *running__;
      ucontext_t main__;
};




}



#endif

