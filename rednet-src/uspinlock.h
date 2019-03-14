
#ifndef REDNET_USPINLOCK_H
#define REDNET_USPINLOCK_H

#include <cstddef>

using namespace std;

namespace rednet
{

class uspinlock
{
  public:
    uspinlock() { k__ = 0; }
    ~uspinlock() = default;

    void lock()
    {
        while (__sync_lock_test_and_set(&k__, 1))
        {
        }
    }

    void unlock()
    {
        __sync_lock_release(&k__);
    }

    bool trylock()
    {
        return __sync_lock_test_and_set(&k__, 1) == 0;
    }

  private:
    int k__;
};

class uspinlocking
{
  public:
    uspinlocking(uspinlock *l) : l__(l) { l__->lock(); }

    ~uspinlocking()
    {
        l__->unlock();
        l__ = NULL;
    }

  private:
    uspinlock *l__;
};

} // namespace rednet

#endif

