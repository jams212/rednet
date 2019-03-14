#include "usignal.h"

using namespace rednet;

static unsigned char signalEvent[__SIGRTMIN] = {0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0,
                                                0, 0, 0, 0, 0, 0, 0, 0};

static void
signalCallback(int signal)
{
    signalEvent[signal] = 1;
}

usignal::usignal(int flags, int sig, signalFun cb)
{
    sa__.sa_handler = &signalCallback;
    sa__.sa_flags = flags;
    sigfillset(&sa__.sa_mask);
    sigaction(sig, &sa__, NULL);
    sig__ = sig;
    cb__ = cb;
}

usignal::~usignal()
{
}

void usignal::wait()
{
    if (signalEvent[sig__])
    {
        cb__();
        signalEvent[sig__] = 0;
    }
}
