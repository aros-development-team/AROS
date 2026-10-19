/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * mdns_ipc.h - messages between bonami.library and mdnsd
 *
 * A client's call becomes one message to the public port MDNS_IPC_PORT,
 * and the library waits for the reply. The library and the daemon are
 * built from this one file; mi_Version says which.
 */

#ifndef MDNSD_MDNS_IPC_H
#define MDNSD_MDNS_IPC_H

#include <exec/types.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <bonami/mdns.h>

#define MDNS_IPC_PORT       "BonAmi"
#define MDNS_IPC_VERSION    3

#define MDNS_IPC_BROWSE     4
#define MDNS_IPC_STOPBROWSE 5
#define MDNS_IPC_RESOLVE    6
#define MDNS_IPC_STOPRESOLVE 7
#define MDNS_IPC_READ       17
#define MDNS_IPC_NOTIFY     18      /* lend mi_Request until an event */

#define MDNS_IPC_REGISTER   1
#define MDNS_IPC_UNREGISTER 2
#define MDNS_IPC_UPDATE     3
#define MDNS_IPC_RELEASE    12      /* drop everything mi_TaskID owns */

/* mi_Flags of MDNS_IPC_UPDATE: mi_TXT replaces the service's text. */
#define MDNS_IPC_UPDF_TXT   0x00000001UL

/* The longest a client may ask a record to live: RFC 6762 section 10's
 * 75 minutes. */
#define MDNS_MAX_TTL        4500

struct MDNSIPCMsg
{
    struct Message  mi_Msg;
    UWORD           mi_Version;
    UWORD           mi_Command;
    LONG            mi_Result;      /* MDNS_OK or MDNSERR_* */
    APTR            mi_Handle;      /* the registration */
    ULONG           mi_Flags;
    char            mi_Name[MDNS_MAX_NAME + 1];
    char            mi_ServiceType[MDNS_MAX_TYPE + 1];
    char            mi_Domain[MDNS_MAX_DOMAIN + 1];
    char            mi_HostName[MDNS_MAX_HOST + 1];
    char            mi_TXT[MDNS_MAX_TXT + 1];
    UWORD           mi_Port;
    UWORD           mi_Pad;
    ULONG           mi_HostAddr;    /* 0xAABBCCDD == A.B.C.D */
    ULONG           mi_TTL;         /* seconds, 0 for the defaults */
    ULONG           mi_DaemonID;    /* expected incarnation, zero on create */
    struct MDNSDiscoveryEvent mi_Event;
    ULONG           mi_TaskID;      /* GetETaskID() of the calling task */
    struct MDNSEventRequest *mi_Request; /* MDNS_IPC_NOTIFY only */
};

#endif /* MDNSD_MDNS_IPC_H */
