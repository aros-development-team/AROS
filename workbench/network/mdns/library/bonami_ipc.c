/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 amigazen project
 * Copyright 2026 Jaime Dias
 *
 * bonami_ipc.c - the library's side of the messages to mdnsd
 */

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <utility/tagitem.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/macros.h>

#include "bonami_intern.h"

/* A copy that always ends with a NUL within size bytes. */
static void
copy_text(char *to, LONG size, CONST_STRPTR from)
{
    LONG n = 0;

    while (from != NULL && n < size - 1 && from[n] != '\0') {
        to[n] = from[n];
        n++;
    }
    to[n] = '\0';
}

/* Is mdnsd there? It may still go away before the next call. */
LONG
bonamiEngineThere(void)
{
    const struct MsgPort *port;

    Forbid();
    port = FindPort((STRPTR)MDNS_IPC_PORT);
    Permit();
    return port != NULL ? MDNS_OK : MDNSERR_NOENGINE;
}

/* Tells mdnsd to drop every registration of the calling task. */
void
bonamiReleaseTask(void)
{
    struct MDNSIPCMsg msg;

    memset(&msg, 0, sizeof(msg));
    msg.mi_Command = MDNS_IPC_RELEASE;
    bonamiSend(&msg);
}

/* What a registration says, out of its tags. A tag that is not given
 * leaves an empty text or a zero, and the domain "local". */
LONG
bonamiTagsToMsg(struct TagItem *tags, struct MDNSIPCMsg *msg)
{
    struct TagItem *tag;
    struct TagItem *cur = tags;

    copy_text(msg->mi_Domain, (LONG)sizeof(msg->mi_Domain),
        (CONST_STRPTR)MDNS_DOMAIN_LOCAL);
    while ((tag = NextTagItem(&cur)) != NULL) {
        switch (tag->ti_Tag) {
        case MDNS_ServiceName:
            copy_text(msg->mi_Name, (LONG)sizeof(msg->mi_Name),
                (CONST_STRPTR)tag->ti_Data);
            break;
        case MDNS_ServiceType:
            copy_text(msg->mi_ServiceType,
                (LONG)sizeof(msg->mi_ServiceType),
                (CONST_STRPTR)tag->ti_Data);
            break;
        case MDNS_Port:
            msg->mi_Port = (UWORD)tag->ti_Data;
            break;
        case MDNS_TimeToLive:
            /* none shorter than a second, which would be a goodbye, and
             * none longer than RFC 6762 section 10 recommends */
            if (tag->ti_Data < 1 || tag->ti_Data > MDNS_MAX_TTL) {
                return MDNSERR_BADPARAM;
            }
            msg->mi_TTL = (ULONG)tag->ti_Data;
            break;
        case MDNS_TXTRecord:
            copy_text(msg->mi_TXT, (LONG)sizeof(msg->mi_TXT),
                (CONST_STRPTR)tag->ti_Data);
            break;
        case MDNS_HostName:
            copy_text(msg->mi_HostName, (LONG)sizeof(msg->mi_HostName),
                (CONST_STRPTR)tag->ti_Data);
            break;
        case MDNS_HostAddr:
            /* mdnsd keeps addresses as 0xAABBCCDD == A.B.C.D */
            msg->mi_HostAddr = AROS_BE2LONG((ULONG)tag->ti_Data);
            break;
        case MDNS_Domain:
            if (tag->ti_Data != 0) {
                copy_text(msg->mi_Domain, (LONG)sizeof(msg->mi_Domain),
                    (CONST_STRPTR)tag->ti_Data);
            }
            break;
        default:
            break;
        }
    }
    return MDNS_OK;
}

/*
 * Sends a copy of msg to mdnsd, waits for the reply and takes the result
 * and the handle from it.
 */
LONG
bonamiSend(struct MDNSIPCMsg *msg)
{
    struct MsgPort *enginePort;
    struct MsgPort *replyPort;
    struct MDNSIPCMsg *copy;

    /* A reply port of the calling task's own, so that clients calling at
     * the same time never take each other's reply. */
    replyPort = CreateMsgPort();
    if (replyPort == NULL) {
        return MDNSERR_NOMEM;
    }
    copy = (struct MDNSIPCMsg *)AllocMem(sizeof(*copy), MEMF_CLEAR);
    if (copy == NULL) {
        DeleteMsgPort(replyPort);
        return MDNSERR_NOMEM;
    }

    memcpy(copy, msg, sizeof(*copy));
    copy->mi_Msg.mn_Node.ln_Type = NT_MESSAGE;
    copy->mi_Msg.mn_Length = sizeof(*copy);
    copy->mi_Msg.mn_ReplyPort = replyPort;
    copy->mi_Version = MDNS_IPC_VERSION;
    copy->mi_Result = MDNSERR_NOTREADY;
    copy->mi_TaskID = GetETaskID(FindTask(NULL));
    if (copy->mi_TaskID == 0) {
        FreeMem(copy, sizeof(*copy));
        DeleteMsgPort(replyPort);
        return MDNSERR_BADPARAM;
    }

    /* mdnsd removes its port before it stops reading it, so a message sent
     * to a port found here is always answered. */
    Forbid();
    enginePort = FindPort((STRPTR)MDNS_IPC_PORT);
    if (enginePort != NULL) {
        PutMsg(enginePort, &copy->mi_Msg);
    }
    Permit();
    if (enginePort == NULL) {
        FreeMem(copy, sizeof(*copy));
        DeleteMsgPort(replyPort);
        return MDNSERR_NOENGINE;
    }

    WaitPort(replyPort);
    GetMsg(replyPort);

    msg->mi_Result = copy->mi_Result;
    msg->mi_Handle = copy->mi_Handle;
    FreeMem(copy, sizeof(*copy));
    DeleteMsgPort(replyPort);
    return msg->mi_Result;
}
