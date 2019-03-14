#include "usocket_server.h"
#include <atomic.h>
#include <assert.h>
#include <ucall.h>
#include <string>

using namespace std;

namespace rednet
{
namespace network
{

struct request_base
{
    int socketId;
    uintptr_t opaque;
};

struct request_start
{
    struct request_base base;
};

struct request_open
{
    struct request_base base;
    int port;
    char host[1];
};

struct request_send
{
    int socketId;
    int sz;
    char *buffer;
};

struct request_close
{
    struct request_base base;
    int shutdown;
};

struct request_listen
{
    struct request_base base;
    SOCKET_HANDLE sock;
    char host[1];
};

struct request_bind
{
    struct request_base base;
    SOCKET_HANDLE sock;
};

struct request_packet
{
    int aaa[8];
    uint8_t header[8];
    union {
        char data[256];
        struct request_listen listen;
        struct request_close close;
        struct request_open open;
        struct request_start start;
        struct request_bind bind;
        struct request_send send;
    } u;
    char dummy[256];
};

} // namespace network
} // namespace rednet

using namespace rednet::network;

usocket_server::usocket_server()
{
#if defined(__linux__)
    ptrPoll__ = new uepoll();
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
    ptrPoll__ = new ukqueue();
#endif

    ptrChannel__ = new uchannel(ptrPoll__);

    for (int i = 0; i < NT_MAX_SOCKET; i++)
    {
        sockAry__[i].setParentPoll(ptrPoll__);
    }
}

usocket_server::~usocket_server()
{
    for (int i = 0; i < NT_MAX_SOCKET; i++)
    {
        usocket *s = &sockAry__[i];
        urecuresivelock l(&s->lock__);
        if (s->status__ != TE_RESERVE)
        {
            s->onClose(&l);
        }
    }

    if (ptrChannel__)
    {
        delete ptrChannel__;
        ptrChannel__ = NULL;
    }

    if (ptrPoll__)
    {
        delete ptrPoll__;
        ptrPoll__ = NULL;
    }
}

usocket *usocket_server::newSocket(int id, SOCKET_HANDLE sck, int protocol, uintptr_t opaque, bool isRegister)
{
    usocket *s = &sockAry__[NT_IDTOPOS(id)];
    assert(s->status__ == socketStatus::TE_RESERVE);
    if (isRegister)
    {
        if (!ptrPoll__->reg(sck, (void *)s))
        {
            s->status__ = socketStatus::TE_INVALID;
            return NULL;
        }
    }

    s->doInit(id, sck, protocol, opaque);
    return s;
}

void usocket_server::forwardSocket(int type, int socketId, uintptr_t opaque, int ud, const char *data)
{
    char *sndata = (char *)"";
    struct asynResult *rst;
    size_t sz = sizeof(*rst);
    if (data)
    {
        size_t msgSz = strlen(data);
        if (msgSz > NT_MAX_INFO)
        {
            msgSz = NT_MAX_INFO;
        }
        sz += msgSz;
        sndata = (char *)data;
    }

    rst = (struct asynResult *)umemory::malloc(sz);
    rst->type = type;
    rst->id = socketId;
    rst->ud = ud;

    rst->buffer = NULL;
    memcpy(rst + 1, sndata, sz - sizeof(*rst));

    struct uevent ev;
    ev.source = 0;
    ev.session = 0;
    ev.type = EP_SOCKET;
    ev.data = rst;
    ev.sz = sz;

    if (!ucall::sendEvent(opaque, &ev))
    {
        umemory::free((void *)rst);
    }
}

void usocket_server::forwardSocket(int type, int socketId, uintptr_t opaque, int ud, char *data)
{
    struct asynResult *rst;
    size_t sz = sizeof(*rst);

    rst = (struct asynResult *)umemory::malloc(sz);
    rst->type = type;
    rst->id = socketId;
    rst->ud = ud;
    rst->buffer = data;

    struct uevent ev;
    ev.source = 0;
    ev.session = 0;
    ev.type = EP_SOCKET;
    ev.data = rst;
    ev.sz = sz;

    if (!ucall::sendEvent(opaque, &ev))
    {
        umemory::free(rst->buffer);
        umemory::free((void *)rst);
    }
}

int usocket_server::operStartSocket(struct request_start *request)
{
    int socketId = request->base.socketId;
    usocket *s = &sockAry__[NT_IDTOPOS(socketId)];
    if (s->status__ == socketStatus::TE_INVALID ||
        s->id__ != socketId)
    {
        forwardSocket(NT_SOCKET_ERR,
                      socketId,
                      request->base.opaque,
                      0,
                      (const char *)"invalid socket");
        return NT_SOCKET_ERR;
    }

    urecuresivelock l(&s->lock__);
    if (s->status__ == socketStatus::TE_PACCEPT ||
        s->status__ == socketStatus::TE_PLISTEN)
    {
        if (!ptrPoll__->reg(s->sck__, s))
        {
            s->onClose(&l);
            forwardSocket(NT_SOCKET_ERR,
                          socketId,
                          request->base.opaque,
                          0,
                          (const char *)strerror(errno));
            return NT_SOCKET_ERR;
        }

        s->status__ = (s->status__ == socketStatus::TE_PACCEPT) ? socketStatus::TE_CONNECTED : socketStatus::TE_LISTEN;
        s->opaque__ = request->base.opaque;
        forwardSocket(NT_SOCKET_OPEN,
                      socketId,
                      s->opaque__,
                      0,
                      (const char *)"start");
        return NT_SOCKET_OPEN;
    }
    else if (s->status__ == socketStatus::TE_CONNECTED)
    {
        // TODO: maybe we should send a message SOCKET_TRANSFER to s->opaque
        s->opaque__ = request->base.opaque;
        forwardSocket(NT_SOCKET_OPEN,
                      socketId,
                      s->opaque__,
                      0,
                      (const char *)"transfer");
        return NT_SOCKET_OPEN;
    }
    return -1;
}

void usocket_server::forwardChannel(struct request_packet *request, char type, int len)
{
    request->header[6] = (uint8_t)type;
    request->header[7] = (uint8_t)len;
    ptrChannel__->forward((const char *)&request->header[6], len + 2);
}

int usocket_server::operOpenSocket(struct request_open *request)
{
    int status = 0;
    int socketId = request->base.socketId;
    uintptr_t opaque = request->base.opaque;

    usocket *ns;

    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    struct addrinfo *ai_ptr = NULL;

    char port[16];
    sprintf(port, "%d", request->port);
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    int sock = NT_INVALID;
    char *result = NULL;

    status = getaddrinfo(request->host, port, &ai_hints, &ai_list);
    if (status != 0)
    {
        result = (char *)gai_strerror(status);
        goto _failed;
    }

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next)
    {
        sock = ::socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0)
        {
            continue;
        }
        unetwork::setKeepalive(sock);
        unetwork::setNonblock(sock);
        status = connect(sock, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
        if (status != 0 && errno != EINPROGRESS)
        {
            ::close(sock);
            sock = -1;
            continue;
        }
        break;
    }

    if (sock < 0)
    {
        result = strerror(errno);
        goto _failed;
    }

    ns = newSocket(socketId, sock, PROTOCOL_TCP, opaque, true);
    if (ns == NULL)
    {
        close(sock);
        result = (char *)"reach socket number limit";
        goto _failed;
    }

    if (status == 0)
    {
        ns->status__ = socketStatus::TE_CONNECTED;
        struct sockaddr *addr = ai_ptr->ai_addr;
        void *sin_addr = (ai_ptr->ai_family == AF_INET) ? (void *)&((struct sockaddr_in *)addr)->sin_addr : (void *)&((struct sockaddr_in6 *)addr)->sin6_addr;

        if (inet_ntop(ai_ptr->ai_family, sin_addr, buffer__, sizeof(buffer__)))
        {
            result = buffer__;
        }
        freeaddrinfo(ai_list);
        forwardSocket(NT_SOCKET_OPEN, socketId, opaque, 0, (const char *)result);
        return NT_SOCKET_OPEN;
    }
    else
    {
        ns->status__ = socketStatus::TE_CONNECTING;
        ptrPoll__->toWrite(ns->sck__, ns, true);
    }

    freeaddrinfo(ai_list);
    return -1;
_failed:
    freeaddrinfo(ai_list);
    sockAry__[NT_IDTOPOS(socketId)].status__ = socketStatus::TE_INVALID;
    forwardSocket(NT_SOCKET_ERR, socketId, opaque, 0, (const char *)result);
    return NT_SOCKET_ERR;
}

int usocket_server::operCloseSocket(struct request_close *request)
{
    int socketId = request->base.socketId;
    usocket *s = &sockAry__[NT_IDTOPOS(socketId)];
    if (s->status__ == socketStatus::TE_INVALID ||
        s->id__ != socketId)
    {
        forwardSocket(NT_SOCKET_CLOSE,
                      socketId,
                      request->base.opaque,
                      0,
                      (char *)NULL);
        return NT_SOCKET_CLOSE;
    }

    urecuresivelock l(&s->lock__);
    if (!s->isForward())
    {
        int result = onForwardMessage(s, &l);
        if (result != -1 && result != NT_SOCKET_WARNING)
            return result;
    }

    if (request->shutdown ||
        s->isForward())
    {
        s->onClose(&l);
        forwardSocket(NT_SOCKET_CLOSE,
                      socketId,
                      request->base.opaque,
                      0,
                      (char *)NULL);
        return NT_SOCKET_CLOSE;
    }

    s->status__ = socketStatus::TE_HALFCLOSE;
    return -1;
}

int usocket_server::operBindSocket(struct request_bind *request)
{
    int socketId = request->base.socketId;
    usocket *s = newSocket(socketId, request->sock, PROTOCOL_TCP, request->base.opaque, true);
    if (s == NULL)
    {
        forwardSocket(NT_SOCKET_ERR,
                      socketId,
                      request->base.opaque,
                      0,
                      (const char *)"reach socket number limit");
        return NT_SOCKET_ERR;
    }

    unetwork::setNonblock(s->sck__);
    s->status__ = socketStatus::TE_BIND;
    forwardSocket(NT_SOCKET_OPEN,
                  socketId,
                  request->base.opaque,
                  0,
                  (const char *)"binded");
    return NT_SOCKET_OPEN;
}

int usocket_server::operListenSocket(struct request_listen *request)
{
    int socketId = request->base.socketId;
    SOCKET_HANDLE listenSocket = request->sock;
    usocket *s = newSocket(socketId, listenSocket, PROTOCOL_TCP, request->base.opaque);
    if (s == NULL)
    {
        goto _failed;
    }

    s->status__ = socketStatus::TE_PLISTEN;
    return -1;
_failed:
    ::close(listenSocket);
    sockAry__[NT_IDTOPOS(socketId)].status__ = socketStatus::TE_INVALID;
    forwardSocket(NT_SOCKET_ERR,
                  socketId,
                  request->base.opaque,
                  0,
                  (const char *)"reach socket number limit");
    return NT_SOCKET_ERR;
}

int usocket_server::operForwardSocket(struct request_send *request, const uint8_t *udpAddress)
{
    int socketId = request->socketId;
    usocket *s = &sockAry__[NT_IDTOPOS(socketId)];
    if (s->status__ == TE_INVALID ||
        s->id__ != socketId ||
        s->status__ == TE_HALFCLOSE ||
        s->status__ == TE_PACCEPT)
    {
        umemory::free(request->buffer); //TODO: free is
        return -1;
    }

    if (s->status__ == TE_PLISTEN ||
        s->status__ == TE_LISTEN)
    {
        fprintf(stderr, "socket-server: write to listen fd %d.\n", socketId);
        umemory::free(request->buffer); //TODO: free is
        return -1;
    }

    if (s->isForwardList() &&
        s->status__ == TE_CONNECTED)
    {
        if (s->protocol__ == PROTOCOL_TCP)
        {
            s->pushForwardList(request->buffer, request->sz);
        }
        else
        {
            /* dup */
        }
        ptrPoll__->toWrite(s->sck__, s, true);
    }
    else
    {
        if (s->protocol__ == PROTOCOL_TCP)
        {
            s->pushForwardList(request->buffer, request->sz);
        }
        else
        {
            /* udp */
        }
    }

    if (s->forwardSize__ >= NT_WARNING_SIZE && s->forwardSize__ >= s->forwardWarnSize__)
    {
        s->forwardWarnSize__ = s->forwardWarnSize__ == 0 ? NT_WARNING_SIZE * 2 : s->forwardWarnSize__ * 2;
        int warnsize = s->forwardWarnSize__ % 1024 ? s->forwardWarnSize__ / 1024 : s->forwardWarnSize__ / 1024 + 1;
        forwardSocket(NT_SOCKET_WARNING, socketId, s->opaque__, warnsize, (char *)NULL);
        return NT_SOCKET_WARNING;
    }

    return -1;
}

void usocket_server::operDecForwardSocket(int socketId)
{
    usocket *s = &sockAry__[NT_IDTOPOS(socketId)];
    if (s->id__ != socketId)
        return;
    s->decForwardRef();
}

bool usocket_server::isInvalid()
{
    return ptrChannel__ && ptrPoll__;
}

int usocket_server::onChannelRecv(uchannel *ch)
{
    uint8_t data[256];
    uint8_t header[2]; //0 command 1 bytes

    ptrChannel__->recv(header, sizeof(header));
    int command = header[0];
    int bytes = header[1];

    ptrChannel__->recv(data, bytes);

    return onChannelProc(command, (void *)data);
}

int usocket_server::onChannelProc(int cmd, void *data)
{
    switch (cmd)
    {
    case 'D':
    case 'P':
    {
        struct request_send *request = (struct request_send *)data;
        int ret = operForwardSocket(request, NULL);
        operDecForwardSocket(request->socketId);
        return ret;
    }
    case 'S':
        return operStartSocket((struct request_start *)data);
    case 'B':
        return operBindSocket((struct request_bind *)data);
    case 'L':
        return operListenSocket((struct request_listen *)data);
    case 'K':
        return operCloseSocket((struct request_close *)data);
    case 'X': //Exit Socket System
        return 0;
    default:
        fprintf(stderr, "socket: Unknown channel %c.\n", cmd);
        break;
    }
    return -1;
}

int usocket_server::reserveId()
{
    for (int i = 0; i < NT_MAX_SOCKET; i++)
    {
        int id = ATOM_INC(&sockAlloc__);
        if (id < 0)
        {
            id = ATOM_AND(&sockAlloc__, 0x7fffffff);
        }
        usocket *s = &sockAry__[NT_IDTOPOS(id)];
        if (s->status__ == socketStatus::TE_INVALID)
        {
            if (ATOM_CAS(&s->status__, socketStatus::TE_INVALID, socketStatus::TE_RESERVE))
            {
                s->id__ = id;
                s->protocol__ = PROTOCOL_UNKNOWN;
                //TODO::
                s->sck__ = NT_INVALID;
                return id;
            }
            else
            {
                // retry
                --i;
            }
        }
    }
    return -1;
}

int usocket_server::onWait()
{
    uintptr_t opaque = 0;
    int evIdx = 0, evNum = 0, socketId = -1;
    for (;;)
    {
        int nr = ptrChannel__->onWait(std::bind(&usocket_server::onChannelRecv, this, placeholders::_1));
        if (nr == NT_ERR) //需要继续处理 通道 命令/判断是否可以退出
            continue;
        else if (nr >= 0)
        {
            return nr;
        }
        /*====================================================================================*/
        if (evIdx == evNum)
        {
            evNum = ptrPoll__->onWait();
            evIdx = 0;
            ptrChannel__->rest();
            if (evNum <= 0)
            {
                evNum = 0;
                if (errno == EINTR)
                {
                    continue;
                }
                return NT_ERR; //出现错误
            }
        }
        /*====================================================================================*/
        struct pollEvent *e = ptrPoll__->getEvent(evIdx++);
        usocket *s = (usocket *)e->s;
        if (s == NULL)
            continue;
        //process accept/read/send/close
        urecuresivelock l(&s->lock__);

        switch (s->status__)
        {
        case socketStatus::TE_CONNECTING:
        {
            return onConnection(s, &l);
        }
        case socketStatus::TE_LISTEN:
        {
            int rok = onReportAccept(s);
            if (rok > 0)
            {
                return NT_SOCKET_ACCEPT;
            }
            if (rok < 0)
            {
                return NT_SOCKET_ERR;
            }
            break;
        }
        case socketStatus::TE_INVALID:
            fprintf(stderr, "usocket-server: invalid socket\n");
            break;
        default:
            if (e->isread)
            {
                int result = 0;
                if (s->protocol__ == PROTOCOL_TCP)
                {
                    result = onRecvMessage(s, &l);
                }
                else
                {
                    result = -1;
                }

                if (e->iswrite && result != NT_SOCKET_CLOSE && result != NT_SOCKET_ERR)
                {
                    e->isread = false;
                    --evIdx;
                }

                if (result == -1)
                    break;
                else if (result == NT_SOCKET_DATA)
                {
                    result = -1;
                }
                return result;
            }
            if (e->iswrite)
            {
                int result = onForwardMessage(s, &l);
                if (result == -1)
                    break;
                return result;
            }
            if (e->iserror)
            {
                int error;
                socklen_t len = sizeof(error);
                int code = getsockopt(s->sck__, SOL_SOCKET, SO_ERROR, &error, &len);
                const char *err = NULL;
                if (code < 0)
                {
                    err = strerror(errno);
                }
                else if (error != 0)
                {
                    err = strerror(error);
                }
                else
                {
                    err = "Unknown error";
                }
                socketId = s->id__;
                opaque = s->opaque__;

                s->onClose(&l);
                forwardSocket(NT_SOCKET_ERR, socketId, opaque, 0, err);
                return NT_SOCKET_ERR;
            }
            if (e->iseof)
            {
                socketId = s->id__;
                opaque = s->opaque__;
                forwardSocket(NT_SOCKET_CLOSE, socketId, opaque, 0, (const char *)NULL);
                s->onClose(&l);
                return NT_SOCKET_CLOSE;
            }
            break;
        }
    }
    return NT_ERR;
}

int usocket_server::onReportAccept(usocket *s)
{
    union sockAddrAll u;
    SOCKET_HANDLE client = s->onAccept(&u);
    if (client < 0)
    {
        if (errno == EMFILE || errno == ENFILE)
        {
            string fero = strerror(errno);
            forwardSocket(NT_SOCKET_ERR, s->sck__, s->opaque__, 0, (const char *)fero.c_str());
            return -1;
        }
        else
            return 0;
    }

    int clientSocketId = reserveId();
    if (clientSocketId < 0)
    {
        close(client);
        return 0;
    }

    unetwork::setKeepalive(client);
    unetwork::setNonblock(client);

    usocket *clientSocket = newSocket(clientSocketId, client, PROTOCOL_TCP, s->opaque__, false);
    if (clientSocket == NULL)
    {
        close(client);
        return 0;
    }

    char *infoData = NULL;
    char infoBuffer[NT_MAX_INFO];
    memset(infoBuffer, 0, NT_MAX_INFO);
    if (unetwork::getAddressInfo(&u, infoBuffer, sizeof(infoBuffer)))
    {
        infoData = infoBuffer;
    }

    clientSocket->status__ = socketStatus::TE_PACCEPT;
    forwardSocket(NT_SOCKET_ACCEPT, s->id__, s->opaque__, clientSocketId, (const char *)infoData);
    return 1;
}

int usocket_server::onConnection(usocket *s, urecuresivelock *sl)
{
    int error;
    char *result = NULL;
    int socketId = s->id__;
    uintptr_t opaque = s->opaque__;

    socklen_t len = sizeof(error);
    int code = getsockopt(s->sck__, SOL_SOCKET, SO_ERROR, &error, &len);
    if (code < 0 || error)
    {
        s->onClose(sl);
        if (code >= 0)
            result = strerror(error);
        else
            result = strerror(errno);
        forwardSocket(NT_SOCKET_ERR, socketId, opaque, 0, (const char *)result);
        return NT_SOCKET_ERR;
    }
    else
    {
        s->status__ = TE_CONNECTED;
        if (s->isForward())
        {
            ptrPoll__->toWrite(s->sck__, s, false);
        }

        union sockAddrAll u;
        socklen_t slen = sizeof(u);
        if (getpeername(s->sck__, &u.s, &slen) == 0)
        {
            void *sin_addr = (u.s.sa_family == AF_INET) ? (void *)&u.v4.sin_addr : (void *)&u.v6.sin6_addr;
            if (inet_ntop(u.s.sa_family, sin_addr, buffer__, sizeof(buffer__)))
            {
                result = buffer__;
            }
        }

        forwardSocket(NT_SOCKET_OPEN, socketId, opaque, 0, (const char *)result);
        return NT_SOCKET_OPEN;
    }
}

int usocket_server::onRecvMessage(usocket *s, urecuresivelock *sl)
{
    int socketId = s->id__;
    uintptr_t opaque = s->opaque__;

    int dataSz = 0;
    char *dataPtr = NULL;
    int r = s->onRecv(sl, &dataPtr, &dataSz);
    if (r == NT_SOCKET_DATA)
    {
        forwardSocket(NT_SOCKET_DATA, socketId, opaque, dataSz, dataPtr);
    }
    else if (r == NT_SOCKET_ERR || r == NT_SOCKET_CLOSE)
    {
        forwardSocket(r, socketId, opaque, 0, (const char *)strerror(errno));
    }

    return r;
}

int usocket_server::onForwardMessage(usocket *s, urecuresivelock *sl)
{
    int socketId = s->id__;
    uintptr_t opaque = s->opaque__;
    int r = s->onForward(sl);
    if (r == NT_SOCKET_CLOSE ||
        r == NT_WARNING_SIZE)
    {
        forwardSocket(r, socketId, opaque, 0, (const char *)NULL);
    }

    return r;
}

int usocket_server::socketListen(uintptr_t opaque, const char *addr, int port, int backlog)
{
    int sock = unetwork::createListen(addr, port, backlog);
    if (sock < 0)
    {
        return -1;
    }
    struct request_packet request;
    int socketId = INST(usocket_server, reserveId);
    if (socketId < 0)
    {
        ::close(socketId);
        return socketId;
    }

    request.u.listen.base.opaque = opaque;
    request.u.listen.base.socketId = socketId;
    request.u.listen.sock = sock;

    INST(usocket_server, forwardChannel, &request, 'L', sizeof(request.u.listen));

    return socketId;
}

void usocket_server::socketStart(uintptr_t opaque, int socketId)
{
    struct request_packet request;
    request.u.start.base.socketId = socketId;
    request.u.start.base.opaque = opaque;

    INST(usocket_server, forwardChannel, &request, 'S', sizeof(request.u.start));
}

int usocket_server::socketBind(uintptr_t opaque, SOCKET_HANDLE sock)
{
    struct request_packet request;
    int socketId = INST(usocket_server, reserveId);
    if (socketId < 0)
        return -1;
    request.u.bind.base.opaque = opaque;
    request.u.bind.base.socketId = socketId;
    request.u.bind.sock = sock;
    INST(usocket_server, forwardChannel, &request, 'B', sizeof(request.u.bind));
    return socketId;
}

int usocket_server::socketSend(int socketId, const void *buffer, int sz)
{
    usocket *s = &INSTGET(usocket_server)->sockAry__[NT_IDTOPOS(socketId)];
    if (s->id__ != socketId ||
        s->status__ == TE_INVALID)
    {
        umemory::free((void *)buffer); //TODO: is free
        return -1;
    }

    urecuresivelock l(&s->lock__);
    if (s->isCanDirectWrite(socketId) && l.trylock())
    {
        if (s->isCanDirectWrite(socketId))
        {
            s->onForwardData((char *)buffer, sz);
            l.unlock();
            return 0;
        }
        l.unlock();
    }

    s->incForwardRef();

    struct request_packet request;
    request.u.send.socketId = socketId;
    request.u.send.sz = sz;
    request.u.send.buffer = (char *)buffer;

    INST(usocket_server, forwardChannel, &request, 'D', sizeof(request.u.send));

    return 0;
}

void usocket_server::socketClose(uintptr_t opaque, int socketId)
{
    struct request_packet request;
    request.u.close.base.socketId = socketId;
    request.u.close.base.opaque = opaque;
    request.u.close.shutdown = 0;

    INST(usocket_server, forwardChannel, &request, 'K', sizeof(request.u.close));
}

void usocket_server::socketShutdown(uintptr_t opaque, int socketId)
{
    struct request_packet request;
    request.u.close.base.socketId = socketId;
    request.u.close.base.opaque = opaque;
    request.u.close.shutdown = 1;

    INST(usocket_server, forwardChannel, &request, 'K', sizeof(request.u.close));
}

void usocket_server::socketExit()
{
    struct request_packet request;
    INST(usocket_server, forwardChannel, &request, 'X', 0);
}