#ifndef REDNET_NETWORK_H
#define REDNET_NETWORK_H

#define NT_POLL_EVENT_MAX 64
#define NT_MAX_INFO 128
#define NT_MAX_SOCKET_P 16
#define NT_MAX_SOCKET (1 << NT_MAX_SOCKET_P)
#define NT_UDP_ADDRESS_SIZE 19

#define POLL_HANDLE int
#define SOCKET_HANDLE int

#define NT_INVALID -1
#define NT_ERR -1
#define NT_CONTINUE_ERR -2
#define NT_IS_INVALID(p) ((p == NT_INVALID) ? false : true)

#define NT_IDTOPOS(id) (((unsigned)id) % NT_MAX_SOCKET)
#define NT_ID_TAG16(id) ((id >> NT_MAX_SOCKET_P) & 0xffff)

#define NT_SOCKET_DATA 0
#define NT_SOCKET_CLOSE 1
#define NT_SOCKET_OPEN 2
#define NT_SOCKET_ACCEPT 3
#define NT_SOCKET_ERR 4
#define NT_SOCKET_EXIT 5
#define NT_SOCKET_UDP 6
#define NT_SOCKET_WARNING 7

#define NT_MIN_READ_BUFFER 64

#define NT_WARNING_SIZE (1024 * 1024)

#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1
#define PROTOCOL_UDPv6 2
#define PROTOCOL_UNKNOWN 255

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__linux__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/event.h>
#elif defined __linux__
#include <sys/epoll.h>
#endif

#include <stdint.h>
#include <functional>

// EAGAIN and EWOULDBLOCK may be not the same value.
#if (EAGAIN != EWOULDBLOCK)
#define AGAIN_WOULDBLOCK \
    EAGAIN:              \
    case EWOULDBLOCK
#else
#define AGAIN_WOULDBLOCK EAGAIN
#endif

/*#define SOCKET_TYPE_INVALID 0
#define SOCKET_TYPE_RESERVE 1
#define SOCKET_TYPE_PLISTEN 2
#define SOCKET_TYPE_LISTEN 3
#define SOCKET_TYPE_CONNECTING 4
#define SOCKET_TYPE_CONNECTED 5
#define SOCKET_TYPE_HALFCLOSE 6
#define SOCKET_TYPE_PACCEPT 7
#define SOCKET_TYPE_BIND 8*/

namespace rednet
{
namespace network
{

struct socketInfo
{
    uint64_t rvTime;
    uint64_t wrTime;
    uint64_t rvBytes;
    uint64_t wrBytes;
};

enum socketEvent
{
    SE_START = 0,
    SE_OPEN = 1,
    SE_LISTEN = 2,
    SE_CLOSE = 3,
};

enum socketStatus
{
    TE_INVALID = 0,    //未被分配
    TE_RESERVE = 1,    //已被分配
    TE_PLISTEN = 2,    //UDP监听
    TE_LISTEN = 3,     //TCP监听
    TE_CONNECTING = 4, //连接中
    TE_CONNECTED = 5,  //已经连接
    TE_HALFCLOSE = 6,  //处于关闭状态
    TE_PACCEPT = 7,    //接受得连接
    TE_BIND = 8,       //Bind状态
};

struct asynResult
{
    int type;
    int id;
    int ud; // for accept, ud is new connection id ; for data, ud is size of data
    char *buffer;
};

union sockAddrAll {
    struct sockaddr s;
    struct sockaddr_in v4;
    struct sockaddr_in6 v6;
};

struct writeBuffer
{
    void *buffer;
    char *ptr;
    int sz;
};

class unetwork
{
  public:
    static void setNonblock(int sock);

    static void setKeepalive(int sock);

    static bool getAddressInfo(union sockAddrAll *u, char *buffer, size_t sz);

    static void freeWriteBuffer(struct writeBuffer *p);

    static int createListen(const char *addr, int port, int backlog);

  private:
    static int doBind(const char *host, int port, int protocol, int *family);
};

} // namespace network
} // namespace rednet

#define TCP_BUFFER_SIZE sizeof(struct writeBuffer)

#endif
