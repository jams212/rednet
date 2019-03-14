#ifndef REDNET_USOCKET_H
#define REDNET_USOCKET_H

#include "unetwork.h"
#include <uobject.h>
#include <urecursivelock.h>
#include <deque>

using namespace std;

namespace rednet
{
namespace network
{

class ipoll;
class usocket : public uobject
{
  friend class usocket_server;

public:
  usocket();
  virtual ~usocket();

  void setParentPoll(ipoll *value) { parentPoll__ = value; }
  int onAccept(union sockAddrAll *u);
  int onForward(urecuresivelock *rl);
  int onForwardData(char *inBuffer, int inSz);
  int onRecv(urecuresivelock *rl, char **outBuffer, int *outSz);
  void onClose(urecuresivelock *rl);

private:
  void doInit(int socketId, SOCKET_HANDLE sck, int protocol, uintptr_t opaque);
  void doForwardListClear();
  //----------------------------------------------------------------
  bool isForward();
  bool isForwardList();
  bool isCanDirectWrite(int socketId);
  void incForwardRef();
  void decForwardRef();
  //----------------------------------------------------------------
  int beginForward();
  int beginForwardList();
  int beginForwardTCP();
  //----------------------------------------------------------------
  void pushForwardList(const char *buffer, int sz);

private:
  int id__;
  uintptr_t opaque__;
  SOCKET_HANDLE sck__;
  /*==========================*/
  volatile uint32_t forwardIn__;
  int forwardBufferOffset__;
  const void *forwardBuffer__;
  size_t forwardBufferSz__;
  /*=========================*/
  deque<struct writeBuffer *> forwardList__;
  int64_t forwardSize__;
  int64_t forwardWarnSize__;
  /*=========================*/
  union {
    int size;
    uint8_t udpAddress;
  } p__;
  /*========================*/
  uint8_t protocol__;
  uint8_t status__;
  struct socketInfo info__;
  /*========================*/
  ipoll *parentPoll__;
  uspinlock lock__;
};

} // namespace network
} // namespace rednet

#endif