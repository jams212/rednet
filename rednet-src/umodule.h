#ifndef REDNET_UMODULE_H
#define REDNET_UMODULE_H

#include "uobject.h"
#include "urwlock.h"
#include "usingleton.h"
#include <stdlib.h>
#include <string.h>
#include <unordered_map>

using namespace std;
namespace rednet
{
class ucomponent;
class umodule : public uobject
{
  friend class ucontext;
  typedef ucomponent *(*ucomponent_dl_create)(void);
  typedef void (*ucomponent_dl_release)(ucomponent *comp);

public:
  umodule(void *plib) : lib__(plib), create_cb__(NULL), release_cb__(NULL) {}
  ~umodule()
  {
    lib__ = NULL;
  }

  bool open(const char *name);

private:
  void *get_api(const char *name, const char *api_name);

private:
  void *lib__;
  ucomponent_dl_create create_cb__;
  ucomponent_dl_release release_cb__;
};

class umodMgr : public usingleton<umodMgr>
{
  struct str_cmp
  {
    bool operator()(const char *a, const char *b) const
    {
      return strcmp(a, b) == 0;
    }
  };

  struct str_map
  {
    int operator()(const char *str) const
    {
      int seed = 131;
      int hash = 0;
      while (*str)
      {
        hash = (hash * seed) + (*str);
        str++;
      }
      return hash & (0x7FFFFFFF);
    }
  };
  typedef unordered_map<const char *, umodule *, str_map, str_cmp> unmaps;

public:
  umodMgr();
  ~umodMgr();

  void init(const char *cpath);

  umodule *query(const char *name);

private:
  umodule *local_query(const char *name);
  umodule *local_register(const char *name);
  void *local_opendl(const char *name);
  void local_clear();

private:
  unmaps mods__;
  urwlock modk__;
  const char *modp__;
};

} // namespace rednet

#endif