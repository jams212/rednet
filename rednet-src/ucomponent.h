#ifndef REDNET_UCOMPONENT_H
#define REDNET_UCOMPONENT_H

#include "macro.h"
#include "uobject.h"
#include <functional>

using namespace std;
namespace rednet
{
class ucontext;
typedef function<int(void* ud, int type, uint32_t source, int session, void *data, size_t sz)> componentFun;
class ucomponent : public uobject
{
  friend class ucontext;

public:
  ucomponent();
  virtual ~ucomponent();

  virtual int init(ucontext *ctx, const char *parm);

  virtual int signal(int signal);

protected:
  ucontext *parent__;
  componentFun callback__;
  void *ud__;
};
} // namespace rednet

#endif