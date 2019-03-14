#ifndef REDNET_SET_H
#define REDNET_SET_H

#define SET_DEFAULT_SIZE 16

#include "alloc.h"

template <typename TData>
class set
{
  public:
    set(void *ud, aoi::Alloc al) : _sz(SET_DEFAULT_SIZE),
                                   _number(0),
                                   _ud(ud),
                                   _alloc(al)
    {
        _slots = (TData **)_alloc(_ud, NULL, sizeof(TData *) * _sz);
    }

    ~set()
    {
        _alloc(_ud, _slots, sizeof(TData *) * _sz);
        _slots = NULL;
    }

    void push_back(TData *dat)
    {
        if (_number >= _sz)
        {
            int sz = _sz << 1;
            void *tmp = _slots;
            _slots = (TData **)_alloc(_ud, NULL, sizeof(TData *) * sz);
            memcpy(_slots, tmp, _sz * sizeof(TData *));
            _alloc(_ud, tmp, sizeof(TData *) * _sz);

            _sz = sz;
        }
        _slots[_number] = dat;
        ++_number;
    }

    TData *get(int i)
    {
        return _slots[i];
    }

    int get_number()
    {
        return _number;
    }

    void zero_number()
    {
        _number = 0;
    }

  private:
    int _sz;
    int _number;
    TData **_slots;

    void *_ud;
    aoi::Alloc _alloc;
};

#endif
