#ifndef REDNET_MAP_H
#define REDNET_MAP_H

#include <assert.h>
#include <stdint.h>

#define DEFAULT_MAP_SZIE 16
#define MAP_INVALID_ID (~0)

#include "alloc.h"

template <typename TData>
class map
{
    struct map_slot
    {
        uint32_t _id;
        TData *_dat;
        int _next;
    };

  public:
    map(void *ud, aoi::Alloc al) : _ud(ud),
                                   _alloc(al)
    {
        _sz = DEFAULT_MAP_SZIE;
        _lastfree = DEFAULT_MAP_SZIE - 1;
        _slots = (struct map_slot *)_alloc(_ud, NULL, _sz * sizeof(struct map_slot));
        for (int i = 0; i < _sz; i++)
        {
            struct map_slot *s = &_slots[i];
            s->_id = MAP_INVALID_ID;
            s->_dat = NULL;
            s->_next = -1;
        }
    }

    ~map()
    {
        _alloc(_ud, _slots, _sz * sizeof(struct map_slot));
        _slots = NULL;
    }

    void insert(uint32_t id, TData *dat)
    {
        struct map_slot *s = _getslot(id);
        if (s->_id == MAP_INVALID_ID)
        {
            s->_id = id;
            s->_dat = dat;
            return;
        }

        if (_getslot(s->_id) != s)
        {
            struct map_slot *last = _getslot(s->_id);
            while (last->_next != s - _slots)
            {
                assert(last->_next >= 0);
                last = &_slots[last->_next];
            }

            uint32_t temp_id = s->_id;
            TData *temp_dat = s->_dat;
            last->_next = s->_next;
            s->_id = id;
            s->_dat = dat;
            s->_next = -1;
            if (temp_dat)
            {
                insert(temp_id, temp_dat);
            }

            return;
        }

        while (_lastfree >= 0)
        {
            struct map_slot *temp = &_slots[_lastfree--];
            if (temp->_id == MAP_INVALID_ID)
            {
                temp->_id = id;
                temp->_dat = dat;
                temp->_next = s->_next;
                s->_next = (int)(temp - _slots);
                return;
            }
        }

        _resize();
        insert(id, dat);
    }

    TData *query(uint32_t id, TData *(*func)(void *ud, uint32_t id), void *ud)
    {
        struct map_slot *s = _getslot(id);
        for (;;)
        {
            if (s->_id == id)
            {
                if (!s->_dat)
                    s->_dat = func(ud, id);
                return s->_dat;
            }

            if (s->_next < 0)
            {
                break;
            }
            s = &_slots[s->_next];
        }

        TData *obj = func(ud, id);
        insert(id, obj);
        return obj;
    }

    TData *drop(uint32_t id)
    {
        uint32_t hash = id & (_sz - 1);
        struct map_slot *s = &_slots[hash];
        for (;;)
        {
            if (s->_id == id)
            {
                TData *dat = s->_dat;
                s->_dat = NULL;
                return dat;
            }
            if (s->_next < 0)
            {
                return NULL;
            }
            s = &_slots[s->_next];
        }
    }

    void foreach (void (*func)(void *ud, TData *obj), void *ud)
    {
        int i;
        for (i = 0; i < _sz; i++)
        {
            if (_slots[i]._dat)
            {
                func(ud, _slots[i]._dat);
            }
        }
    }

  private:
    inline struct map_slot *_getslot(uint32_t id)
    {
        uint32_t hash = id & (_sz - 1);
        return &_slots[hash];
    }

    void _resize()
    {
        struct map_slot *old_slots = _slots;
        size_t old_sz = _sz;
        _sz <<= 1;
        _slots = (struct map_slot *)_alloc(_ud, NULL, _sz * sizeof(struct map_slot));
        _lastfree = _sz - 1;
        int i;
        for (i = 0; i < _sz; i++)
        {
            struct map_slot *s = &_slots[i];
            s->_id = MAP_INVALID_ID;
            s->_dat = NULL;
            s->_next = -1;
        }

        for (i = 0; i < (int)old_sz; i++)
        {
            struct map_slot *s = &old_slots[i];
            if (s->_dat)
                insert(s->_id, s->_dat);
        }

        _alloc(_ud, old_slots, old_sz * sizeof(struct map_slot));
    }

  private:
    int _sz;
    int _lastfree;
    struct map_slot *_slots;

    void *_ud;
    aoi::Alloc _alloc;
};

#endif
