#ifndef REDNET_URECURESIVELOCK_H
#define REDNET_URECURESIVELOCK_H

#include "uspinlock.h"
#include <assert.h>

namespace rednet
{
class urecuresivelock
{
  public:
    urecuresivelock(uspinlock *ls) : lk__(ls),
                                     count__(0)
    {
    }
    ~urecuresivelock() {}

    void lock()
    {
        if (count__ == 0)
        {
            lk__->lock();
        }
        ++count__;
    }

    int trylock()
    {
        if (count__ == 0)
        {
            if (!lk__->trylock())
                return 0;
        }
        ++count__;
        return 1;
    }

    void unlock()
    {
        --count__;
        if (count__ <= 0)
        {
            assert(count__ == 0);
            lk__->unlock();
        }
    }

  private:
    uspinlock *lk__;
    int count__;
};
} // namespace rednet

#endif
