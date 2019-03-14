#ifndef REDNET_USIGNAL_H
#define REDNET_USIGNAL_H

#include <functional>
#include <pthread.h>
#include <signal.h>

using namespace std;
namespace rednet
{
typedef function<void(void)> signalFun;
class usignal
{
  public:
    usignal(int flags, int sig, signalFun cb);
    ~usignal();

    void wait();

  private:
    int sig__;
    struct sigaction sa__;
    signalFun cb__;
};
} // namespace rednet

#endif
