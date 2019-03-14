#ifndef REDNET_UREDNET_H
#define REDNET_UREDNET_H

#define REDNET_VERSION "2.0.0 bate"

#include "uevent_queue.h"
#include "usingleton.h"

namespace rednet
{

//class utimer;
//class uworks;
//class uenv;
//class ucontexts;
//class umodules;
//class usignal;
//class uglobal_queue;

class usignal;

namespace server
{
class ugroup;
}

class uframework : public usingleton<uframework>
{
  struct deploy
  {
    int workerNumber;
    int serverGroupID;
    const char *daemon;
    const char *modulePath;
    const char *bootstrap;
    const char *logger;
    const char *luaPath;
    const char *luaCPath;
    const char *luaService;
    bool proFile;
  };

  enum workerSerial
  {
    WS_WORKER = 0,
    WS_MAIN = 1,
    WS_TIMER = 2,
    WS_SOCKET = 3,
  };

  friend class uenv;
  friend class ucall;
  friend class utimer;
  friend class ucontext;
  friend class uevent_queue;

public:
  uframework();
  ~uframework();

  void startup(const char *file);

  struct deploy *getDeploy() { return &dep__; }

private:
  void init();
  bool loadDeploy(const char *filename);
  bool openService(const char *cmdline, ucontext **out);
  void bootstrap(const char *logparm, const char *cmdline);
  void destory();

private:
  eq_ptr onDispatchEvent(eq_ptr q, int weight);

private:
  int onWorkTimer(void *param);
  int onWorkSocket(void *param);
  int onWorkLogic(void *param);
  void onSignal();

private:
  usignal *sig__;
  deploy dep__;
};

} // namespace rednet

#endif