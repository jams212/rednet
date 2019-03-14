#include "ugroup.h"
#include "ucontext.h"
#include <assert.h>

using namespace rednet::server;
ugroup::ugroup() : groupID__(~0),
                   remote__(NULL)
{
}

ugroup::~ugroup()
{
}

void ugroup::init(int groupID)
{
    groupID__ = (unsigned int)groupID << ID_REMOTE_SHIFT;
}

bool ugroup::isRemote(uint32_t id)
{
    assert(groupID__ != (uint32_t)~0);
    int h = (groupID__ & ~ID_MARK);
    return (uint32_t)h != groupID__ && h != 0;
}
