#ifndef REDNET_UNETDRIVE_H
#define REDNET_UNNEDRIVE_H

#include "uobject.h"
#include "network/unetwork.h"
#include <unordered_map>
#include <vector>
#include <functional>

#define SOCKET_BUFFER_MAX 4096
#define SOCKET_PACK_MAX (SOCKET_BUFFER_MAX >> 1)

using namespace std;
using namespace rednet::network;

namespace rednet
{

class unetsocket;
typedef function<void(struct asynResult *result)> netAsynCallback;
typedef function<void(int socketId)> netAsynRecv;

struct socketBuffer
{
  char data[SOCKET_BUFFER_MAX];
  uint32_t sz;
};

class unetsocket : public uobject
{
  friend class unetdrive;

public:
  unetsocket()
  {
  }
  virtual ~unetsocket() {}

public:
  int socketId;
  bool connected;
  struct socketBuffer buffer;
  int protocol;
  netAsynRecv onRecv;
  netAsynCallback onCallback;
  netAsynCallback onWarning;
};

class unetdrive : public uobject
{
public:
  unetdrive(uintptr_t opaque);
  virtual ~unetdrive();

  int beginListen(const char *address);
  int beginListen(const char *host, const int port, const int backlog);
  void beginAccept(int socketId, netAsynCallback asynFun);
  int beginRecv(int socketId, netAsynRecv asynRecv);
  void endAccept(int socketId, netAsynCallback asynFun);
  void setWarning(int socketId, netAsynCallback asynWaring);

  const char *check(int socketId, int &outSz);
  int clone(int socketId);
  int read(int socketId, char *outBuffer, const int maxBuffer);
  int send(int socketId, const char *inBuffer, const int inSz);
  int size();

  bool invalid(int socketId);
  void close(int socketId);
  void shutdown(int socketId);
  void abandon(int socketId);

public:
  //event is process
  inline int onEvent(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
  {
    struct asynResult *result = (struct asynResult *)data;
    switch (result->type)
    {
    case NT_SOCKET_ACCEPT:
      onAccept(result);
      break;
    case NT_SOCKET_DATA:
      onData(result);
      break;
    case NT_SOCKET_OPEN:
      onOpen(result);
      break;
    case NT_SOCKET_ERR:
      onError(result);
      break;
    case NT_SOCKET_CLOSE:
      onClose(result);
      break;
    case NT_SOCKET_UDP:
      break;
    case NT_SOCKET_WARNING:
      onWarn(result);
      break;
    default:
      fprintf(stderr, "DEFAULT\n");
      break;
    }

    if (result && result->buffer)
    {
      umemory::free(result->buffer);
      result->buffer = NULL;
    }

    return 0;
  }

protected:
  void onClose(struct asynResult *r);

  void onError(struct asynResult *r);

  void onOpen(struct asynResult *r);

  void onData(struct asynResult *r);

  void onAccept(struct asynResult *r);

  void onWarn(struct asynResult *r);

protected:
  int doListen(const char *host, const int port, const int backlog);

protected:
  unetsocket *grab(int socketId);
  void insert(unetsocket *s);
  void earse(int socketId);

protected:
  uintptr_t opaque__;
  unordered_map<int, unetsocket> netSocketMap__; //需要考虑同步问题,考虑引用计数器
};

} // namespace rednet

#endif
