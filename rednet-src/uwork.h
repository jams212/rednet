#ifndef REDNET_UWORKER_H
#define REDNET_UWORKER_H

#include "uobject.h"
#include "usingleton.h"

#include <functional>
#include <pthread.h>
#include <ucontext.h>
#include <vector>

using namespace std;
namespace rednet
{

typedef function<int(void *parm)> uwork_func;
class uworker;
struct uworker_param
{
  uworker *w;
  uwork_func f;
  void *p;
};

class uworker
{
public:
  uworker(uwork_func, void *parm);
  ~uworker();

  void join();

private:
  static void *__run__(void *parm);

protected:
  pthread_t t__;
};

class uworks : public usingleton<uworks>
{
public:
  uworks();
  ~uworks();

  void stop();

  void append(uwork_func f, void *parm);

  void wakeup(int busy);

  void wait();

  void join();

  inline int getCount() { return count__; }

  inline bool isShutdown() { return (bool)shutdown__; }

private:
  int count__;
  int sleep__;
  int shutdown__;

  vector<uworker *> ws__;

  pthread_cond_t cd__;
  pthread_mutex_t tx__;
};

} // namespace rednet

#endif