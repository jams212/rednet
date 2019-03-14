#ifndef REDNET_UKQUEUE_H
#define REDNET_UKQUEUE_H

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

#include "ipoll.h"

namespace rednet
{
namespace network
{
class ukqueue : public ipoll
{
public:
  ukqueue();
  ~ukqueue();

  bool reg(int sock, void *ud) override;
  void unReg(int sock) override;
  void toWrite(int sock, void *ud, bool enable) override;

  int onWait() override;

protected:
  POLL_HANDLE initial() override;
  void destory() override;
};
} // namespace network
} // namespace rednet

#endif

#endif