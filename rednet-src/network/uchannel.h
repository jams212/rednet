#ifndef REDNET_UCHANNEL_H
#define REDNET_UCHANNEL_H

#include <functional>
#include <sys/select.h>
#include <sys/time.h>

using namespace std;
namespace rednet
{
namespace network
{
class ipoll;
class uchannel
{
public:
  uchannel(ipoll *ptr);
  ~uchannel();

  bool isInvalid();

  void forward(const char *data, const size_t bytes);

  void recv(void *buffer, const size_t bytes);

  void rest();

  int onWait(function<int(uchannel *h)> onRecvSync);

private:
  bool isRead();

private:
  int rCtrl__;
  int sCtrl__;
  int cCtrl__;
  fd_set rsfds__;

private:
  ipoll *ptrPoll__;
};
} // namespace network
} // namespace rednet

#endif