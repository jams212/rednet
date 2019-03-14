#ifndef GSTRUCTURE_HEAP_MIN_H
#define GSTRUCTURE_HEAP_MIN_H

template <typename TD, int (*Get)(TD *val), TD *(*Alloc)(int num), void (*Free)(TD *p)>
class binary_heap_min
{
  public:
    void push(TD *data)
    {
        if (_sz == _cap)
        {
            int tmpcap = _cap << 1;
            TD **tmp = _heap;
            _heap = Alloc(tmpcap);
            _cap = tmpcap;
            for (int i = 0; i < _sz; i++)
            {
                _heap[i] = tmp[i];
            }
            Free(tmp);
        }

        _heap[_sz] = data;
        _filterup(_sz);
        _sz++;
    }

  private:
    int _get_index(const TD *value)
    {
        int i = 0;
        for (i = 0; i < _sz; i++)
        {
            if (Get(value) == Get(_heap[i]))
                return i;
        }
        return -1;
    }

    void _filterup(int start)
    {
        int c = start;
        int p = (c - 1) / 2;
        int tmp = _heap[c];
        while (c > 0)
        {
            if (_heap[p] <= tmp)
                break;
            else
            {
                _heap[c] = _heap[p];
                c = p;
                p = (p - 1) / 2;
            }
        }
        _heap[c] = tmp;
    }

  private:
    int _cap;
    int _sz;
    TD **_heap;
};

#endif
