/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_intern.h - what the parts of dnssd.library share
 */
#ifndef DNSSD_INTERN_H
#define DNSSD_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <dnssd/dnssd.h>
#include "dnssd_ipc.h"

/*
 * The library holds no DNS-SD state: every call is a message to mdnsd
 * (dnssd_ipc.h). A session and its operations are the caller's memory:
 * the session lends mdnsd its notification message, which mdnsd replies to
 * the caller's port when there are events to read.
 */
struct DNSSDIntern
{
    struct Library  di_Lib;
};

struct DNSSDSession
{
    struct Message  ds_Notify;      /* lent to mdnsd while it is not on ds_Port */
    struct MsgPort *ds_Port;
    ULONG           ds_Session;
    ULONG           ds_Incarnation;
    struct MinList  ds_Operations;
};

struct DNSSDOperation
{
    struct MinNode  do_Node;
    struct DNSSDSession *do_Session;
    ULONG           do_ID;
};

LONG dnssdSend(struct DNSSDIPCMsg *msg);

#endif /* DNSSD_INTERN_H */
