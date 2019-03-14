#ifndef REDNET_USOCKET_SERVER_H
#define REDNET_USOCKET_SERVER_H

#include "uchannel.h"
#include "upoll.h"
#include "usocket.h"
#include <usingleton.h>

namespace rednet
{
namespace network
{

class usocket_server : public usingleton<usocket_server>
{
public:
  static int socketListen(uintptr_t opaque, const char *addr, int port, int backlog);
  static void socketStart(uintptr_t opaque, int socketId);
  static int socketBind(uintptr_t opaque, SOCKET_HANDLE sock);
  static int socketSend(int socketId, const void *buffer, int sz);
  static void socketClose(uintptr_t opaque, int socketId);
  static void socketShutdown(uintptr_t opaque, int socketId);
  static void socketExit();

public:
  usocket_server();
  ~usocket_server();

  bool isInvalid();

  int reserveId();

  int onWait();

  void onUpdateTime(uint64_t nowTm) { nowTime__ = nowTm; }
  uint64_t getNowTime() { return nowTime__; }

private:
  int onChannelRecv(uchannel *ch);
  int onChannelProc(int cmd, void *data);

private:
  int operStartSocket(struct request_start *request);
  int operOpenSocket(struct request_open *request);
  int operCloseSocket(struct request_close *request);
  int operBindSocket(struct request_bind *request);
  int operListenSocket(struct request_listen *request);
  int operForwardSocket(struct request_send *request, const uint8_t *udpAddress);
  void operDecForwardSocket(int socketId);

private:
  usocket *newSocket(int id, SOCKET_HANDLE sck, int protocol, uintptr_t opaque, bool isRegister = false);
  void forwardSocket(int type, int socketId, uintptr_t opaque, int ud, char *data);
  void forwardSocket(int type, int socketId, uintptr_t opaque, int ud, const char *data);
  void forwardChannel(struct request_packet *request, char type, int len);

private:
  int onReportAccept(usocket *s);
  int onConnection(usocket *s, urecuresivelock *sl);
  int onRecvMessage(usocket *s, urecuresivelock *sl);
  int onForwardMessage(usocket *s, urecuresivelock *sl);

private:
  uchannel *ptrChannel__;
  ipoll *ptrPoll__;
  uint64_t nowTime__;
  usocket sockAry__[NT_MAX_SOCKET];
  int sockAlloc__;
  char buffer__[NT_MAX_INFO];
};

} // namespace network
} // namespace rednet

#endif
