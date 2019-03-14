#ifndef REDNET_UEVENT_H
#define REDNET_UEVENT_H

#include "macro.h"

namespace rednet
{

struct uevent
{
    uint32_t source;
    int32_t session;
    void *data;
    uint16_t type;
    uint32_t sz;
};

enum uevent_type
{
    EP_DONT = 0,
    EP_TEXT,
    EP_OPER,
    EP_RESONE,
    EP_ERROR,
    EP_SOCKET,
    EP_LUA,
    EP_DEBUG,
    EP_USER,
};

} // namespace rednet

#endif