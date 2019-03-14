#include "aoi.h"
#include "math.h"
#include <malloc.h>
#include <string.h>

#define MODE_WATCHER 1
#define MODE_MARKER 2
#define MODE_MOVE 4
#define MODE_DROP 8

aoi_space::aoi_space(void *ud, aoi::Alloc al) : _object(ud, al),
                                                _watcher_static(ud, al),
                                                _marker_static(ud, al),
                                                _watcher_move(ud, al),
                                                _marker_move(ud, al),
                                                _hot(NULL),
                                                _alloc(al),
                                                _alloc_ud(ud),
                                                _radis(10.0f)
{
}

aoi_space::aoi_space() : _object(NULL, aoi_space::_default_alloc__),
                         _watcher_static(NULL, aoi_space::_default_alloc__),
                         _marker_static(NULL, aoi_space::_default_alloc__),
                         _watcher_move(NULL, aoi_space::_default_alloc__),
                         _marker_move(NULL, aoi_space::_default_alloc__),
                         _hot(NULL),
                         _alloc(aoi_space::_default_alloc__),
                         _alloc_ud(NULL),
                         _radis(10.0f)
{
}

aoi_space::~aoi_space()
{
    _object.foreach (aoi_space::_delete_object__, this);
    _delete_pair_list();
}

void aoi_space::update(uint32_t id, const char *modestr, float pos[3])
{
    struct object *obj = _object.query(id, aoi_space::_alloc_object__, this);
    int i;
    bool set_watcher = false;
    bool set_marker = false;

    for (i = 0; modestr[i]; ++i)
    {
        char m = modestr[i];
        switch (m)
        {
        case 'w':
            set_watcher = true;
            break;
        case 'm':
            set_marker = true;
            break;
        case 'd':
            if (!(obj->_mode & MODE_DROP))
            {
                obj->_mode = MODE_DROP;
                _release_object(obj);
            }
            return;
        }
    }

    if (obj->_mode & MODE_DROP)
    {
        obj->_mode &= ~MODE_DROP;
        _grab_object__(obj);
    }

    bool changed = _change_mode__(obj, set_watcher, set_marker);
    _copy_pos__(obj->_position, pos);
    if (changed || !math::near__(pos, obj->_last, _radis))
    {
        _copy_pos__(obj->_last, pos);
        obj->_mode |= MODE_MOVE;
        ++obj->_version;
    }
}

void aoi_space::message(aoi::CallBack cb, void *ud)
{
    _flush_pair(cb, ud);
    _watcher_static.zero_number();
    _marker_static.zero_number();
    _watcher_move.zero_number();
    _marker_move.zero_number();
    _object.foreach (aoi_space::_set_push__, this);
    _gen_pair_list(&_watcher_static, &_marker_move, cb, ud);
    _gen_pair_list(&_watcher_move, &_marker_static, cb, ud);
    _gen_pair_list(&_watcher_move, &_marker_move, cb, ud);
}

void aoi_space::_flush_pair(aoi::CallBack cb, void *ud)
{
    struct pair_list **last = &(_hot);
    struct pair_list *p = *last;
    while (p)
    {
        struct pair_list *next = p->_next;
        if (p->_watcher->_version != p->_watcher_version ||
            p->_marker->_version != p->_marker_version ||
            (p->_watcher->_mode & MODE_DROP) ||
            (p->_marker->_mode & MODE_DROP))
        {
            _drop_pair(p);
            *last = next;
        }
        else
        {
            float distance2 = math::disp__(p->_watcher->_position, p->_marker->_position);
            if (distance2 > math::pow__(_radis) * 4)
            {
                _drop_pair(p);
                *last = next;
            }
            else if (distance2 < math::pow__(_radis))
            {
                cb(ud, p->_watcher->_id, p->_marker->_id);
                _drop_pair(p);
                *last = next;
            }
            else
            {
                last = &(p->_next);
            }
        }
        p = next;
    }
}

void aoi_space::_gen_pair_list(set<object> *watcher, set<object> *marker, aoi::CallBack cb, void *ud)
{
    int i, j;
    for (i = 0; i < watcher->get_number(); i++)
    {
        for (j = 0; j < marker->get_number(); j++)
        {
            _gen_pair(watcher->get(i), marker->get(j), cb, ud);
        }
    }
}

void aoi_space::_gen_pair(struct object *watcher, struct object *marker, aoi::CallBack cb, void *ud)
{
    if (watcher == marker)
    {
        return;
    }

    float distance2 = math::disp__(watcher->_position, marker->_position);
    if (distance2 < math::pow__(_radis))
    {
        cb(ud, watcher->_id, marker->_id);
        return;
    }

    if (distance2 > math::pow__(_radis) * 4)
    {
        return;
    }

    struct pair_list *p = (struct pair_list *)_alloc(_alloc_ud, NULL, sizeof(*p));
    p->_watcher = watcher;
    _grab_object__(watcher);
    p->_marker = marker;
    _grab_object__(marker);
    p->_watcher_version = watcher->_version;
    p->_marker_version = marker->_version;
    p->_next = _hot;
    _hot = p;
}

void aoi_space::_drop_pair(struct pair_list *p)
{
    _release_object(p->_watcher);
    _release_object(p->_marker);
    _alloc(_alloc_ud, p, sizeof(*p));
}

void aoi_space::_delete_pair_list()
{
    struct pair_list *p = _hot;

    while (p)
    {
        struct pair_list *next = p->_next;
        _alloc(_alloc_ud, p, sizeof(*p));
        p = next;
    }
}

bool aoi_space::_change_mode__(struct object *pobj, bool set_watcher, bool set_marker)
{
    bool change = false;
    if (pobj->_mode == 0)
    {
        if (set_watcher)
        {
            pobj->_mode = MODE_WATCHER;
        }
        if (set_marker)
        {
            pobj->_mode |= MODE_MARKER;
        }
        return true;
    }
    if (set_watcher)
    {
        if (!(pobj->_mode & MODE_WATCHER))
        {
            pobj->_mode |= MODE_WATCHER;
            change = true;
        }
    }
    else
    {
        if (pobj->_mode & MODE_WATCHER)
        {
            pobj->_mode &= ~MODE_WATCHER;
            change = true;
        }
    }
    if (set_marker)
    {
        if (!(pobj->_mode & MODE_MARKER))
        {
            pobj->_mode |= MODE_MARKER;
            change = true;
        }
    }
    else
    {
        if (pobj->_mode & MODE_MARKER)
        {
            pobj->_mode &= ~MODE_MARKER;
            change = true;
        }
    }
    return change;
}

void aoi_space::_set_push__(void *s, struct object *pobj)
{
    aoi_space *space = static_cast<aoi_space *>(s);
    int mode = pobj->_mode;
    if (mode & MODE_WATCHER)
    {
        if (mode & MODE_MOVE)
        {
            space->_watcher_move.push_back(pobj);
            pobj->_mode &= ~MODE_MOVE;
        }
        else
        {
            space->_watcher_static.push_back(pobj);
        }
    }
    if (mode & MODE_MARKER)
    {
        if (mode & MODE_MOVE)
        {
            space->_marker_move.push_back(pobj);
            pobj->_mode &= ~MODE_MOVE;
        }
        else
        {
            space->_marker_static.push_back(pobj);
        }
    }
}

void aoi_space::_grab_object__(struct object *pobj)
{
    ++pobj->_ref;
}

void aoi_space::_release_object(struct object *pobj)
{
    --pobj->_ref;
    if (pobj->_ref <= 0)
    {
        _object.drop(pobj->_id);
        _alloc(_alloc_ud, pobj, sizeof(*pobj));
    }
}

void aoi_space::_copy_pos__(float dst[3], float sur[3])
{
    memcpy((char *)dst, (char *)sur, sizeof(float) * 3);
}

void *
aoi_space::_default_alloc__(void *ud, void *ptr, size_t sz)
{
    if (ptr == NULL)
    {
        void *p = malloc(sz);
        return p;
    }
    free(ptr);
    return NULL;
}
