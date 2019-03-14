#include "uchannel.h"
#include "upoll.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

using namespace rednet::network;

uchannel::uchannel(ipoll *ptr) : rCtrl__(NT_INVALID),
                                 sCtrl__(NT_INVALID),
                                 cCtrl__(NT_INVALID),
                                 ptrPoll__(ptr)
{
    FD_ZERO(&rsfds__);
    int fd[2];
    if (pipe(fd))
    {
        //fprintf(stderr, "ctrl: create socket pair failed.\n");
        //return false;
        return;
    }

    if (!ptrPoll__->reg(fd[0], NULL))
    {
        close(fd[0]);
        close(fd[1]);
        return;
    }

    rCtrl__ = fd[0];
    sCtrl__ = fd[1];
    cCtrl__ = 1;

    assert(rCtrl__ < FD_SETSIZE);
}

uchannel::~uchannel()
{
    if (!NT_IS_INVALID(rCtrl__))
    {
        if (ptrPoll__)
        {
            ptrPoll__->unReg(rCtrl__);
        }
        close(rCtrl__);
        rCtrl__ = NT_INVALID;
    }

    if (!NT_IS_INVALID(sCtrl__))
    {
        close(sCtrl__);
        sCtrl__ = NT_INVALID;
    }

    cCtrl__ = NT_INVALID;
    ptrPoll__ = NULL;
}

bool uchannel::isInvalid()
{
    return NT_IS_INVALID(rCtrl__) && NT_IS_INVALID(sCtrl__) && NT_IS_INVALID(cCtrl__);
}

void uchannel::forward(const char *data, const size_t bytes)
{
    for (;;)
    {
        ssize_t n = ::write(sCtrl__, data, bytes);
        if (n < 0)
        {
            if (errno != EINTR)
            {
                fprintf(stderr, "ctrl server : send command error %s.\n", strerror(errno));
            }
            continue;
        }
        assert((const size_t)n == bytes);
        return;
    }
}

void uchannel::recv(void *buffer, const size_t bytes)
{
    for (;;)
    {
        int n = ::read(rCtrl__, buffer, bytes);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "Channel : read pipe error %s.\n", strerror(errno));
            return;
        }
        assert((const size_t)n == bytes);
        return;
    }
}

void uchannel::rest()
{
    cCtrl__ = 1;
}

bool uchannel::isRead()
{
    struct timeval tv = {0, 0};
    int retval;

    FD_SET(rCtrl__, &rsfds__);
    retval = ::select(rCtrl__ + 1, &rsfds__, NULL, NULL, &tv);
    if (retval == 1)
        return true;
    return false;
}

int uchannel::onWait(function<int(uchannel *h)> onRecvSync)
{
    if (!cCtrl__)
        return NT_CONTINUE_ERR;
    if (!isRead())
    {
        cCtrl__ = 0;
        return NT_CONTINUE_ERR;
    }

    return onRecvSync(this);
}