#ifndef REDNET_ULOCK_H
#define REDNET_ULOCK_H

#include <pthread.h>

namespace rednet
{
class ulock
{
  public:
    ulock()
    {
        pthread_mutex_init(&mutex__);
    }

    ~ulock()
    {
        pthread_mutex_destroy(&mutex__);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex__);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex__);
    }

    bool trylock()
    {
        if (pthread_mutex_trylock(&mutex__) == 0)
        {
            return true;
        }
        return false;
    }

  private:
    pthread_mutex_t mutex__;
};

class ulocking
{
  public:
    ulocking(ulock *l) : l__(l)
    {
        l__->lock();
    }

    ~ulocking()
    {
        l__->unlock();
    }

  private:
    ulock *l__;
};

} // namespace rednet

#endif