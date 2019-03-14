#ifndef REDNET_UTIMER_H
#define REDNET_UTIMER_H

#include "ucall.h"
#include "ucontext.h"
#include "uobject.h"
#include "usingleton.h"
#include "uspinlock.h"
#include "utimestamp.h"
#include <list>
#include <stdint.h>

#define TIME_SLOT_SHOFT 8
#define TIME_SLOT (1 << TIME_SLOT_SHOFT)
#define TIME_SLOT_MASK (TIME_SLOT - 1)
#define TIME_LEVEL_SHOFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHOFT)
#define TIME_LEVEL_MASK (TIME_LEVEL - 1)

using namespace std;
namespace rednet
{
struct timeEvent
{
    uint32_t id;
    int session;
};

struct timeHandle
{
    uint32_t expire;
    char data[1];
};

class utimeSlot : public uobject
{
    friend class utimer;

  public:
    utimeSlot() noexcept {}
    ~utimeSlot();

  private:
    void local_push(struct timeHandle *lnode);
    struct timeHandle *local_pop();

    void local_dispatch(struct timeHandle *lcurrent);

    inline void local_process(uspinlock *plock)
    {
        while (!lst__.empty())
        {
            struct timeHandle *lcur = lst__.front();
            lst__.pop_front();
            plock->unlock();
            local_dispatch(lcur);
            plock->lock();
        }
    }

  private:
    list<struct timeHandle *> lst__;
};

class utimer : public usingleton<utimer>
{
  public:
    utimer();
    ~utimer();

    void tick();

    uint64_t now() { return current__; }

    uint32_t startTime() { return starttime__; }

    int timeOut(uint32_t id, int time, int session);

  private:
    inline void local_shift()
    {
        int mask = TIME_SLOT;
        uint32_t ct = ++time__;
        if (ct == 0)
            local_move(3, 0);
        else
        {
            uint32_t time = ct >> TIME_SLOT_SHOFT;
            int i = 0;
            while ((ct & (mask - 1)) == 0)
            {
                int idx = time & TIME_LEVEL_MASK;
                if (idx != 0)
                {
                    local_move(i, idx);
                    break;
                }
                mask <<= TIME_LEVEL_SHOFT;
                time >>= TIME_LEVEL_SHOFT;
                ++i;
            }
        }
    }

    inline void local_execute()
    {
        int idx = time__ & TIME_SLOT_MASK;
        slots__[idx].local_process(&lock__);
    }

    inline void update()
    {
        lock__.lock();
        local_execute();
        local_shift();
        local_execute();
        lock__.unlock();
    }

    inline void local_move(int level, int idx)
    {
        struct timeHandle *lcurr = NULL;

        while ((lcurr = ts__[level][idx].local_pop()) != NULL)
        {
            local_append(lcurr);
        }
    }

    inline void local_append(struct timeHandle *lnde)
    {
        uint32_t time = lnde->expire;
        uint32_t current_time = time__;
        if ((time | (TIME_SLOT)) == (current_time | TIME_SLOT_MASK)) //current round
            slots__[time & TIME_LEVEL_MASK].local_push(lnde);
        else
        {
            int i;
            uint32_t mask = TIME_SLOT << TIME_LEVEL_SHOFT;
            for (i = 0; i < 3; i++)
            {
                if ((time | (mask - 1)) == (current_time | (mask - 1)))
                    break;
                mask <<= TIME_LEVEL_SHOFT;
            }
            ts__[i][(time >> (TIME_SLOT_SHOFT + i * TIME_LEVEL_SHOFT)) & TIME_LEVEL_MASK].local_push(lnde);
        }
    }

  private:
    utimeSlot slots__[TIME_SLOT];
    utimeSlot ts__[4][TIME_LEVEL];
    uspinlock lock__;

    uint32_t time__;
    uint32_t starttime__;
    uint64_t current__;
    uint64_t current_point__;
};

} // namespace rednet

#endif