

#ifndef AOI_H
#define AOI_H

#include <stddef.h>
#include <stdint.h>

#include "map.h"
#include "set.h"

class aoi_space
{
    struct object
    {
        int _ref;
        uint32_t _id;
        int _version;
        int _mode;
        float _last[3];
        float _position[3];
    };

    struct pair_list
    {
        pair_list *_next;
        struct object *_watcher;
        struct object *_marker;
        int _watcher_version;
        int _marker_version;
    };

  public:
    aoi_space(void *ud, aoi::Alloc al);
    aoi_space();
    ~aoi_space();

    void update(uint32_t id, const char *modestr, float pos[3]);

    void message(aoi::CallBack cb, void *ud);

  private:
    static void *_default_alloc__(void *ud, void *ptr, size_t sz);
    static inline struct object *_alloc_object__(void *ud, uint32_t id)
    {
        aoi_space *space = static_cast<aoi_space *>(ud);
        struct object *obj = (struct object *)space->_alloc(space->_alloc_ud, NULL, sizeof(*obj));
        obj->_ref = 1;
        obj->_id = id;
        obj->_version = 0;
        obj->_mode = 0;
        return obj;
    }

    static inline void _delete_object__(void *ud, struct object *pobj)
    {
        aoi_space *space = static_cast<aoi_space *>(ud);
        space->_alloc(space->_alloc_ud, pobj, sizeof(*pobj));
    }

    static void _grab_object__(struct object *pobj);
    static bool _change_mode__(struct object *pobj, bool set_watcher, bool set_marker);
    static void _copy_pos__(float dst[3], float sur[3]);
    static void _set_push__(void *s, struct object *pobj);

    void _flush_pair(aoi::CallBack cb, void *ud);
    void _gen_pair_list(set<object> *watcher, set<object> *marker, aoi::CallBack cb, void *ud);
    void _gen_pair(struct object *watcher, struct object *marker, aoi::CallBack cb, void *ud);
    void _delete_pair_list();

    void _drop_pair(struct pair_list *p);
    void _release_object(struct object *pobj);

  private:
    map<object> _object;
    set<object> _watcher_static;
    set<object> _marker_static;
    set<object> _watcher_move;
    set<object> _marker_move;
    struct pair_list *_hot;

    aoi::Alloc _alloc;
    void *_alloc_ud;

    float _radis;
};

#endif
