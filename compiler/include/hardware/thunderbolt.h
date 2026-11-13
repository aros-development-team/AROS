#ifndef HARDWARE_THUNDERBOLT_H
#define HARDWARE_THUNDERBOLT_H

/*
    Copyright © 2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

#define TBT_MAX_HOPS            6
#define TBT_MAX_PORTS           64
#define TBT_ROUTE_HOP_BITS      8
#define TBT_ROUTE_HOP_MASK      0xff
#define TBT_ROUTE_HOP_UNUSED    0xff

#define TBT_ROUTE_SHIFT(hop)    ((hop) * TBT_ROUTE_HOP_BITS)
#define TBT_ROUTE_PORT(route, hop) \
    (((route) >> TBT_ROUTE_SHIFT(hop)) & TBT_ROUTE_HOP_MASK)

struct TBT_Route
{
    UBYTE hops[TBT_MAX_HOPS];
};

struct TBT_DeviceAddress
{
    UWORD domain;
    UBYTE depth;
    UBYTE port;
    UQUAD route;
};

#endif
