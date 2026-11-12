/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * dnssd_server.c - dnssd.library's sessions and operations in mdnsd
 *
 * A session belongs to one client task. Its operations put their results
 * in the session's queue as finished events (struct DNSSDEvent and its
 * parts, in mdnsd's memory); the client copies them out one at a time
 * (DNSSD_IPC_READ). When the queue goes from empty to not empty mdnsd
 * replies to the notification message the session lent it, and the client,
 * woken on its port, reads until the queue is empty, which lends the
 * message again. Nothing of the client's is kept but that message, and it
 * is only replied to while the client task exists.
 *
 * Sessions and operations are slots named by a generation and the slot
 * (dnssd_ipc.h), under the daemon's incarnation: a name from before a
 * restart, or of a slot used since, is refused.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <string.h>

#include "mdnsd.h"

#define SERVER_SESSIONS         16
#define SERVER_OPERATIONS       32
#define SESSION_OPERATIONS      8      /* one session's at a time */
#define SESSION_EVENTS          64     /* queued for one session */
#define LOOKUP_ADDRESSES        16     /* distinct addresses one lookup reports */

#define OP_LOOKUP               1

struct QueuedEvent
{
    struct MinNode  qe_Node;
    ULONG           qe_Size;
    /* the event follows */
};

struct ServerSession
{
    UWORD           ss_Generation;
    UBYTE           ss_Active;
    UBYTE           ss_Lost;            /* events were dropped */
    ULONG           ss_Task;
    struct Message *ss_Notify;          /* lent, not yet replied to */
    struct MinList  ss_Queue;
    ULONG           ss_Queued;
    ULONG           ss_Operations;
};

struct ServerOperation
{
    UWORD           so_Generation;
    UBYTE           so_Active;
    UBYTE           so_Kind;
    UWORD           so_Session;         /* slot */
    UBYTE           so_Pending;         /* questions still asked */
    UBYTE           so_Found;           /* addresses reported */
    ULONG           so_Sequence;
    ULONG           so_Interface;       /* 0: all */
    IPTR            so_UserData;
    APTR            so_Query[2];
    struct DNSSDAddress so_Seen[LOOKUP_ADDRESSES];
    struct MDNSDaemon *so_Daemon;
};

struct DNSSDServer
{
    UWORD                   sv_NextGeneration;
    struct ServerSession    sv_Sessions[SERVER_SESSIONS];
    struct ServerOperation  sv_Operations[SERVER_OPERATIONS];
};

static UWORD
next_generation(struct DNSSDServer *sv)
{
    if (++sv->sv_NextGeneration == 0) {
        sv->sv_NextGeneration = 1;
    }
    return sv->sv_NextGeneration;
}

static ULONG
make_id(UWORD generation, LONG slot)
{
    return ((ULONG)generation << 16) | (ULONG)(slot + 1);
}

static LONG
mdns_error(LONG rc)
{
    switch (rc) {
    case MDNS_OK:           return DNSSD_OK;
    case MDNSERR_NOMEM:     return DNSSD_ERR_NOMEM;
    case MDNSERR_BUSY:      return DNSSD_ERR_BUSY;
    case MDNSERR_TIMEOUT:   return DNSSD_ERR_TIMEOUT;
    case MDNSERR_CANCELLED: return DNSSD_ERR_CANCELLED;
    case MDNSERR_NOENGINE:  return DNSSD_ERR_NOENGINE;
    default:                return DNSSD_ERR_BADPARAM;
    }
}

static struct ServerSession *
find_session(struct MDNSDaemon *md, const struct DNSSDIPCMsg *msg)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    ULONG slot = (msg->dm_Session & 0xffff) - 1;
    struct ServerSession *ss;

    if (sv == NULL || msg->dm_Incarnation != md->md_DaemonID ||
        slot >= SERVER_SESSIONS) {
        return NULL;
    }
    ss = &sv->sv_Sessions[slot];
    if (!ss->ss_Active || ss->ss_Generation != (msg->dm_Session >> 16) ||
        ss->ss_Task != msg->dm_TaskID) {
        return NULL;
    }
    return ss;
}

/* The client has something to read: its notification goes back to it. */
static void
notify(struct ServerSession *ss)
{
    struct Message *message = ss->ss_Notify;

    if (message == NULL) {
        return;
    }
    ss->ss_Notify = NULL;
    if (FindTaskByPID(ss->ss_Task) != NULL) {
        ReplyMsg(message);
    }
}

static void
drop_queue(struct ServerSession *ss)
{
    struct QueuedEvent *qe;

    while ((qe = (struct QueuedEvent *)RemHead((struct List *)&ss->ss_Queue))
            != NULL) {
        FreeVec(qe);
    }
    ss->ss_Queued = 0;
}

/*
 * Queue an event of op: the header, the name (len bytes, not terminated)
 * and count addresses. A queue already full drops what it holds: the
 * client reads DNSSD_EVENT_LOST and starts again.
 */
static void
push_event(struct DNSSDServer *sv, struct ServerOperation *op, UWORD kind,
    LONG result, ULONG ifindex, ULONG ttl, UWORD rrtype, const char *name,
    ULONG len, const struct DNSSDAddress *addresses, UWORD count)
{
    struct ServerSession *ss = &sv->sv_Sessions[op->so_Session];
    struct QueuedEvent *qe;
    struct DNSSDEvent *event;
    ULONG size = sizeof(*event) + len;
    ULONG addrOffset;

    addrOffset = (size + 3) & ~3UL;
    size = addrOffset + (ULONG)count * sizeof(*addresses);
    if (ss->ss_Queued >= SESSION_EVENTS) {
        drop_queue(ss);
        ss->ss_Lost = 1;
        notify(ss);
        return;
    }
    qe = AllocVec(sizeof(*qe) + size, MEMF_CLEAR);
    if (qe == NULL) {
        drop_queue(ss);
        ss->ss_Lost = 1;
        notify(ss);
        return;
    }
    qe->qe_Size = size;
    event = (struct DNSSDEvent *)(qe + 1);
    event->de_Size = size;
    event->de_Version = DNSSD_EVENT_VERSION;
    event->de_Kind = kind;
    event->de_Result = result;
    event->de_Sequence = ++op->so_Sequence;
    event->de_Interface = ifindex;
    event->de_TTL = ttl;
    event->de_RRType = rrtype;
    event->de_AddressCount = count;
    event->de_UserData = op->so_UserData;
    if (len != 0) {
        event->de_Name.dr_Offset = sizeof(*event);
        event->de_Name.dr_Size = len;
        memcpy((UBYTE *)event + sizeof(*event), name, len);
    }
    if (count != 0) {
        event->de_Addresses.dr_Offset = addrOffset;
        event->de_Addresses.dr_Size = (ULONG)count * sizeof(*addresses);
        memcpy((UBYTE *)event + addrOffset, addresses,
            (ULONG)count * sizeof(*addresses));
    }
    AddTail((struct List *)&ss->ss_Queue, (struct Node *)qe);
    if (ss->ss_Queued++ == 0) {
        notify(ss);
    }
}

static void
end_operation(struct DNSSDServer *sv, struct ServerOperation *op)
{
    struct ServerSession *ss = &sv->sv_Sessions[op->so_Session];
    LONG i;

    for (i = 0; i < 2; i++) {
        if (op->so_Query[i] != NULL) {
            mdnsQueryStopSink(op->so_Daemon, op->so_Query[i]);
            op->so_Query[i] = NULL;
        }
    }
    if (ss->ss_Operations != 0) {
        ss->ss_Operations--;
    }
    op->so_Active = 0;
}

/* A record answering a lookup's question `which' (0 A, 1 AAAA), or (record
 * NULL) the end of that question. */
static void
lookup_answer(struct ServerOperation *op, LONG which,
    const struct MDNSRecordEvent *record)
{
    struct DNSSDServer *sv = op->so_Daemon->md_DNSSD;
    struct DNSSDAddress address;
    LONG i;

    if (!op->so_Active) {
        return;
    }
    if (record == NULL) {
        /* the question's session is gone already */
        op->so_Query[which] = NULL;
        if (op->so_Pending != 0 && --op->so_Pending == 0) {
            push_event(sv, op, DNSSD_EVENT_END,
                op->so_Found != 0 ? DNSSD_OK : DNSSD_ERR_TIMEOUT,
                0, 0, 0, NULL, 0, NULL, 0);
            end_operation(sv, op);
        }
        return;
    }
    memset(&address, 0, sizeof(address));
    if (record->re_Type == DNS_TYPE_A && record->re_DataLength == 4) {
        address.da_Family = DNSSD_FAMILY_IPV4;
        address.da_Length = 4;
    } else if (record->re_Type == DNS_TYPE_AAAA &&
               record->re_DataLength == 16) {
        address.da_Family = DNSSD_FAMILY_IPV6;
        address.da_Length = 16;
    } else {
        return;
    }
    if (op->so_Interface != 0 &&
        op->so_Interface != record->re_InterfaceIndex) {
        return;
    }
    address.da_Interface = record->re_InterfaceIndex;
    memcpy(address.da_Bytes, record->re_Data, address.da_Length);
    for (i = 0; i < op->so_Found; i++) {
        if (memcmp(&op->so_Seen[i], &address, sizeof(address)) == 0) {
            return;                     /* said already */
        }
    }
    if (op->so_Found < LOOKUP_ADDRESSES) {
        op->so_Seen[op->so_Found++] = address;
        push_event(sv, op, DNSSD_EVENT_ADDRESS, DNSSD_OK,
            address.da_Interface, record->re_TTL, record->re_Type,
            record->re_Name, strlen(record->re_Name), &address, 1);
    }
}

static void
lookup_sink_a(APTR data, const struct MDNSRecordEvent *record, LONG result)
{
    lookup_answer((struct ServerOperation *)data, 0, record);
}

static void
lookup_sink_aaaa(APTR data, const struct MDNSRecordEvent *record, LONG result)
{
    lookup_answer((struct ServerOperation *)data, 1, record);
}

static LONG
start_lookup(struct MDNSDaemon *md, struct ServerSession *ss, LONG sslot,
    struct DNSSDIPCMsg *msg)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    struct ServerOperation *op = NULL;
    char name[256];
    ULONG timeout = msg->dm_Timeout != 0 ? msg->dm_Timeout : 3;
    LONG i, rc = MDNS_OK;

    if (msg->dm_Data == NULL || msg->dm_DataSize == 0 ||
        msg->dm_DataSize >= sizeof(name) || timeout > 60 ||
        (msg->dm_Family != DNSSD_FAMILY_ANY &&
         msg->dm_Family != DNSSD_FAMILY_IPV4 &&
         msg->dm_Family != DNSSD_FAMILY_IPV6)) {
        return DNSSD_ERR_BADPARAM;
    }
    memcpy(name, msg->dm_Data, msg->dm_DataSize);
    name[msg->dm_DataSize] = '\0';
    if (ss->ss_Operations >= SESSION_OPERATIONS) {
        return DNSSD_ERR_BUSY;
    }
    for (i = 0; i < SERVER_OPERATIONS; i++) {
        if (!sv->sv_Operations[i].so_Active) {
            op = &sv->sv_Operations[i];
            break;
        }
    }
    if (op == NULL) {
        return DNSSD_ERR_BUSY;
    }
    memset(op, 0, sizeof(*op));
    op->so_Generation = next_generation(sv);
    op->so_Active = 1;
    op->so_Kind = OP_LOOKUP;
    op->so_Session = (UWORD)sslot;
    op->so_Interface = msg->dm_Interface;
    op->so_UserData = msg->dm_UserData;
    op->so_Daemon = md;
    ss->ss_Operations++;
    msg->dm_Operation = make_id(op->so_Generation, i);
    /* both questions are counted before either is asked: an own-host answer
     * comes back during the start */
    op->so_Pending = msg->dm_Family == DNSSD_FAMILY_ANY ? 2 : 1;
    if (msg->dm_Family != DNSSD_FAMILY_IPV6) {
        rc = mdnsQueryStartSink(md, name, DNS_TYPE_A, (UWORD)timeout,
            lookup_sink_a, op, &op->so_Query[0]);
    }
    if (rc == MDNS_OK && msg->dm_Family != DNSSD_FAMILY_IPV4) {
        rc = mdnsQueryStartSink(md, name, DNS_TYPE_AAAA, (UWORD)timeout,
            lookup_sink_aaaa, op, &op->so_Query[1]);
    }
    if (rc != MDNS_OK) {
        end_operation(sv, op);
        msg->dm_Operation = 0;
        return mdns_error(rc);
    }
    return DNSSD_OK;
}

static void
close_session(struct DNSSDServer *sv, struct ServerSession *ss, LONG slot)
{
    LONG i;

    for (i = 0; i < SERVER_OPERATIONS; i++) {
        struct ServerOperation *op = &sv->sv_Operations[i];

        if (op->so_Active && op->so_Session == slot) {
            end_operation(sv, op);
        }
    }
    drop_queue(ss);
    ss->ss_Notify = NULL;           /* given back: the client has it */
    ss->ss_Active = 0;
}

/* The READ of a session: the next event, or the notification lent again. */
static LONG
read_event(struct ServerSession *ss, struct DNSSDIPCMsg *msg)
{
    struct QueuedEvent *qe;

    if (ss->ss_Lost) {
        struct DNSSDEvent lost;

        if (msg->dm_DataMax < sizeof(lost) || msg->dm_Data == NULL) {
            msg->dm_DataSize = sizeof(lost);
            return DNSSD_ERR_TOOSMALL;
        }
        memset(&lost, 0, sizeof(lost));
        lost.de_Size = sizeof(lost);
        lost.de_Version = DNSSD_EVENT_VERSION;
        lost.de_Kind = DNSSD_EVENT_LOST;
        memcpy(msg->dm_Data, &lost, sizeof(lost));
        msg->dm_DataSize = sizeof(lost);
        ss->ss_Lost = 0;
        return DNSSD_OK;
    }
    qe = (struct QueuedEvent *)ss->ss_Queue.mlh_Head;
    if (qe->qe_Node.mln_Succ == NULL) {
        if (ss->ss_Notify == NULL && msg->dm_Notify != NULL) {
            ss->ss_Notify = msg->dm_Notify;
        }
        return DNSSD_ERR_EMPTY;
    }
    msg->dm_DataSize = qe->qe_Size;
    if (msg->dm_Data == NULL || msg->dm_DataMax < qe->qe_Size) {
        return DNSSD_ERR_TOOSMALL;
    }
    memcpy(msg->dm_Data, qe + 1, qe->qe_Size);
    Remove((struct Node *)qe);
    FreeVec(qe);
    ss->ss_Queued--;
    return DNSSD_OK;
}

void
dnssdServerMessage(struct MDNSDaemon *md, struct DNSSDIPCMsg *msg)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    struct ServerSession *ss;
    LONG i, slot;

    if (msg->dm_Version != DNSSD_IPC_VERSION ||
        msg->dm_Msg.mn_Length != sizeof(*msg) || msg->dm_TaskID == 0) {
        msg->dm_Result = DNSSD_ERR_BADPARAM;
        return;
    }
    if (sv == NULL) {
        sv = AllocMem(sizeof(*sv), MEMF_CLEAR);
        if (sv == NULL) {
            msg->dm_Result = DNSSD_ERR_NOMEM;
            return;
        }
        for (i = 0; i < SERVER_SESSIONS; i++) {
            NEWLIST((struct List *)&sv->sv_Sessions[i].ss_Queue);
        }
        md->md_DNSSD = sv;
    }
    if (msg->dm_Command == DNSSD_IPC_OPEN) {
        msg->dm_Result = DNSSD_ERR_BUSY;
        if (msg->dm_Notify == NULL) {
            msg->dm_Result = DNSSD_ERR_BADPARAM;
            return;
        }
        for (i = 0; i < SERVER_SESSIONS; i++) {
            ss = &sv->sv_Sessions[i];
            if (!ss->ss_Active) {
                ss->ss_Active = 1;
                ss->ss_Lost = 0;
                ss->ss_Generation = next_generation(sv);
                ss->ss_Task = msg->dm_TaskID;
                ss->ss_Notify = msg->dm_Notify;
                ss->ss_Queued = 0;
                ss->ss_Operations = 0;
                msg->dm_Session = make_id(ss->ss_Generation, i);
                msg->dm_Incarnation = md->md_DaemonID;
                msg->dm_Result = DNSSD_OK;
                break;
            }
        }
        return;
    }
    if (msg->dm_Command == DNSSD_IPC_STATUS) {
        msg->dm_Incarnation = md->md_DaemonID;
        msg->dm_StatusSessions = 0;
        msg->dm_StatusOperations = 0;
        for (i = 0; i < SERVER_SESSIONS; i++) {
            msg->dm_StatusSessions += sv->sv_Sessions[i].ss_Active;
        }
        for (i = 0; i < SERVER_OPERATIONS; i++) {
            msg->dm_StatusOperations += sv->sv_Operations[i].so_Active;
        }
        msg->dm_Result = DNSSD_OK;
        return;
    }
    ss = find_session(md, msg);
    if (ss == NULL) {
        msg->dm_Result = DNSSD_ERR_STALE;
        return;
    }
    slot = ss - sv->sv_Sessions;
    switch (msg->dm_Command) {
    case DNSSD_IPC_CLOSE:
        close_session(sv, ss, slot);
        msg->dm_Result = DNSSD_OK;
        break;
    case DNSSD_IPC_READ:
        msg->dm_Result = read_event(ss, msg);
        break;
    case DNSSD_IPC_LOOKUP:
        msg->dm_Result = start_lookup(md, ss, slot, msg);
        break;
    case DNSSD_IPC_CANCEL: {
        ULONG oslot = (msg->dm_Operation & 0xffff) - 1;
        struct ServerOperation *op;

        msg->dm_Result = DNSSD_OK;          /* idempotent: gone is cancelled */
        if (oslot >= SERVER_OPERATIONS) {
            msg->dm_Result = DNSSD_ERR_BADPARAM;
            break;
        }
        op = &sv->sv_Operations[oslot];
        if (op->so_Active && op->so_Session == slot &&
            op->so_Generation == (msg->dm_Operation >> 16)) {
            end_operation(sv, op);
        }
        break;
    }
    default:
        msg->dm_Result = DNSSD_ERR_BADPARAM;
        break;
    }
}

/* Sessions of tasks that ended without closing them. */
void
dnssdServerReap(struct MDNSDaemon *md)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    LONG i;

    if (sv == NULL) {
        return;
    }
    for (i = 0; i < SERVER_SESSIONS; i++) {
        struct ServerSession *ss = &sv->sv_Sessions[i];

        if (ss->ss_Active && FindTaskByPID(ss->ss_Task) == NULL) {
            close_session(sv, ss, i);
        }
    }
}

/* mdnsd is ending: every session that waits is woken, to find it gone. */
void
dnssdServerCancel(struct MDNSDaemon *md)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    LONG i;

    if (sv == NULL) {
        return;
    }
    for (i = 0; i < SERVER_SESSIONS; i++) {
        if (sv->sv_Sessions[i].ss_Active) {
            notify(&sv->sv_Sessions[i]);
        }
    }
}

/* mdnsd ends: no client message is replied to any more. */
void
dnssdServerFree(struct MDNSDaemon *md)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    LONG i;

    if (sv == NULL) {
        return;
    }
    for (i = 0; i < SERVER_SESSIONS; i++) {
        if (sv->sv_Sessions[i].ss_Active) {
            close_session(sv, &sv->sv_Sessions[i], i);
        }
    }
    FreeMem(sv, sizeof(*sv));
    md->md_DNSSD = NULL;
}
