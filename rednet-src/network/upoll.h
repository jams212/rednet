#ifndef REDNET_UPOLL_H
#define REDNET_UPOLL_H

#if defined(__linux__)
#include "uepoll.h"
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include "ukqueue.h"
#endif

#endif