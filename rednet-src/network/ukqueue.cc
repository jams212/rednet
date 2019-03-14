#include "ukqueue.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

using namespace rednet::network;

ukqueue::ukqueue()
{
    handle__ = initial();
}

ukqueue::~ukqueue()
{
    destory();
}

bool ukqueue::reg(int sock, void *ud)
{
    struct kevent ke;
    EV_SET(&ke, sock, EVFILT_READ, EV_ADD, 0, 0, ud);
    if (kevent(handle__, &ke, 1, NULL, 0, NULL) == -1 || ke.flags & EV_ERROR)
    {
        return false;
    }
    EV_SET(&ke, sock, EVFILT_WRITE, EV_ADD, 0, 0, ud);
    if (kevent(handle__, &ke, 1, NULL, 0, NULL) == -1 || ke.flags & EV_ERROR)
    {
        EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        kevent(handle__, &ke, 1, NULL, 0, NULL);
        return false;
    }
    EV_SET(&ke, sock, EVFILT_WRITE, EV_DISABLE, 0, 0, ud);
    if (kevent(handle__, &ke, 1, NULL, 0, NULL) == -1 || ke.flags & EV_ERROR)
    {
        UnRegister(sock);
        return false;
    }
    return true;
}

void ukqueue::unReg(int sock)
{
    struct kevent ke;
    EV_SET(&ke, sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(handle__, &ke, 1, NULL, 0, NULL);
    EV_SET(&ke, sock, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    kevent(handle__, &ke, 1, NULL, 0, NULL);
}

void ukqueue::toWrite(int sock, void *ud, bool enable)
{
    struct kevent ke;
    EV_SET(&ke, sock, EVFILT_WRITE, enable ? EV_ENABLE : EV_DISABLE, 0, 0, ud);
    if (kevent(handle__, &ke, 1, NULL, 0, NULL) == -1 || ke.flags & EV_ERROR)
    {
    }
}

int ukqueue::onWait(int max)
{
    struct kevent ev[max];
    int n = kevent(handle__, NULL, 0, ev, max, NULL);
    if (n <= 0)
    {
        if (errno == EINTR)
            return -2;
        return -1;
    }

    int i;
    for (i = 0; i < n; i++)
    {
        evAry__[i].s = ev[i].udata;
        unsigned filter = ev[i].filter;
        bool eof = (ev[i].flags & EV_EOF) != 0;
        evAry__[i].iswrite = (filter == EVFILT_WRITE) && (!eof);
        evAry__[i].isread = (filter == EVFILT_READ) && (!eof);
        evAry__[i].iserror = (ev[i].flags & EV_ERROR) != 0;
        evAry__[i].iseof = eof;
    }

    return n;
}

POLL_HANDLE ukqueue::initial()
{
    return kqueue();
}

void ukqueue::destory()
{
    close(handle__);
    handle__ = -1;
}

#endif