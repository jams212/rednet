#include "uwork.h"
#include "umemory.h"

#include <assert.h>

using namespace rednet;

void *uworker::__run__(void *parm)
{
    struct uworker_param *wp = (struct uworker_param *)(parm);
    wp->f(wp->p);
    umemory::free(wp);
    return 0L;
}

uworker::uworker(uwork_func f, void *parm)
{
    struct uworker_param *wp = (struct uworker_param *)umemory::malloc(sizeof(*wp));
    assert(wp);
    wp->p = parm;
    wp->w = this;
    wp->f = f;
    if (pthread_create(&t__, NULL, uworker::__run__, (void *)wp) != 0)
    {
        umemory::free(wp);
        fprintf(stderr, "rednet : create thread fail.\n");
        exit(0);
    }
}

uworker::~uworker()
{
}

void uworker::join()
{
    pthread_join(t__, NULL);
}

uworks::uworks() : count__(0),
                   sleep__(0),
                   shutdown__(0)
{
    pthread_cond_init(&cd__, NULL);
    pthread_mutex_init(&tx__, NULL);
}

uworks::~uworks()
{
    while (!ws__.empty())
    {
        uworker *w = ws__.front();
        ws__.erase(ws__.begin());
        delete w;
    }

    pthread_cond_destroy(&cd__);
    pthread_mutex_destroy(&tx__);
}

void uworks::stop()
{
    pthread_mutex_lock(&tx__);
    shutdown__ = 1;
    pthread_cond_broadcast(&cd__);
    pthread_mutex_unlock(&tx__);
}

void uworks::append(uwork_func f, void *parm)
{
    uworker *wk = new uworker(f, parm);
    assert(wk);
    ws__.push_back(wk);
}

void uworks::wakeup(int busy)
{
    if (sleep__ >= count__ - busy)
        pthread_cond_signal(&cd__);
}

void uworks::wait()
{
    if (pthread_mutex_lock(&tx__) == 0)
    {
        ++sleep__;
        if (!shutdown__)
            pthread_cond_wait(&cd__, &tx__);
        --sleep__;
        if (pthread_mutex_unlock(&tx__))
        {
            fprintf(stderr, "unlock mutex error");
            exit(1);
        }
    }
}

void uworks::join()
{
    for (size_t i = 0; i < ws__.size(); i++)
    {
        ws__[i]->join();
    }
}
