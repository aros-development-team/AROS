/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_api.c - dnssd.library's calls
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <devices/timer.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <aros/libcall.h>

#include "dnssd_intern.h"

/*
 * One call of mdnsd: msg goes to its port and comes back answered. A reply
 * port of the caller's own, so that tasks calling at once never take each
 * other's answer.
 */
LONG
dnssdSend(struct DNSSDIPCMsg *msg)
{
    struct MsgPort *engine, *reply;

    reply = CreateMsgPort();
    if (reply == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    msg->dm_Msg.mn_Node.ln_Type = NT_MESSAGE;
    msg->dm_Msg.mn_Length = sizeof(*msg);
    msg->dm_Msg.mn_ReplyPort = reply;
    msg->dm_Version = DNSSD_IPC_VERSION;
    msg->dm_TaskID = GetETaskID(FindTask(NULL));
    msg->dm_Result = DNSSD_ERR_NOENGINE;
    Forbid();
    engine = FindPort((STRPTR)DNSSD_IPC_PORT);
    if (engine != NULL) {
        PutMsg(engine, &msg->dm_Msg);
    }
    Permit();
    if (engine != NULL) {
        WaitPort(reply);
        GetMsg(reply);
    }
    DeleteMsgPort(reply);
    return msg->dm_Result;
}

/*
 * The notification came back (mdnsd replied to it): if it still waits on
 * the session's port, it is taken off, so that it can be lent again. The
 * caller may have taken it off itself; either way it is the library's.
 */
static BOOL
notify_back(struct DNSSDSession *ds)
{
    struct Node *node;

    if (ds->ds_Notify.mn_Node.ln_Type != NT_REPLYMSG) {
        return FALSE;                   /* still lent */
    }
    Forbid();
    for (node = ds->ds_Port->mp_MsgList.lh_Head; node->ln_Succ != NULL;
         node = node->ln_Succ) {
        if (node == &ds->ds_Notify.mn_Node) {
            Remove(node);
            break;
        }
    }
    Permit();
    return TRUE;
}

/*
 * A name as text - labels between dots, "\." a dot and "\\" a backslash
 * inside a label (RFC 1035 section 5.1), a trailing dot allowed - as DNS
 * wire labels at wire + at, ended by the root label. The length of the
 * whole, or -1 if it does not make a name.
 */
static LONG
text_to_wire(const char *text, UBYTE *wire, LONG at)
{
    while (*text != '\0') {
        LONG start = at++;

        while (*text != '\0' && *text != '.') {
            if (*text == '\\' && text[1] != '\0') {
                text++;
            }
            if (at >= 255 || at - start > 63) {
                return -1;
            }
            wire[at++] = (UBYTE)*text++;
        }
        if (at - start == 1) {
            return -1;                  /* an empty label */
        }
        wire[start] = (UBYTE)(at - start - 1);
        if (*text == '.') {
            text++;
        }
    }
    if (at == 0 || at >= 256) {
        return -1;
    }
    wire[at++] = 0;
    return at;
}

/* One label, as it is (an instance's name may hold dots), at wire + at;
 * the length so far, or -1. */
static LONG
label_to_wire(const char *label, UBYTE *wire, LONG at)
{
    ULONG length = strnlen(label, 64);

    if (length == 0 || length > 63 || at + 1 + length > 255) {
        return -1;
    }
    wire[at] = (UBYTE)length;
    memcpy(wire + at + 1, label, length);
    return at + 1 + length;
}

/* An operation of session from msg, which names it; the tags every one
 * takes. */
static LONG
start(struct DNSSDSession *session, struct DNSSDIPCMsg *msg,
    struct TagItem *tags, DNSSDOperationRef *operation)
{
    struct DNSSDOperation *op;
    LONG rc;

    op = AllocVec(sizeof(*op), MEMF_PUBLIC | MEMF_CLEAR);
    if (op == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    msg->dm_Interface = GetTagData(DNSSD_Interface, 0, tags);
    msg->dm_UserData = GetTagData(DNSSD_UserData, 0, tags);
    rc = dnssdSend(msg);
    if (rc != DNSSD_OK) {
        FreeVec(op);
        return rc;
    }
    op->do_Session = session;
    op->do_ID = msg->dm_Operation;
    AddTail((struct List *)&session->ds_Operations, (struct Node *)op);
    *operation = op;
    return DNSSD_OK;
}

/*
 * <type>.<domain> after wire + at (and "<subtype>._sub." before them): the
 * offsets of the type and the domain, and the whole length, or -1.
 */
static LONG
service_to_wire(const char *subtype, const char *type, const char *domain,
    UBYTE *wire, LONG at, UWORD *typeOffset, UWORD *domainOffset)
{
    if (subtype != NULL) {
        if ((at = label_to_wire(subtype, wire, at)) < 0 ||
            (at = label_to_wire("_sub", wire, at)) < 0) {
            return -1;
        }
    }
    *typeOffset = (UWORD)at;
    /* a type is two labels, "_name._tcp" or "_name._udp" */
    if ((at = text_to_wire(type, wire, at)) < 0) {
        return -1;
    }
    at--;                               /* the domain follows */
    *domainOffset = (UWORD)at;
    return text_to_wire(domain, wire, at);
}

static void
init_msg(struct DNSSDIPCMsg *msg, const struct DNSSDSession *ds, UWORD command)
{
    memset(msg, 0, sizeof(*msg));
    msg->dm_Command = command;
    if (ds != NULL) {
        msg->dm_Session = ds->ds_Session;
        msg->dm_Incarnation = ds->ds_Incarnation;
    }
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, DNSSD_OpenSessionA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef *, session, A0),
        AROS_LHA(struct MsgPort *, port, A1),
        AROS_LHA(struct TagItem *, tags, A2),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 5, DNSSD)

/*  FUNCTION
        Opens a session of the calling task with mdnsd. When it has events
        for the session's operations mdnsd puts a message on port - one for
        any number of events - and the task reads them with
        DNSSD_ReadEvent() until it says DNSSD_ERR_EMPTY. The message is the
        library's: the task may take it off the port, and must not reply to
        it. The port should be the session's alone.

    INPUTS
        session - where the new session goes
        port - the task's port for the session's notification
        tags - none yet

    RESULT
        DNSSD_OK, DNSSD_ERR_NOENGINE (mdnsd is not running),
        DNSSD_ERR_BUSY, DNSSD_ERR_NOMEM or DNSSD_ERR_BADPARAM.

    SEE ALSO
        DNSSD_CloseSession(), DNSSD_ReadEvent()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    struct DNSSDSession *ds;
    LONG rc;

    if (session == NULL || port == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    *session = NULL;
    ds = AllocVec(sizeof(*ds), MEMF_PUBLIC | MEMF_CLEAR);
    if (ds == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    ds->ds_Port = port;
    NEWLIST((struct List *)&ds->ds_Operations);
    ds->ds_Notify.mn_Node.ln_Type = NT_MESSAGE;
    ds->ds_Notify.mn_Length = sizeof(ds->ds_Notify);
    ds->ds_Notify.mn_ReplyPort = port;
    init_msg(&msg, NULL, DNSSD_IPC_OPEN);
    msg.dm_Notify = &ds->ds_Notify;
    rc = dnssdSend(&msg);
    if (rc != DNSSD_OK) {
        FreeVec(ds);
        return rc;
    }
    ds->ds_Session = msg.dm_Session;
    ds->ds_Incarnation = msg.dm_Incarnation;
    *session = ds;
    return DNSSD_OK;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(void, DNSSD_CloseSession,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 6, DNSSD)

/*  FUNCTION
        Cancels every operation of the session and closes it. Nothing of it
        reaches the port afterwards, and its notification is off the port.
        The operations' handles are freed.

    INPUTS
        session - the session, or NULL

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    struct DNSSDOperation *op;

    if (session == NULL) {
        return;
    }
    init_msg(&msg, session, DNSSD_IPC_CLOSE);
    dnssdSend(&msg);
    /* mdnsd holds the notification no more, replied or not */
    session->ds_Notify.mn_Node.ln_Type = NT_REPLYMSG;
    notify_back(session);
    while ((op = (struct DNSSDOperation *)RemHead(
            (struct List *)&session->ds_Operations)) != NULL) {
        FreeVec(op);
    }
    FreeVec(session);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, DNSSD_ReadEvent,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(struct DNSSDEvent *, event, A1),
        AROS_LHA(ULONG *, size, A2),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 7, DNSSD)

/*  FUNCTION
        Copies the session's next event into event, *size bytes, and sets
        *size to the event's size. An event that does not fit stays the
        next one: the call says DNSSD_ERR_TOOSMALL and *size how many bytes
        it needs. With none left the call says DNSSD_ERR_EMPTY, and the
        session's port is told when the next one comes.

    RESULT
        DNSSD_OK, DNSSD_ERR_EMPTY, DNSSD_ERR_TOOSMALL, DNSSD_ERR_STALE (mdnsd
        restarted: the session is no more) or DNSSD_ERR_NOENGINE.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    LONG rc;

    if (session == NULL || size == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    init_msg(&msg, session, DNSSD_IPC_READ);
    msg.dm_Data = (UBYTE *)event;
    msg.dm_DataMax = event != NULL ? *size : 0;
    /* lent again only if mdnsd finds nothing to read */
    if (notify_back(session)) {
        msg.dm_Notify = &session->ds_Notify;
    }
    rc = dnssdSend(&msg);
    if (rc == DNSSD_ERR_EMPTY && msg.dm_Notify != NULL) {
        session->ds_Notify.mn_Node.ln_Type = NT_MESSAGE;
    }
    if (rc == DNSSD_OK || rc == DNSSD_ERR_TOOSMALL) {
        *size = msg.dm_DataSize;
    }
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, DNSSD_Cancel,

/*  SYNOPSIS */
        AROS_LHA(DNSSDOperationRef, operation, A0),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 8, DNSSD)

/*  FUNCTION
        Ends the operation, if it has not ended, and frees its handle.
        Events of it already queued may still be read; none comes after.
        Every operation's handle is freed this way, ended or not (or by
        DNSSD_CloseSession()).

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    LONG rc;

    if (operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    init_msg(&msg, operation->do_Session, DNSSD_IPC_CANCEL);
    msg.dm_Operation = operation->do_ID;
    rc = dnssdSend(&msg);
    Remove((struct Node *)operation);
    FreeVec(operation);
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, DNSSD_LookupAddressA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(CONST_STRPTR, name, A1),
        AROS_LHA(DNSSDOperationRef *, operation, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 9, DNSSD)

/*  FUNCTION
        Looks the addresses of name up over multicast DNS (a name with no
        dot is taken in "local."). Each address found is an event
        DNSSD_EVENT_ADDRESS with the address, its family and the index of
        the interface it was learned on; the same address is not reported
        twice. When the time is up the operation ends with DNSSD_EVENT_END:
        DNSSD_OK if an address was found, DNSSD_ERR_TIMEOUT if not.

    INPUTS
        DNSSD_Family - DNSSD_FAMILY_IPV4, _IPV6 or _ANY (the default)
        DNSSD_Interface - only addresses learned on this interface index
        DNSSD_Timeout - seconds, 1 to 60, default 3
        DNSSD_UserData - returned in the operation's events

    RESULT
        DNSSD_OK and *operation, or an error.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    UBYTE wire[256];
    LONG length;

    if (session == NULL || name == NULL || operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    length = text_to_wire(name, wire, 0);
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    init_msg(&msg, session, DNSSD_IPC_LOOKUP);
    msg.dm_Data = wire;
    msg.dm_DataSize = length;
    msg.dm_Family = GetTagData(DNSSD_Family, DNSSD_FAMILY_ANY, tags);
    msg.dm_Timeout = GetTagData(DNSSD_Timeout, 3, tags);
    if (msg.dm_Timeout < 1 || msg.dm_Timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    return start(session, &msg, tags, operation);
    AROS_LIBFUNC_EXIT
}

/* After the first address, how long the others (the other family, other
 * links) are waited for. */
#define SYNC_GRACE_MICROS 250000

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, DNSSD_LookupAddressSyncA,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),
        AROS_LHA(struct DNSSDAddress *, addresses, A1),
        AROS_LHA(ULONG *, count, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 10, DNSSD)

/*  FUNCTION
        DNSSD_LookupAddressA() for a caller that waits: the addresses of
        name, at most *count of them, in addresses; *count is set to how
        many. It returns a quarter of a second after the first address, or
        when the time is up, or when the task gets SIGBREAKF_CTRL_C. It
        uses a session and a port of its own: any task may call it.

    INPUTS
        The tags of DNSSD_LookupAddressA().

    RESULT
        DNSSD_OK with at least one address, DNSSD_ERR_NOTFOUND,
        DNSSD_ERR_CANCELLED (Ctrl-C) or another error.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    union {
        struct DNSSDEvent event;
        UBYTE bytes[sizeof(struct DNSSDEvent) + 256 + sizeof(struct DNSSDAddress) + 8];
    } buf;
    struct MsgPort *port = NULL, *tport = NULL;
    struct timerequest *treq = NULL;
    DNSSDSessionRef session = NULL;
    DNSSDOperationRef op = NULL;
    ULONG max, found = 0;
    BOOL done = FALSE, timing = FALSE, timerOpen = FALSE;
    LONG rc;

    if (name == NULL || addresses == NULL || count == NULL || *count == 0) {
        return DNSSD_ERR_BADPARAM;
    }
    max = *count;
    *count = 0;
    port = CreateMsgPort();
    tport = CreateMsgPort();
    if (port == NULL || tport == NULL) {
        rc = DNSSD_ERR_NOMEM;
        goto out;
    }
    treq = (struct timerequest *)CreateIORequest(tport, sizeof(*treq));
    if (treq == NULL ||
        OpenDevice((STRPTR)TIMERNAME, UNIT_MICROHZ,
            (struct IORequest *)treq, 0) != 0) {
        rc = DNSSD_ERR_NOMEM;
        goto out;
    }
    timerOpen = TRUE;
    rc = AROS_LC3(LONG, DNSSD_OpenSessionA,
        AROS_LCA(DNSSDSessionRef *, &session, A0),
        AROS_LCA(struct MsgPort *, port, A1),
        AROS_LCA(struct TagItem *, NULL, A2),
        struct DNSSDIntern *, DNSSDBase, 5, DNSSD);
    if (rc != DNSSD_OK) {
        goto out;
    }
    rc = AROS_LC4(LONG, DNSSD_LookupAddressA,
        AROS_LCA(DNSSDSessionRef, session, A0),
        AROS_LCA(CONST_STRPTR, name, A1),
        AROS_LCA(DNSSDOperationRef *, &op, A2),
        AROS_LCA(struct TagItem *, tags, A3),
        struct DNSSDIntern *, DNSSDBase, 9, DNSSD);
    if (rc != DNSSD_OK) {
        goto out;
    }
    rc = DNSSD_ERR_NOTFOUND;
    while (!done) {
        ULONG got = Wait((1UL << port->mp_SigBit) | (1UL << tport->mp_SigBit) |
            SIGBREAKF_CTRL_C);

        if (got & SIGBREAKF_CTRL_C) {
            rc = DNSSD_ERR_CANCELLED;
            break;
        }
        if (timing && CheckIO((struct IORequest *)treq)) {
            WaitIO((struct IORequest *)treq);
            timing = FALSE;
            break;                      /* the grace after the first is over */
        }
        for (;;) {
            ULONG size = sizeof(buf);
            LONG r = AROS_LC3(LONG, DNSSD_ReadEvent,
                AROS_LCA(DNSSDSessionRef, session, A0),
                AROS_LCA(struct DNSSDEvent *, &buf.event, A1),
                AROS_LCA(ULONG *, &size, A2),
                struct DNSSDIntern *, DNSSDBase, 7, DNSSD);

            if (r == DNSSD_ERR_EMPTY) {
                break;
            }
            if (r != DNSSD_OK) {
                rc = r;
                done = TRUE;
                break;
            }
            if (buf.event.de_Kind == DNSSD_EVENT_ADDRESS &&
                buf.event.de_AddressCount != 0 && found < max) {
                memcpy(&addresses[found++],
                    buf.bytes + buf.event.de_Addresses.dr_Offset,
                    sizeof(*addresses));
                rc = DNSSD_OK;
                if (!timing) {
                    treq->tr_node.io_Command = TR_ADDREQUEST;
                    treq->tr_time.tv_secs = 0;
                    treq->tr_time.tv_micro = SYNC_GRACE_MICROS;
                    SendIO((struct IORequest *)treq);
                    timing = TRUE;
                }
            } else if (buf.event.de_Kind == DNSSD_EVENT_END ||
                       buf.event.de_Kind == DNSSD_EVENT_LOST) {
                done = TRUE;
                break;
            }
            if (found == max) {
                done = TRUE;
                break;
            }
        }
    }
    *count = found;
out:
    if (session != NULL) {
        AROS_LC1(void, DNSSD_CloseSession,
            AROS_LCA(DNSSDSessionRef, session, A0),
            struct DNSSDIntern *, DNSSDBase, 6, DNSSD);
    }
    if (timerOpen) {
        if (timing) {
            AbortIO((struct IORequest *)treq);
            WaitIO((struct IORequest *)treq);
        }
        CloseDevice((struct IORequest *)treq);
    }
    if (treq != NULL) {
        DeleteIORequest((struct IORequest *)treq);
    }
    if (tport != NULL) {
        DeleteMsgPort(tport);
    }
    if (port != NULL) {
        DeleteMsgPort(port);
    }
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, DNSSD_GetStatusA,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 11, DNSSD)

/*  FUNCTION
        mdnsd's state: each tag's ULONG * gets its value.

    INPUTS
        DNSSD_Status_Incarnation, DNSSD_Status_Sessions,
        DNSSD_Status_Operations

    RESULT
        DNSSD_OK or DNSSD_ERR_NOENGINE.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    struct TagItem *tstate = tags, *tag;
    LONG rc;

    init_msg(&msg, NULL, DNSSD_IPC_STATUS);
    rc = dnssdSend(&msg);
    if (rc != DNSSD_OK) {
        return rc;
    }
    while ((tag = NextTagItem(&tstate)) != NULL) {
        ULONG *to = (ULONG *)tag->ti_Data;

        if (to == NULL) {
            continue;
        }
        switch (tag->ti_Tag) {
        case DNSSD_Status_Incarnation:
            *to = msg.dm_Incarnation;
            break;
        case DNSSD_Status_Sessions:
            *to = msg.dm_StatusSessions;
            break;
        case DNSSD_Status_Operations:
            *to = msg.dm_StatusOperations;
            break;
        }
    }
    return DNSSD_OK;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, DNSSD_BrowseA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(CONST_STRPTR, type, A1),
        AROS_LHA(DNSSDOperationRef *, operation, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 12, DNSSD)

/*  FUNCTION
        Finds the instances of a service type ("_ipp._tcp") on the links,
        until cancelled: DNSSD_EVENT_ADD for each instance on each link
        (de_Instance, de_Type, de_Domain, de_Interface), DNSSD_EVENT_REMOVE
        when it goes. What mdnsd knows already comes at once.

    INPUTS
        DNSSD_Domain - default "local"
        DNSSD_Subtype - only the instances of this subtype ("_printer")
        DNSSD_Interface - only this interface index
        DNSSD_UserData - returned in the operation's events

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    UBYTE wire[256];
    LONG length;

    if (session == NULL || type == NULL || operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    init_msg(&msg, session, DNSSD_IPC_BROWSE);
    length = service_to_wire((CONST_STRPTR)GetTagData(DNSSD_Subtype, 0, tags),
        type, (CONST_STRPTR)GetTagData(DNSSD_Domain, (IPTR)"local", tags),
        wire, 0, &msg.dm_TypeOffset, &msg.dm_DomainOffset);
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    msg.dm_Data = wire;
    msg.dm_DataSize = length;
    return start(session, &msg, tags, operation);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(LONG, DNSSD_ResolveA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(CONST_STRPTR, instance, A1),
        AROS_LHA(CONST_STRPTR, type, A2),
        AROS_LHA(DNSSDOperationRef *, operation, A3),
        AROS_LHA(struct TagItem *, tags, A4),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 13, DNSSD)

/*  FUNCTION
        What an instance (its own label, as Browse gave it in de_Instance)
        of a type offers: DNSSD_EVENT_RESOLVED with the host (de_Target),
        port, SRV priority and weight, the TXT record's data and every
        address of the host known, each with its interface - again when
        any of that changes, DNSSD_EVENT_REMOVE if the service goes. When
        the time is up the operation ends (DNSSD_EVENT_END: DNSSD_OK if it
        was resolved, DNSSD_ERR_TIMEOUT if not).

    INPUTS
        DNSSD_Domain - default "local"
        DNSSD_Interface - only on this interface index
        DNSSD_Timeout - seconds, 1 to 60, default 5
        DNSSD_UserData - returned in the operation's events

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    UBYTE wire[256];
    LONG length;

    if (session == NULL || instance == NULL || type == NULL ||
        operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    init_msg(&msg, session, DNSSD_IPC_RESOLVE);
    length = label_to_wire(instance, wire, 0);
    if (length >= 0) {
        length = service_to_wire(NULL, type,
            (CONST_STRPTR)GetTagData(DNSSD_Domain, (IPTR)"local", tags),
            wire, length, &msg.dm_TypeOffset, &msg.dm_DomainOffset);
    }
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    msg.dm_Data = wire;
    msg.dm_DataSize = length;
    msg.dm_Timeout = GetTagData(DNSSD_Timeout, 5, tags);
    if (msg.dm_Timeout < 1 || msg.dm_Timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    return start(session, &msg, tags, operation);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH5(LONG, DNSSD_QueryRecordA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(CONST_STRPTR, name, A1),
        AROS_LHA(ULONG, rrtype, D0),
        AROS_LHA(DNSSDOperationRef *, operation, A2),
        AROS_LHA(struct TagItem *, tags, A3),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 14, DNSSD)

/*  FUNCTION
        The records of a name and type (255 for every type) on the links:
        DNSSD_EVENT_ADD for each (de_RRType, de_RData with the names in it
        uncompressed, de_TTL, de_Interface), DNSSD_EVENT_REMOVE when it
        goes - until cancelled, or until DNSSD_Timeout.

    INPUTS
        DNSSD_Interface - only on this interface index
        DNSSD_Timeout - seconds, 1 to 60; none by default
        DNSSD_UserData - returned in the operation's events

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DNSSDIPCMsg msg;
    UBYTE wire[256];
    LONG length;

    if (session == NULL || name == NULL || operation == NULL ||
        rrtype == 0 || rrtype > 0xffff) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    length = text_to_wire(name, wire, 0);
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    init_msg(&msg, session, DNSSD_IPC_QUERY);
    msg.dm_Data = wire;
    msg.dm_DataSize = length;
    msg.dm_RRType = (UWORD)rrtype;
    msg.dm_Timeout = GetTagData(DNSSD_Timeout, 0, tags);
    if (msg.dm_Timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    return start(session, &msg, tags, operation);
    AROS_LIBFUNC_EXIT
}
