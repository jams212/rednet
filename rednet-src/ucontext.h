#ifndef REDNET_UCONTEXT_H
#define REDNET_UCONTEXT_H

#include "macro.h"
#include "ucomponent.h"
#include "uevent_queue.h"
#include "umodule.h"
#include "uobject.h"
#include "urwlock.h"
#include "usingleton.h"
#include <memory>
#include <unordered_map>
#include <vector>

#define ID_MARK 0xffffff
#define ID_REMOTE_SHIFT 24

using namespace std;
namespace rednet
{

class ucontext : public uobject
{
  friend class ucall;
public:
  ucontext();
  virtual ~ucontext();

  uint32_t init(const char *name, const char *parm);

  void execute(uevent *e);

  int32_t session();

  void setCallback(componentFun cb, void *ud)
  {
    cp__->callback__ = cb;
    cp__->ud__ = ud;
  };

  void isAssert(uevent_queue *p) { assert(p == eq.get()); }

  void disponseAll();

  const char *self();

  static void error(uint32_t source, const char *msg, ...);

public:
  uint32_t id;
  eq_ptr eq;
  char result[64];

protected:
  ucomponent *cp__;
  umodule *md__;
  int32_t session_id__;
  uint64_t cpu_cost__;
  uint64_t cpu_start__;
  int32_t event_count__;
  bool profile__;

  bool inited__;
};

typedef shared_ptr<ucontext> ctx_ptr;
class uctxMgr : public usingleton<uctxMgr>
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
  typedef unordered_map<const char *, uint32_t, str_map, str_cmp> umaps;

public:
  uctxMgr();
  ~uctxMgr();

  void init(int serverGroupID);

  uint32_t registerId(ucontext *ctx);

  const char *registerName(uint32_t id, const char *name);

  bool unRegister(uint32_t id);

  ctx_ptr grab(uint32_t id);

  uint32_t convertId(const char *name);

  const char *convertName(uint32_t id);

  void retireAll();

private:
  inline uint32_t local_addr(uint32_t id)
  {
    return id & (ctxsCap__ - 1);
  }

  void local_erase_name(uint32_t id);

private:
  uint32_t serverGroupID__;
  vector<ctx_ptr> ctxs__;
  umaps ctxsName__;
  int32_t ctxsSer__;
  int32_t ctxsCap__;
  urwlock ctxsLck__;
};

} // namespace rednet

#endif