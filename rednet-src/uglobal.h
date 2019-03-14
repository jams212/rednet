#ifndef REDNET_GLOBAL_H
#define REDNET_GLOBAL_H

#include "uobject.h"
#include "usingleton.h"
#include <pthread.h>
#include <stdint.h>

namespace rednet
{
class uglobal : public usingleton<uglobal>
{
public:
  uglobal();
  ~uglobal();

  void init();

  void exit();

  uint32_t currentContext();

  void bindWorker(int m);

  void contextInc();
  void contextDec();
  bool contextEmpty();

  void setProFile(bool profile) { proFile__ = profile; }
  bool getProFile() { return proFile__; }

private:
  pthread_key_t contextKey__;
  uint32_t contextTotal__;
  bool proFile__;
  bool systemInited__;
};

} // namespace rednet

#endif