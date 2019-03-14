#ifndef REDNET_UEPOLL_H
#define REDNET_UEPOLL_H

#include "ipoll.h"

#ifdef __linux__

namespace rednet
{
namespace network
{
class uepoll : public ipoll
{
public:
  uepoll();
  ~uepoll();

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
