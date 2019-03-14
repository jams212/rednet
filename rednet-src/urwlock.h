#ifndef REDNET_URWLOCK_H
#define REDNET_URWLOCK_H

#include <pthread.h>

namespace rednet
{
class urwlock
{
  public:
    urwlock()
    {
        pthread_rwlock_init(&k__, NULL);
    }

    ~urwlock()
    {
        pthread_rwlock_destroy(&k__);
    }

    inline void wlock()
    {
        pthread_rwlock_wrlock(&k__);
    }

    inline void rlock()
    {
        pthread_rwlock_rdlock(&k__);
    }

    inline void unlock()
    {
        pthread_rwlock_unlock(&k__);
    }

  private:
    pthread_rwlock_t k__;
};

class wlocking
{
  public:
    wlocking(urwlock *h) : h__(h)
    {
        h__->wlock();
    }

    ~wlocking()
    {
        h__->unlock();
        h__ = NULL;
    }

  private:
    urwlock *h__;
};

class rlocking
{
  public:
    rlocking(urwlock *h) : h__(h)
    {
        h__->rlock();
    }

    ~rlocking()
    {
        h__->unlock();
        h__ = NULL;
    }

  private:
    urwlock *h__;
};
} // namespace rednet

#endif