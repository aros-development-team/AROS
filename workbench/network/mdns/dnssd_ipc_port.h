/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_ipc_port.h - the Exec side of dnssd.library's requests to mdnsd
 *
 * A request (dnssd_ipc.h) travels in a message sent to the public port
 * DNSSD_IPC_PORT: the Exec message, then the request and its data, one
 * block of mn_Length bytes that the library allocates for the call and
 * waits on until mdnsd replies. mdnsd reads the request only after
 * dnssd_ipc_validate() has passed it, over the bytes mn_Length says.
 *
 * The one thing lent is the session's notification message (OPEN, and
 * READ when it found the queue empty): mdnsd replies to it, to the
 * session's port, when the session has events to read, and gives it back
 * on CLOSE. When the client task is gone mdnsd drops the session without
 * replying.
 */
#ifndef MDNSD_DNSSD_IPC_PORT_H
#define MDNSD_DNSSD_IPC_PORT_H

#include <exec/types.h>
#include <exec/ports.h>
#include <stddef.h>

#include "dnssd_ipc.h"

#define DNSSD_IPC_PORT      "DNSSD"

struct DNSSDIPCMsg
{
    struct Message           dm_Msg;     /* mn_Length: the whole block */
    struct Message          *dm_Notify;  /* OPEN, READ: the session's, lent */
    struct dnssd_ipc_request dm_Request; /* and its data, after it */
};

/* Bytes of the request in a message mn_Length long. */
#define DNSSD_IPC_REQUEST_ROOM(length) \
    ((length) > offsetof(struct DNSSDIPCMsg, dm_Request) ? \
     (size_t)(length) - offsetof(struct DNSSDIPCMsg, dm_Request) : 0)

#endif /* MDNSD_DNSSD_IPC_PORT_H */
