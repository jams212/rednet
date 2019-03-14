#ifndef REDNET_IPOLL_H
#define REDNET_IPOLL_H

#include "unetwork.h"
#include <assert.h>

namespace rednet
{
namespace network
{

struct pollEvent
{
  void *s;
  bool isread;
  bool iswrite;
  bool iserror;
  bool iseof;
};

class ipoll
{
public:
  virtual ~ipoll() {}

  inline struct pollEvent *getEvent(int32_t idx)
  {
    assert(idx >= 0 && idx < NT_POLL_EVENT_MAX);
    return &evAry__[idx];
  }

  virtual bool reg(int sock, void *ud) = 0;
  virtual void unReg(int sock) = 0;
  virtual void toWrite(int sock, void *ud, bool enable) = 0;

  virtual int onWait() = 0;

protected:
  virtual POLL_HANDLE initial() = 0;
  virtual void destory() = 0;

protected:
  struct pollEvent evAry__[NT_POLL_EVENT_MAX];
  POLL_HANDLE handle__;
};

} // namespace network
} // namespace rednet

#endif
