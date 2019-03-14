#include "uepoll.h"

#ifdef __linux__

using namespace rednet::network;

uepoll::uepoll()
{
    handle__ = initial();
}

uepoll::~uepoll()
{
    destory();
}

bool uepoll::reg(int sock, void *ud)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = ud;
    if (epoll_ctl(handle__, EPOLL_CTL_ADD, sock, &ev) == -1)
        return false;
    return true;
}

void uepoll::unReg(int sock)
{
    epoll_ctl(handle__, EPOLL_CTL_DEL, sock, NULL);
}

void uepoll::toWrite(int sock, void *ud, bool enable)
{
    struct epoll_event ev;
    ev.events = EPOLLIN | (enable ? EPOLLOUT : 0);
    ev.data.ptr = ud;
    epoll_ctl(handle__, EPOLL_CTL_MOD, sock, &ev);
}

int uepoll::onWait()
{
    struct epoll_event ev[NT_POLL_EVENT_MAX];
    int n = epoll_wait(handle__, ev, NT_POLL_EVENT_MAX, -1);
    if (n <= 0)
    {
        if (errno == EINTR)
            return -2;
        return -1;
    }

    int i;
    for (i = 0; i < n; i++)
    {
        evAry__[i].s = ev[i].data.ptr;
        unsigned flag = ev[i].events;
        evAry__[i].iswrite = (flag & EPOLLOUT) != 0;
        evAry__[i].isread = (flag & (EPOLLIN | EPOLLHUP)) != 0;
        evAry__[i].iserror = (flag & EPOLLERR) != 0;
        evAry__[i].iseof = false;
    }
    return n;
}

POLL_HANDLE uepoll::initial()
{
    return epoll_create(1024);
}

void uepoll::destory()
{
    close(handle__);
    handle__ = -1;
}

#endif