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
#include "dnssd_model.h"

/*
 * A request of command with parts, of session ds (NULL: none yet, or none
 * needed), in a block of its own (dnssd_ipc_port.h); NULL and *rc if it
 * cannot be made. Its data is copied in: nothing of the caller's is lent.
 */
struct DNSSDIPCMsg *
dnssdNewMsg(UWORD command, const struct dnssd_ipc_parts *parts,
    const struct DNSSDSession *ds, LONG *rc)
{
    static const struct dnssd_ipc_parts none;
    const ULONG head = offsetof(struct DNSSDIPCMsg, dm_Request);
    struct DNSSDIPCMsg *msg;
    size_t need, written;

    if (parts == NULL) {
        parts = &none;
    }
    if (dnssd_ipc_measure(parts, &need) != DNSSD_IPC_OK || head + need > 0xffff) {
        *rc = DNSSD_ERR_BADPARAM;
        return NULL;
    }
    msg = AllocVec(head + need, MEMF_PUBLIC | MEMF_CLEAR);
    if (msg == NULL) {
        *rc = DNSSD_ERR_NOMEM;
        return NULL;
    }
    if (dnssd_ipc_build(command, parts, &msg->dm_Request, need, &written) !=
            DNSSD_IPC_OK) {
        FreeVec(msg);
        *rc = DNSSD_ERR_BADPARAM;
        return NULL;
    }
    msg->dm_Msg.mn_Length = (UWORD)(head + written);
    if (ds != NULL) {
        msg->dm_Request.session = ds->ds_Session;
        msg->dm_Request.incarnation = ds->ds_Incarnation;
    }
    *rc = DNSSD_OK;
    return msg;
}

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
    msg->dm_Msg.mn_ReplyPort = reply;
    msg->dm_Request.task = GetETaskID(FindTask(NULL));
    msg->dm_Request.result = DNSSD_ERR_NOENGINE;
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
    return msg->dm_Request.result;
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

/* The DNSSD_HostAddress tags, each a struct DNSSDAddress *, into addresses
 * (at most DNSSD_HOST_ADDRESSES); how many, or -1 if there are more or one
 * is NULL. */
static LONG
host_addresses(struct TagItem *tags, struct DNSSDAddress *addresses)
{
    struct TagItem *tstate = tags, *tag;
    LONG count = 0;

    while ((tag = NextTagItem(&tstate)) != NULL) {
        if (tag->ti_Tag != DNSSD_HostAddress) {
            continue;
        }
        if (tag->ti_Data == 0 || count == DNSSD_HOST_ADDRESSES) {
            return -1;
        }
        addresses[count++] = *(const struct DNSSDAddress *)tag->ti_Data;
    }
    return count;
}

/* An operation of session from msg (NULL: rc says why it was not made),
 * which names it and is freed here; the tags every one takes. */
static LONG
start(struct DNSSDSession *session, struct DNSSDIPCMsg *msg, LONG rc,
    struct TagItem *tags, DNSSDOperationRef *operation)
{
    struct DNSSDOperation *op;
    ULONG id;

    if (msg == NULL) {
        return rc;
    }
    op = AllocVec(sizeof(*op), MEMF_PUBLIC | MEMF_CLEAR);
    if (op == NULL) {
        FreeVec(msg);
        return DNSSD_ERR_NOMEM;
    }
    msg->dm_Request.interface_index = GetTagData(DNSSD_Interface, 0, tags);
    rc = dnssdSend(msg);
    id = msg->dm_Request.operation;
    FreeVec(msg);
    if (rc != DNSSD_OK) {
        FreeVec(op);
        return rc;
    }
    op->do_Session = session;
    op->do_ID = id;
    AddTail((struct List *)&session->ds_Operations, (struct Node *)op);
    *operation = op;
    return DNSSD_OK;
}

/*
 * A service's type and domain (text) as wire names, in the parts; their
 * bytes in typeWire and domainWire. DNSSD_ERR_BADNAME if either is no name.
 */
static LONG
service_parts(CONST_STRPTR type, CONST_STRPTR domain, UBYTE *typeWire,
    UBYTE *domainWire, struct dnssd_ipc_parts *parts)
{
    LONG t = text_to_wire(type, typeWire, 0);
    LONG d = text_to_wire(domain, domainWire, 0);

    if (t < 0 || d < 0) {
        return DNSSD_ERR_BADNAME;
    }
    parts->service_type = typeWire;
    parts->service_type_size = (ULONG)t;
    parts->domain = domainWire;
    parts->domain_size = (ULONG)d;
    return DNSSD_OK;
}

/* DNSSD_HostAddress's struct DNSSDAddress is sent as it is */
typedef char address_layout_check[
    sizeof(struct DNSSDAddress) == sizeof(struct dnssd_model_wire_address) &&
    offsetof(struct DNSSDAddress, da_Interface) ==
        offsetof(struct dnssd_model_wire_address, interface_index) ? 1 : -1];

/* An instance's own label (UTF-8, dots allowed, 1 to 63 bytes), as it is. */
static LONG
instance_part(CONST_STRPTR instance, struct dnssd_ipc_parts *parts)
{
    ULONG length = strnlen(instance, 64);

    if (length == 0 || length > 63) {
        return DNSSD_ERR_BADNAME;
    }
    parts->instance = (const UBYTE *)instance;
    parts->instance_size = length;
    return DNSSD_OK;
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
    struct DNSSDIPCMsg *msg;
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
    msg = dnssdNewMsg(DNSSD_IPC_OPEN, NULL, NULL, &rc);
    if (msg != NULL) {
        msg->dm_Notify = &ds->ds_Notify;
        rc = dnssdSend(msg);
        ds->ds_Session = msg->dm_Request.session;
        ds->ds_Incarnation = msg->dm_Request.incarnation;
        FreeVec(msg);
    }
    if (rc != DNSSD_OK) {
        FreeVec(ds);
        return rc;
    }
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
    struct DNSSDIPCMsg *msg;
    struct DNSSDOperation *op;
    LONG rc;

    if (session == NULL) {
        return;
    }
    msg = dnssdNewMsg(DNSSD_IPC_CLOSE, NULL, session, &rc);
    if (msg != NULL) {
        dnssdSend(msg);
        FreeVec(msg);
    }
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
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    ULONG room;
    LONG rc;

    if (session == NULL || size == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    /* mdnsd writes the event into the request's block, then it is copied
     * out. The room is never more than the caller has: an event that does
     * not fit stays mdnsd's, and the reply says its size (a caller with no
     * buffer asks just that) */
    room = event != NULL ? (*size & ~3UL) : 0;
    if (room > DNSSD_IPC_MAX_SIZE - 512) {
        room = DNSSD_IPC_MAX_SIZE - 512;
    }
    memset(&parts, 0, sizeof(parts));
    parts.reply_room = room != 0 ? room : 4;
    msg = dnssdNewMsg(DNSSD_IPC_READ, &parts, session, &rc);
    if (msg == NULL) {
        return rc;
    }
    /* lent again only if mdnsd finds nothing to read */
    if (notify_back(session)) {
        msg->dm_Notify = &session->ds_Notify;
    }
    rc = dnssdSend(msg);
    if (rc == DNSSD_ERR_EMPTY && msg->dm_Notify != NULL) {
        session->ds_Notify.mn_Node.ln_Type = NT_MESSAGE;
    }
    if (rc == DNSSD_OK) {
        CopyMem((UBYTE *)&msg->dm_Request + msg->dm_Request.reply.offset, event,
            msg->dm_Request.reply.size);
    }
    if (rc == DNSSD_OK || rc == DNSSD_ERR_TOOSMALL) {
        *size = msg->dm_Request.reply.size;
    }
    FreeVec(msg);
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
    struct DNSSDIPCMsg *msg;
    LONG rc;

    if (operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    msg = dnssdNewMsg(DNSSD_IPC_CANCEL, NULL, operation->do_Session, &rc);
    if (msg != NULL) {
        msg->dm_Request.operation = operation->do_ID;
        rc = dnssdSend(msg);
        FreeVec(msg);
    }
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

    RESULT
        DNSSD_OK and *operation, or an error.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    UBYTE wire[256];
    ULONG timeout = GetTagData(DNSSD_Timeout, 3, tags);
    LONG length, rc;

    if (session == NULL || name == NULL || operation == NULL ||
        timeout < 1 || timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    length = text_to_wire(name, wire, 0);
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    memset(&parts, 0, sizeof(parts));
    parts.name = wire;
    parts.name_size = (ULONG)length;
    msg = dnssdNewMsg(DNSSD_IPC_LOOKUP, &parts, session, &rc);
    if (msg != NULL) {
        msg->dm_Request.family = GetTagData(DNSSD_Family, DNSSD_FAMILY_ANY, tags);
        msg->dm_Request.timeout = timeout;
    }
    return start(session, msg, rc, tags, operation);
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
    struct DNSSDIPCMsg *msg;
    struct TagItem *tstate = tags, *tag;
    ULONG incarnation, sessions, operations;
    LONG rc;

    msg = dnssdNewMsg(DNSSD_IPC_STATUS, NULL, NULL, &rc);
    if (msg == NULL) {
        return rc;
    }
    rc = dnssdSend(msg);
    incarnation = msg->dm_Request.incarnation;
    sessions = msg->dm_Request.count[0];
    operations = msg->dm_Request.count[1];
    FreeVec(msg);
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
            *to = incarnation;
            break;
        case DNSSD_Status_Sessions:
            *to = sessions;
            break;
        case DNSSD_Status_Operations:
            *to = operations;
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    UBYTE typeWire[256], domainWire[256], subtype[64];
    CONST_STRPTR sub = (CONST_STRPTR)GetTagData(DNSSD_Subtype, 0, tags);
    LONG rc;

    if (session == NULL || type == NULL || operation == NULL) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    memset(&parts, 0, sizeof(parts));
    rc = service_parts(type, (CONST_STRPTR)GetTagData(DNSSD_Domain, (IPTR)"local", tags),
        typeWire, domainWire, &parts);
    if (rc != DNSSD_OK) {
        return rc;
    }
    if (sub != NULL) {
        LONG length = label_to_wire(sub, subtype, 0);

        if (length < 0) {
            return DNSSD_ERR_BADNAME;
        }
        parts.subtypes = subtype;
        parts.subtypes_size = (ULONG)length;
    }
    msg = dnssdNewMsg(DNSSD_IPC_BROWSE, &parts, session, &rc);
    return start(session, msg, rc, tags, operation);
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
        DNSSD_Timeout - seconds, 1 to 60, default 5; 0: no time limit, the
            service is followed until the operation is cancelled

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    UBYTE typeWire[256], domainWire[256];
    ULONG timeout = GetTagData(DNSSD_Timeout, 5, tags);
    LONG rc;

    if (session == NULL || instance == NULL || type == NULL ||
        operation == NULL || timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    memset(&parts, 0, sizeof(parts));
    if ((rc = instance_part(instance, &parts)) != DNSSD_OK ||
        (rc = service_parts(type,
            (CONST_STRPTR)GetTagData(DNSSD_Domain, (IPTR)"local", tags),
            typeWire, domainWire, &parts)) != DNSSD_OK) {
        return rc;
    }
    msg = dnssdNewMsg(DNSSD_IPC_RESOLVE, &parts, session, &rc);
    if (msg != NULL) {
        msg->dm_Request.timeout = timeout;
        if (timeout == 0) {
            msg->dm_Request.flags = DNSSD_IPC_FLAG_UNTIL_CANCELLED;
        }
    }
    return start(session, msg, rc, tags, operation);
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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    UBYTE wire[256];
    ULONG timeout = GetTagData(DNSSD_Timeout, 0, tags);
    LONG length, rc;

    if (session == NULL || name == NULL || operation == NULL ||
        rrtype == 0 || rrtype > 0xffff || timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    length = text_to_wire(name, wire, 0);
    if (length < 0) {
        return DNSSD_ERR_BADNAME;
    }
    memset(&parts, 0, sizeof(parts));
    parts.name = wire;
    parts.name_size = (ULONG)length;
    msg = dnssdNewMsg(DNSSD_IPC_QUERY, &parts, session, &rc);
    if (msg != NULL) {
        msg->dm_Request.rrtype = (UWORD)rrtype;
        msg->dm_Request.timeout = timeout;
    }
    return start(session, msg, rc, tags, operation);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH6(LONG, DNSSD_RegisterA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDSessionRef, session, A0),
        AROS_LHA(CONST_STRPTR, instance, A1),
        AROS_LHA(CONST_STRPTR, type, A2),
        AROS_LHA(ULONG, port, D0),
        AROS_LHA(DNSSDOperationRef *, operation, A3),
        AROS_LHA(struct TagItem *, tags, A4),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 15, DNSSD)

/*  FUNCTION
        Publishes a service of this host: instance (its own label, UTF-8,
        dots allowed, at most 63 bytes) of type ("_ipp._tcp") on port.
        mdnsd probes for the name and announces it (RFC 6762 section 8):
        DNSSD_EVENT_REGISTERED says the name it has - another one ("Name
        (2)") if another host had it, and again if a conflict renames it
        later. The service lasts as long as the operation: DNSSD_Cancel(),
        DNSSD_CloseSession() or the end of the task withdraw it, with its
        goodbyes.

    INPUTS
        DNSSD_TXT, DNSSD_TXTLength - its TXT data, length-prefixed strings
            (DNS-SD's "key=value" items, binary values allowed), at most
            DNSSD_TXT_MAX bytes - RFC 6763 section 6.2 asks for no more
            than 1300 where it can be; none: one empty string
        DNSSD_Subtype - a subtype it has ("_printer"), one tag each
        DNSSD_Domain - default "local"
        DNSSD_TimeToLive - seconds its records live in other hosts' caches,
            1-4500 (RFC 6762 section 10); default 120 for the SRV and the
            host's addresses, 4500 for the others
        DNSSD_Host, DNSSD_HostAddress - published for another host, one
            that does not speak mDNS itself: its name's label ("printer",
            which becomes "printer.local") and its addresses, IPv4 and
            IPv6, one tag each (an IPv6 link-local one with its
            da_Interface, and published on that link alone). Their A and
            AAAA records are probed, announced, answered and defended with
            the service's; if another host has the name, the operation
            ends with DNSSD_EVENT_END, DNSSD_ERR_CONFLICT.

    RESULT
        DNSSD_OK and *operation; DNSSD_ERR_EXISTS if this host has that
        instance of that type already; DNSSD_ERR_BADNAME, _BADPARAM.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    /* the request's parts, off the caller's stack (it may be 4 KB) */
    struct RegisterScratch {
        UBYTE typeWire[256], domainWire[256];
        UBYTE subtypes[DNSSD_IPC_MAX_SUBTYPES * 64];
        struct DNSSDAddress addresses[DNSSD_HOST_ADDRESSES];
    } *scratch;
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    struct TagItem *tstate = tags, *tag;
    const UBYTE *txt = (const UBYTE *)GetTagData(DNSSD_TXT, 0, tags);
    ULONG txtLength = GetTagData(DNSSD_TXTLength, 0, tags);
    CONST_STRPTR host = (CONST_STRPTR)GetTagData(DNSSD_Host, 0, tags);
    ULONG ttl = GetTagData(DNSSD_TimeToLive, 0, tags);
    ULONG at = 0, subtypes = 0;
    LONG count, rc;

    if (session == NULL || instance == NULL || type == NULL ||
        operation == NULL || port == 0 || port > 0xffff ||
        ttl > DNSSD_IPC_MAX_TTL ||
        (txt == NULL && txtLength != 0) || txtLength > DNSSD_TXT_MAX) {
        return DNSSD_ERR_BADPARAM;
    }
    *operation = NULL;
    scratch = AllocVec(sizeof(*scratch), MEMF_ANY);
    if (scratch == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    memset(&parts, 0, sizeof(parts));
    if ((rc = instance_part(instance, &parts)) != DNSSD_OK ||
        (rc = service_parts(type,
            (CONST_STRPTR)GetTagData(DNSSD_Domain, (IPTR)"local", tags),
            scratch->typeWire, scratch->domainWire, &parts)) != DNSSD_OK) {
        FreeVec(scratch);
        return rc;
    }
    while ((tag = NextTagItem(&tstate)) != NULL) {
        if (tag->ti_Tag == DNSSD_Subtype) {
            LONG next;

            if (subtypes++ == DNSSD_IPC_MAX_SUBTYPES ||
                (next = label_to_wire((CONST_STRPTR)tag->ti_Data,
                    scratch->subtypes + at, 0)) < 0) {
                FreeVec(scratch);
                return DNSSD_ERR_BADNAME;
            }
            at += next;
        }
    }
    if (at != 0) {
        parts.subtypes = scratch->subtypes;
        parts.subtypes_size = at;
    }
    if (txtLength != 0) {
        parts.txt = txt;
        parts.txt_size = txtLength;
    }
    count = host_addresses(tags, scratch->addresses);
    if (count < 0 || (host != NULL) != (count > 0) ||
        (host != NULL && (host[0] == '\0' || strnlen(host, 64) > 63))) {
        FreeVec(scratch);
        return DNSSD_ERR_BADPARAM;
    }
    if (host != NULL) {
        parts.host = (const UBYTE *)host;
        parts.host_size = strnlen(host, 64);
        parts.addresses = scratch->addresses;
        parts.address_count = (ULONG)count;
    }
    msg = dnssdNewMsg(DNSSD_IPC_REGISTER, &parts, session, &rc);
    FreeVec(scratch);
    if (msg != NULL) {
        msg->dm_Request.port = (UWORD)port;
        msg->dm_Request.ttl = ttl;
    }
    return start(session, msg, rc, tags, operation);
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, DNSSD_UpdateA,

/*  SYNOPSIS */
        AROS_LHA(DNSSDOperationRef, operation, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 16, DNSSD)

/*  FUNCTION
        Changes what a registered service (DNSSD_RegisterA()) says: its TXT
        data, its port, the addresses of the host it stands in for. What
        changed is announced, an address that goes with its goodbye; an
        update that changes nothing is not announced.

    INPUTS
        DNSSD_TXT, DNSSD_TXTLength - the new TXT data
        DNSSD_Port - the new port
        DNSSD_HostAddress - the host's addresses, all of them (one tag each)
        DNSSD_TimeToLive - another time to live for its records

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    static const UBYTE empty[1] = { 0 };    /* TXT of one empty string */
    struct dnssd_ipc_parts parts;
    struct DNSSDIPCMsg *msg;
    const UBYTE *txt = (const UBYTE *)GetTagData(DNSSD_TXT, 0, tags);
    ULONG txtLength = GetTagData(DNSSD_TXTLength, 0, tags);
    ULONG port = GetTagData(DNSSD_Port, 0, tags);
    ULONG ttl = GetTagData(DNSSD_TimeToLive, 0, tags);
    struct DNSSDAddress *addresses;
    LONG count, rc;

    if (operation == NULL || port > 0xffff || txtLength > DNSSD_TXT_MAX ||
        ttl > DNSSD_IPC_MAX_TTL ||
        (txt == NULL && txtLength != 0)) {
        return DNSSD_ERR_BADPARAM;
    }
    addresses = AllocVec(sizeof(*addresses) * DNSSD_HOST_ADDRESSES, MEMF_ANY);
    if (addresses == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    count = host_addresses(tags, addresses);
    if (count < 0) {
        FreeVec(addresses);
        return DNSSD_ERR_BADPARAM;
    }
    memset(&parts, 0, sizeof(parts));
    if (FindTagItem(DNSSD_TXT, tags) != NULL) {
        parts.txt = txtLength != 0 ? txt : empty;
        parts.txt_size = txtLength != 0 ? txtLength : sizeof(empty);
    }
    if (count > 0) {
        parts.addresses = addresses;
        parts.address_count = (ULONG)count;
    }
    msg = dnssdNewMsg(DNSSD_IPC_UPDATE, &parts, operation->do_Session, &rc);
    FreeVec(addresses);
    if (msg == NULL) {
        return rc;
    }
    msg->dm_Request.operation = operation->do_ID;
    msg->dm_Request.port = (UWORD)port;
    msg->dm_Request.ttl = ttl;
    rc = dnssdSend(msg);
    FreeVec(msg);
    return rc;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH4(LONG, DNSSD_NameToText,

/*  SYNOPSIS */
        AROS_LHA(CONST_APTR, name, A0),
        AROS_LHA(ULONG, size, D0),
        AROS_LHA(STRPTR, text, A1),
        AROS_LHA(ULONG, max, D1),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 17, DNSSD)

/*  FUNCTION
        A name of an event (DNS wire labels, size bytes: de_Name, de_Type,
        de_Domain, de_Target) as text: its labels with a dot between them,
        a dot or backslash inside a label written with a backslash before
        it (RFC 1035 section 5.1), terminated, in text (max bytes).

    RESULT
        The text's length, or -1 if name is not a name or text is too small.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    const UBYTE *wire = name;
    ULONG at = 0, n = 0;

    if (wire == NULL || text == NULL || max == 0) {
        return -1;
    }
    while (at < size && wire[at] != 0) {
        ULONG len = wire[at++], i;

        if (len > 63 || at + len > size) {
            return -1;
        }
        if (n != 0) {
            if (n + 1 >= max) {
                return -1;
            }
            text[n++] = '.';
        }
        for (i = 0; i < len; i++, at++) {
            if (wire[at] == '.' || wire[at] == '\\') {
                if (n + 1 >= max) {
                    return -1;
                }
                text[n++] = '\\';
            }
            if (n + 1 >= max) {
                return -1;
            }
            text[n++] = (char)wire[at];
        }
    }
    if (at >= size) {
        return -1;                      /* no root label */
    }
    text[n] = '\0';
    return (LONG)n;
    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1(ULONG, DNSSD_OperationID,

/*  SYNOPSIS */
        AROS_LHA(DNSSDOperationRef, operation, A0),

/*  LOCATION */
        struct DNSSDIntern *, DNSSDBase, 18, DNSSD)

/*  FUNCTION
        The number mdnsd gave the operation: the de_Operation of its events,
        so that a program with several operations on one session can tell
        whose an event is. 0 for NULL.

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return operation != NULL ? operation->do_ID : 0;
    AROS_LIBFUNC_EXIT
}
