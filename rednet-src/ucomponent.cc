#include "ucomponent.h"

using namespace rednet;

ucomponent::ucomponent() : parent__(NULL),
                           callback__(nullptr),
                           ud__(NULL)
{
}

ucomponent::~ucomponent()
{
    parent__ = NULL;
}

int ucomponent::init(ucontext *ctx, const char *parm)
{
    parent__ = ctx;
    return 1;
}

int ucomponent::signal(int signal)
{
    return 1;
}
