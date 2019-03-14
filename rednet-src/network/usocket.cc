#include "usocket.h"
#include "usocket_server.h"
#include "upoll.h"
#include <atomic.h>
#include <string.h>

using namespace rednet::network;

usocket::usocket() : id__(NT_INVALID),
                     opaque__(0),
                     sck__(NT_INVALID),
                     forwardIn__(0),
                     forwardBufferOffset__(0),
                     forwardBuffer__(NULL),
                     forwardBufferSz__(0),
                     forwardSize__(),
                     forwardWarnSize__(0),
                     protocol__(PROTOCOL_UNKNOWN),
                     status__(TE_INVALID),
                     parentPoll__(NULL)
{
    doForwardListClear();
    p__.size = 0;
    memset(&info__, 0, sizeof(struct socketInfo));
}

usocket::~usocket()
{
}

bool usocket::isForward()
{
    return isForwardList() && (forwardBuffer__ == NULL) && ((forwardIn__ && 0xffff) == 0);
}

bool usocket::isForwardList()
{
    return forwardList__.empty();
}

bool usocket::isCanDirectWrite(int socketId)
{
    return id__ == socketId && isForward() && status__ == TE_CONNECTED;
}

void usocket::incForwardRef()
{
    if (protocol__ != PROTOCOL_TCP)
        return;
    for (;;)
    {
        uint32_t forwardIn = forwardIn__;
        if ((forwardIn >> 16) == (uint32_t)NT_ID_TAG16(id__))
        {
            if ((forwardIn & 0xffff) == 0xffff)
            {
                // forwardIn may overflow (rarely), so busy waiting here for socket thread dec it. see issue #794
                continue;
            }
            // inc sending only matching the same socket id
            if (ATOM_CAS(&forwardIn__, forwardIn, forwardIn + 1))
                return;
            // atom inc failed, retry
        }
        else
        {
            // socket id changed, just return
            return;
        }
    }
}
void usocket::decForwardRef()
{
    if (protocol__ != PROTOCOL_TCP)
        return;
    assert((forwardIn__ & 0xffff) != 0);
    ATOM_DEC(&forwardIn__);
}

int usocket::beginForwardList()
{
    if (protocol__ == PROTOCOL_TCP)
    {
        return beginForwardTCP();
    }
    else
    {
        return -1;
    }
}

int usocket::beginForwardTCP()
{
    while (!forwardList__.empty())
    {
        struct writeBuffer *tmp = forwardList__.front();
        for (;;)
        {
            ssize_t sz = ::write(sck__, tmp->ptr, tmp->sz);
            if (sz < 0)
            {
                switch (errno)
                {
                case EINTR:
                    continue;
                case AGAIN_WOULDBLOCK:
                    return -1;
                }
                return NT_SOCKET_CLOSE;
            }

            info__.wrTime = INST(usocket_server, getNowTime);
            info__.wrBytes += sz;

            forwardSize__ -= sz;
            if (sz != tmp->sz)
            {
                tmp->ptr += sz;
                tmp->sz -= sz;
                return -1;
            }
            break;
        }
        forwardList__.pop_front();
        unetwork::freeWriteBuffer(tmp);
    }
    return -1;
}

void usocket::pushForwardList(const char *buffer, int sz)
{
    struct writeBuffer *buf = (struct writeBuffer *)umemory::malloc(sz);

    buf->ptr = (char *)buffer;
    buf->sz = sz;
    buf->buffer = (char *)buffer;

    forwardList__.push_back(buf);

    forwardSize__ += sz;
}

void usocket::doForwardListClear()
{
    struct writeBuffer *lwfree = NULL;
    while (!forwardList__.empty())
    {
        lwfree = forwardList__.front();
        forwardList__.pop_front();
        unetwork::freeWriteBuffer(lwfree);
    }
}

void usocket::doInit(int socketId, SOCKET_HANDLE sck, int protocol, uintptr_t opaque)
{
    //int socketId, SOCKET_HANDLE sck, int protocol, uintptr_t opaque
    id__ = socketId;
    sck__ = sck;
    protocol__ = protocol;
    opaque__ = opaque;
    forwardIn__ = NT_ID_TAG16(socketId) << 16 | 0;
    p__.size = NT_MIN_READ_BUFFER;
    forwardBuffer__ = NULL;
    forwardBufferOffset__ = 0;
    forwardBufferSz__ = 0;
    forwardSize__ = 0;
    forwardWarnSize__ = 0;
    doForwardListClear();
}

int usocket::beginForward()
{
    // step 1
    if (beginForwardList() == NT_SOCKET_CLOSE)
    {
        return NT_SOCKET_CLOSE;
    }

    if (forwardList__.empty())
    {
        assert(forwardSize__ == 0);
        parentPoll__->toWrite(sck__, this, false);

        if (status__ == TE_HALFCLOSE)
        {
            return NT_SOCKET_CLOSE;
        }

        if (forwardWarnSize__ > 0)
        {
            forwardWarnSize__ = 0;
            return NT_SOCKET_WARNING;
        }
    }

    return -1;
}

int usocket::onAccept(union sockAddrAll *u)
{
    socklen_t len = sizeof(*u);
    return accept(sck__, &(u->s), &len);
}

int usocket::onForward(urecuresivelock *rl)
{
    if (!rl->trylock())
        return -1;
    if (forwardBuffer__)
    {
        struct writeBuffer *buf = (struct writeBuffer *)umemory::malloc(TCP_BUFFER_SIZE);
        buf->ptr = ((char *)forwardBuffer__ + forwardBufferOffset__);
        buf->sz = forwardBufferSz__ - forwardBufferOffset__;
        buf->buffer = (char *)forwardBuffer__;

        forwardSize__ += buf->sz;
        forwardList__.push_front(buf);
        forwardBufferOffset__ = 0;
        forwardBufferSz__ = 0;
        forwardBuffer__ = NULL;
    }

    int r = beginForward();
    if (r == NT_SOCKET_CLOSE)
    {
        onClose(rl);
    }
    rl->unlock();

    return r;
}

int usocket::onForwardData(char *inBuffer, int inSz)
{
    ssize_t n = 0;
    if (protocol__ == PROTOCOL_TCP)
    {
        n = ::write(sck__, inBuffer, inSz);
    }
    else
    {
        //TODO: UDP
    }

    if (n < 0)
    {
        // ignore error, let socket thread try again
        n = 0;
    }

    if (n == inSz)
    {
        umemory::free(inBuffer); //TODO: free rest
        return 0;
    }

    forwardBuffer__ = inBuffer;
    forwardBufferSz__ = inSz;
    forwardBufferOffset__ = n;

    parentPoll__->toWrite(sck__, this, true);

    return 0;
}

int usocket::onRecv(urecuresivelock *rl, char **outBuffer, int *outSz)
{
    int sz = p__.size;
    char *buffer = (char *)umemory::malloc(sz);
    assert(buffer);
    int n = (int)::read(sck__, buffer, sz);
    if (n < 0)
    {
        umemory::free(buffer);
        switch (errno)
        {
        case EINTR:
            break;
        case AGAIN_WOULDBLOCK:
            fprintf(stderr, "socket-server: EAGAIN capture.\n");
            break;
        default:
            onClose(rl);
            return NT_SOCKET_ERR;
        }
        return -1;
    }

    if (n == 0)
    {
        umemory::free(buffer);
        onClose(rl);
        return NT_SOCKET_CLOSE;
    }
    if (status__ == TE_HALFCLOSE)
    {
        umemory::free(buffer);
        return -1;
    }
    if (n == sz)
    {
        p__.size *= 2;
    }
    else if (sz > NT_MIN_READ_BUFFER && n * 2 > sz)
    {
        p__.size /= 2;
    }
    info__.rvTime = INST(usocket_server, getNowTime);
    info__.rvBytes += n;

    *outBuffer = buffer;
    *outSz = n;
    return NT_SOCKET_DATA;
}

void usocket::onClose(urecuresivelock *rl)
{
    if (status__ == socketStatus::TE_INVALID)
    {
        return;
    }

    assert(status__ != socketStatus::TE_RESERVE);

    doForwardListClear();
    if (status__ != socketStatus::TE_PACCEPT &&
        status__ != socketStatus::TE_PLISTEN)
    {
        if (parentPoll__ != NULL)
        {
            parentPoll__->unReg(sck__);
        }
    }

    rl->lock();
    if (status__ != socketStatus::TE_BIND)
    {
        if (::close(sck__) < 0)
        {
            perror("close socket:");
        }
    }
    status__ = socketStatus::TE_INVALID;
    if (forwardBuffer__)
    {
        umemory::free((void *)forwardBuffer__);
        forwardBuffer__ = NULL;
    }
    rl->unlock();
}
