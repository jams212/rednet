#ifndef REDNET_UEVENT_QUEUE_H
#define REDNET_UEVENT_QUEUE_H

#include "uevent.h"
#include "usingleton.h"
#include "uspinlock.h"
#include "uobject.h"
#include "ucall.h"
#include <assert.h>
#include <memory>
#include <queue>
#include <string.h>

using namespace std;

namespace rednet
{

class uevent_queue;
typedef shared_ptr<uevent_queue> eq_ptr;

class uglobal_queue : public usingleton<uglobal_queue>
{
  public:
  public:
    uglobal_queue() {}
    ~uglobal_queue() {}

    void push(eq_ptr ptr)
    {
        uspinlocking lk(&eqk__);
        eq__.push(ptr);
    }

    eq_ptr pop()
    {
        uspinlocking lk(&eqk__);
        if (eq__.empty())
            return nullptr;
        eq_ptr ptr = eq__.front();
        eq__.pop();
        return ptr;
    }

  private:
    queue<eq_ptr> eq__;
    uspinlock eqk__;
};

class uevent_queue : public uobject
{
    static constexpr auto DEFAULT_SIZE = 64;
    static constexpr auto EQ_OVERLOAD = 1024;

  public:
    uevent_queue(uint32_t id) : id(id),
                                cap__(DEFAULT_SIZE),
                                head__(0),
                                tail__(0),
                                inglobal__(1),
                                overload__(0),
                                overload_threshold__(EQ_OVERLOAD)
    {
        eqs__ = (struct uevent *)umemory::malloc(sizeof(struct uevent) * cap__);
        memset(eqs__, 0, sizeof(struct uevent) * cap__);
    }

    ~uevent_queue()
    {
        uevent e;
        while (pop(&e))
        {
            local_dropevent(&e);
        }

        umemory::free(eqs__);
        eqs__ = NULL;
    }

    int length()
    {
        int head, tail, cap;
        k__.lock();
        head = head__;
        tail = tail__;
        cap = cap__;
        k__.unlock();

        if (head <= tail)
            return tail - head;
        return tail + cap - head;
    }

    void push(uevent *event, eq_ptr ptr);

    bool pop(uevent *event)
    {
        bool ret = false;

        uspinlocking lk(&k__);
        if (head__ != tail__)
        {
            ret = true;
            *event = eqs__[head__++];
            if (head__ >= cap__)
                head__ = 0;

            int length = tail__ - head__;
            if (length < 0)
                length += cap__;

            while (length > overload_threshold__)
            {
                overload__ = length;
                overload_threshold__ *= 2;
            }
        }
        else
            overload_threshold__ = EQ_OVERLOAD;

        if (!ret)
            inglobal__ = 0;

        return ret;
    }

    int overload()
    {
        if (overload__)
        {
            int overload = overload__;
            overload__ = 0;
            return overload;
        }
        return 0;
    }

  public:
    uint32_t id;

  private:
    void local_expand()
    {
        uevent *new_eq = new uevent[cap__ * 2];
        for (int i = 0; i < cap__; i++)
        {
            new_eq[i] = eqs__[(head__ + i) % cap__];
        }

        head__ = 0;
        tail__ = cap__;
        cap__ *= 2;

        umemory::free(eqs__);
        eqs__ = new_eq;
    }

    void local_dropevent(uevent *e)
    {
        if(e->data)
            umemory::free(e->data);
        uint32_t source = id;
        assert(source);
        ucall::sendEvent(source, e->source, EP_ERROR, 0, NULL, 0);
    }

  private:
    uspinlock k__;
    uevent *eqs__;
    int cap__;
    int head__;
    int tail__;
    int inglobal__;
    int overload__;
    int overload_threshold__;
};

} // namespace rednet

#endif