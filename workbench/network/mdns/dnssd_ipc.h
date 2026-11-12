/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_ipc.h - messages between dnssd.library and mdnsd
 *
 * Private to the two: the library is the only client, and both are built
 * from this file (dm_Version says which). Each library call is one message
 * to the public port DNSSD_IPC_PORT; the library waits for the reply, so
 * mdnsd may read and write dm_Data, which is the caller's, while it handles
 * the message and never after.
 *
 * Sessions and operations are numbers mdnsd gives out: a slot and a
 * generation, under the daemon's incarnation, so that a number from before
 * a restart, or of a slot used again since, is refused (DNSSD_ERR_STALE).
 *
 * A session lends mdnsd one notification message (OPEN, and READ when it
 * found the queue empty): mdnsd replies to it, to the session's port, when
 * the session has events to read, and gives it back on CLOSE. When the
 * client task is gone mdnsd drops the session without replying.
 */
#ifndef MDNSD_DNSSD_IPC_H
#define MDNSD_DNSSD_IPC_H

#include <exec/types.h>
#include <exec/ports.h>
#include <dnssd/dnssd.h>

#define DNSSD_IPC_PORT      "DNSSD"
#define DNSSD_IPC_VERSION   1

#define DNSSD_IPC_OPEN      1   /* dm_Notify lent; -> dm_Session, dm_Incarnation */
#define DNSSD_IPC_CLOSE     2   /* cancels every operation; dm_Notify given back */
#define DNSSD_IPC_CANCEL    3   /* dm_Operation */
#define DNSSD_IPC_READ      4   /* next event into dm_Data/dm_DataMax; EMPTY lends dm_Notify again */
#define DNSSD_IPC_STATUS    5
#define DNSSD_IPC_LOOKUP    6   /* dm_Data: the name; dm_Family, dm_Interface, dm_Timeout -> dm_Operation */
#define DNSSD_IPC_BROWSE    7   /* dm_Data: [subtype._sub.]type.domain; dm_TypeOffset, dm_DomainOffset, dm_Interface */
#define DNSSD_IPC_RESOLVE   8   /* dm_Data: instance.type.domain; dm_TypeOffset, dm_DomainOffset, dm_Interface, dm_Timeout */
#define DNSSD_IPC_QUERY     9   /* dm_Data: the name; dm_RRType, dm_Interface, dm_Timeout (0: until cancelled) */
#define DNSSD_IPC_REGISTER  10  /* dm_Data: instance.type.domain, offsets; dm_Port; dm_Extra: TXT
                                 * (dm_TXTLength bytes) then each subtype as a label, then 0 */
#define DNSSD_IPC_UPDATE    11  /* dm_Operation; dm_Extra: TXT if DNSSD_IPCF_TXT; dm_Port if not 0 */

#define DNSSD_IPCF_TXT      1   /* dm_Flags: UPDATE carries TXT data */

/* Names in dm_Data are DNS wire labels, uncompressed, dm_DataSize bytes
 * with the root label; the offsets are where a part's labels start. */

struct DNSSDIPCMsg
{
    struct Message  dm_Msg;
    UWORD           dm_Version;
    UWORD           dm_Command;
    LONG            dm_Result;
    ULONG           dm_TaskID;      /* GetETaskID() of the calling task */
    ULONG           dm_Incarnation; /* mdnsd's, as OPEN gave it */
    ULONG           dm_Session;
    ULONG           dm_Operation;
    struct Message *dm_Notify;      /* OPEN, READ: the session's notification */
    ULONG           dm_Family;
    ULONG           dm_Interface;
    ULONG           dm_Timeout;     /* seconds */
    UWORD           dm_RRType;
    UWORD           dm_TypeOffset;
    UWORD           dm_DomainOffset;
    UWORD           dm_Port;
    UWORD           dm_TXTLength;
    UWORD           dm_Flags;
    ULONG           dm_ExtraSize;
    UBYTE          *dm_Extra;
    UBYTE           dm_Host[64];    /* REGISTER for another host: its name's first label */
    struct DNSSDAddress dm_HostAddress;
    IPTR            dm_UserData;
    ULONG           dm_DataSize;    /* bytes of dm_Data used */
    ULONG           dm_DataMax;     /* bytes of dm_Data */
    UBYTE          *dm_Data;
    ULONG           dm_StatusSessions;
    ULONG           dm_StatusOperations;
};

#endif /* MDNSD_DNSSD_IPC_H */
