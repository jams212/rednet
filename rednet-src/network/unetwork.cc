#include "unetwork.h"
#include <umemory.h>
#include <string.h>
#include <assert.h>

using namespace rednet::network;

void unetwork::setNonblock(int sock)
{
    int flag = fcntl(sock, F_GETFL, 0);
    if (-1 == flag)
        return;
    fcntl(sock, F_SETFL, flag | O_NONBLOCK);
}

void unetwork::setKeepalive(int sock)
{
    int keepalive = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepalive, sizeof(keepalive));
}

bool unetwork::getAddressInfo(union sockAddrAll *u, char *buffer, size_t sz)
{
    char tmp[INET6_ADDRSTRLEN];
    void *sin_addr = (u->s.sa_family == AF_INET) ? (void *)&u->v4.sin_addr : (void *)&u->v6.sin6_addr;
    int sin_port = ntohs((u->s.sa_family == AF_INET) ? u->v4.sin_port : u->v6.sin6_port);
    if (inet_ntop(u->s.sa_family, sin_addr, tmp, sizeof(tmp)))
    {
        snprintf(buffer, sz, "%s:%d", tmp, sin_port);
        return true;
    }
    else
    {
        buffer[0] = '\0';
        return false;
    }
}

int unetwork::createListen(const char *addr, int port, int backlog)
{
    int family = 0;
    int listenSock = unetwork::doBind(addr, port, IPPROTO_TCP, &family);
    if (listenSock < 0)
    {
        return -1;
    }

    if (listen(listenSock, backlog) == -1)
    {
        close(listenSock);
        return -1;
    }
    return listenSock;
}

void unetwork::freeWriteBuffer(struct writeBuffer *p)
{
    umemory::free(p->buffer);
    umemory::free(p);
}

int unetwork::doBind(const char *host, int port, int protocol, int *family)
{
    int sock;
    int status;
    int reuse = 1;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    char portstr[16];
    if (host == NULL || host[0] == 0)
    {
        host = "0.0.0.0"; // INADDR_ANY
    }
    sprintf(portstr, "%d", port);
    memset(&ai_hints, 0, sizeof(ai_hints));

    ai_hints.ai_family = AF_UNSPEC;
    if (protocol == IPPROTO_TCP)
    {
        ai_hints.ai_socktype = SOCK_STREAM;
    }
    else
    {
        assert(protocol == IPPROTO_UDP);
        ai_hints.ai_socktype = SOCK_DGRAM;
    }
    ai_hints.ai_protocol = protocol;

    status = ::getaddrinfo(host, portstr, &ai_hints, &ai_list);
    if (status != 0)
    {
        return -1;
    }

    *family = ai_list->ai_family;
    sock = ::socket(*family, ai_list->ai_socktype, 0);
    if (sock < 0)
    {
        goto _failed_fd;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(int)) == -1)
    {
        goto _failed;
    }
    status = bind(sock, (struct sockaddr *)ai_list->ai_addr, ai_list->ai_addrlen);
    if (status != 0)
        goto _failed;

    freeaddrinfo(ai_list);
    return sock;
_failed:
    close(sock);
_failed_fd:
    freeaddrinfo(ai_list);
    return -1;
}