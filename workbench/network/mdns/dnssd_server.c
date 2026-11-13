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

#include <stddef.h>

#include "mdnsd.h"
#include "dnssd_model.h"

#define SERVER_SESSIONS         16
#define SERVER_OPERATIONS       32
#define SESSION_OPERATIONS      8      /* one session's at a time */
#define SESSION_EVENTS          64     /* queued for one session */
#define LOOKUP_ADDRESSES        16     /* distinct addresses one lookup reports */

#define OP_LOOKUP               1
#define OP_BROWSE               2
#define OP_RESOLVE              3
#define OP_QUERY                4
#define OP_REGISTER             5


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
    ULONG           ss_Sequence;        /* events queued for it so far */
};

/*
 * An operation: the watches it keeps on the cache and what it has said.
 * Allocated while it runs (a resolve holds a TXT record).
 */
struct ServerOperation
{
    struct MDNSDaemon *so_Daemon;
    UWORD           so_Generation;
    UWORD           so_Slot;
    UBYTE           so_Kind;
    UBYTE           so_Found;           /* addresses in so_Seen */
    UWORD           so_Session;         /* slot */
    UWORD           so_RRType;
    UWORD           so_TypeOffset;      /* where the type starts in so_Name */
    UWORD           so_DomainOffset;
    UWORD           so_Port, so_Priority, so_Weight;
    UWORD           so_TXTLength;
    BOOL            so_HaveSRV, so_HaveTXT;
    BOOL            so_Enumerate;       /* Browse of _services._dns-sd._udp */
    ULONG           so_Sequence;
    ULONG           so_Interface;       /* 0: all */
    ULONG           so_Deadline;        /* mdnsTicks(), 0: none */
    ULONG           so_TTL;
    ULONG           so_Reported;        /* Resolve: signature of what was said */
    APTR            so_Watch[4];
    APTR            so_Service;         /* Register: mdnsRegAddNative()'s ref */
    UBYTE           so_Name[256];       /* wire labels */
    UBYTE           so_Target[256];     /* Resolve: the SRV's target */
    struct DNSSDAddress so_Seen[LOOKUP_ADDRESSES];
    UBYTE          *so_TXT;             /* Resolve: AllocVec()'d, so_TXTLength bytes */
};

struct DNSSDServer
{
    UWORD                   sv_NextGeneration;
    struct ServerSession    sv_Sessions[SERVER_SESSIONS];
    struct ServerOperation *sv_Operations[SERVER_OPERATIONS];
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
 * Every event is mdnsd's canonical message (dnssd_model.h), built by
 * dnssd_model_build() - which checks what each kind carries, and sorts and
 * deduplicates the addresses - and handed to the client as it is: struct
 * DNSSDEvent has its layout. The session's sequence and the daemon's ids
 * are filled in here. A queue already full drops what it holds: the client
 * reads DNSSD_EVENT_LOST and starts again.
 */
typedef char dnssd_event_is_the_model_message[
    sizeof(struct DNSSDEvent) == sizeof(struct dnssd_model_message) &&
    offsetof(struct DNSSDEvent, de_Addresses) ==
        offsetof(struct dnssd_model_message, addresses) &&
    sizeof(struct DNSSDAddress) == sizeof(struct dnssd_model_address) ? 1 : -1];

static struct dnssd_model_span
span(const void *data, ULONG size)
{
    struct dnssd_model_span s;

    s.data = data;
    s.size = size;
    return s;
}

/* The wire labels of name from offset from to offset to (or to its end),
 * ended by the root label, into out; their length. */
static ULONG
wire_part(const UBYTE *name, LONG from, LONG to, UBYTE *out)
{
    ULONG n = 0;

    while (name[from] != 0 && (to < 0 || from < to) && n + name[from] + 2 <= 256) {
        UBYTE len = name[from];

        memcpy(out + n, name + from, len + 1);
        n += len + 1;
        from += len + 1;
    }
    out[n++] = 0;
    return n;
}

static void
push_event(struct DNSSDServer *sv, struct ServerOperation *op,
    struct dnssd_model_service *m)
{
    struct ServerSession *ss = &sv->sv_Sessions[op->so_Session];
    struct QueuedEvent *qe = NULL;
    size_t required = 0, written;

    m->incarnation = op->so_Daemon->md_DaemonID;
    m->session = make_id(ss->ss_Generation, op->so_Session);
    m->operation = make_id(op->so_Generation, op->so_Slot);
    m->sequence = ss->ss_Sequence + 1;
    if (ss->ss_Queued >= SESSION_EVENTS ||
        dnssd_model_measure(m, &required) != DNSSD_MODEL_OK ||
        (qe = AllocVec(sizeof(*qe) + required, MEMF_ANY)) == NULL ||
        dnssd_model_build(m, qe + 1, required, &written) != DNSSD_MODEL_OK) {
        if (qe != NULL) {
            FreeVec(qe);
        }
        drop_queue(ss);
        ss->ss_Lost = 1;
        notify(ss);
        return;
    }
    ss->ss_Sequence++;
    qe->qe_Size = (ULONG)written;
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

    for (i = 0; i < 4; i++) {
        if (op->so_Watch[i] != NULL) {
            mdnsCacheUnwatch(op->so_Daemon, op->so_Watch[i]);
            op->so_Watch[i] = NULL;
        }
    }
    if (op->so_Service != NULL) {
        mdnsRegRemoveRef(op->so_Daemon, op->so_Service);  /* with its goodbyes */
        op->so_Service = NULL;
    }
    if (op->so_TXT != NULL) {
        FreeVec(op->so_TXT);
        op->so_TXT = NULL;
    }
    if (ss->ss_Operations != 0) {
        ss->ss_Operations--;
    }
    sv->sv_Operations[op->so_Slot] = NULL;
    FreeVec(op);
}

static void
end_with(struct DNSSDServer *sv, struct ServerOperation *op, LONG result)
{
    struct dnssd_model_service m;

    memset(&m, 0, sizeof(m));
    m.kind = DNSSD_EVENT_END;
    m.result = result;
    push_event(sv, op, &m);
    end_operation(sv, op);
}

/* An address record as a struct DNSSDAddress; FALSE if it is not one. */
static BOOL
address_of(const struct MDNSCacheEntry *e, struct DNSSDAddress *a)
{
    memset(a, 0, sizeof(*a));
    if (e->ce_Type == DNS_TYPE_A && e->ce_DataLength == 4) {
        a->da_Family = DNSSD_FAMILY_IPV4;
    } else if (e->ce_Type == DNS_TYPE_AAAA && e->ce_DataLength == 16) {
        a->da_Family = DNSSD_FAMILY_IPV6;
    } else {
        return FALSE;
    }
    a->da_Length = e->ce_DataLength;
    a->da_Interface = e->ce_Link;
    memcpy(a->da_Bytes, e->ce_Data, e->ce_DataLength);
    return TRUE;
}

static LONG
seen_index(const struct ServerOperation *op, const struct DNSSDAddress *a)
{
    LONG i;

    for (i = 0; i < op->so_Found; i++) {
        if (memcmp(&op->so_Seen[i], a, sizeof(*a)) == 0) {
            return i;
        }
    }
    return -1;
}

/* LookupAddress: each address once, as it comes. */
static void
lookup_notify(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    struct DNSSDAddress address;
    struct dnssd_model_service m;

    if (!added || !address_of(e, &address) ||
        (op->so_Interface != 0 && op->so_Interface != e->ce_Link) ||
        seen_index(op, &address) >= 0 || op->so_Found >= LOOKUP_ADDRESSES) {
        return;
    }
    op->so_Seen[op->so_Found++] = address;
    memset(&m, 0, sizeof(m));
    m.kind = DNSSD_EVENT_ADDRESS;
    m.interface_index = e->ce_Link;
    m.ttl = e->ce_TTL;
    m.rrtype = e->ce_Type;
    m.name = span(e->ce_Name, e->ce_NameLength);
    m.addresses = (const struct dnssd_model_address *)&address;
    m.address_count = 1;
    push_event(op->so_Daemon->md_DNSSD, op, &m);
}

/* QueryRecord: each record as it comes and as it goes. */
static void
query_notify(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    struct dnssd_model_service m;

    if (op->so_Interface != 0 && op->so_Interface != e->ce_Link) {
        return;
    }
    memset(&m, 0, sizeof(m));
    m.kind = DNSSD_EVENT_RECORD;
    m.flags = added ? 0 : DNSSD_EVENTF_REMOVED;
    m.interface_index = e->ce_Link;
    m.ttl = added ? e->ce_TTL : 0;
    m.rrtype = e->ce_Type;
    m.name = span(e->ce_Name, e->ce_NameLength);
    m.rdata = span(e->ce_Data, e->ce_DataLength);
    push_event(op->so_Daemon->md_DNSSD, op, &m);
}

/*
 * The parts of an instance's name (wire labels at name, its type at
 * typeOffset and its domain at domainOffset): its own label, and its type
 * and domain as names of their own, in type[] and domain[].
 */
static void
instance_parts(struct dnssd_model_service *m, const UBYTE *name,
    LONG typeOffset, LONG domainOffset, UBYTE *type)
{
    m->instance = span(name + 1, name[0]);
    m->service_type = span(type, wire_part(name, typeOffset, domainOffset, type));
    m->domain = span(name + domainOffset, (ULONG)mdnsWireNameLength(name + domainOffset));
}

static BOOL label_at(const UBYTE *name, ULONG size, UWORD offset);

/*
 * Browse: a PTR whose target is <instance>.<type>.<domain> is an instance
 * on the link it was heard on (RFC 6763 section 4.1); it comes and goes.
 * Of "_services._dns-sd._udp" the targets are the service types on the link
 * (section 9).
 */
static void
browse_notify(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    struct dnssd_model_service m;
    UBYTE type[256];
    const UBYTE *target = e->ce_Data;
    const UBYTE *base = op->so_Name + op->so_TypeOffset;
    LONG baseLength = mdnsWireNameLength(base);

    if (op->so_Interface != 0 && op->so_Interface != e->ce_Link) {
        return;
    }
    memset(&m, 0, sizeof(m));
    m.interface_index = e->ce_Link;
    m.ttl = added ? e->ce_TTL : 0;
    m.rrtype = DNS_TYPE_PTR;
    if (op->so_Enumerate) {
        LONG domainLength = baseLength - (op->so_DomainOffset - op->so_TypeOffset);
        LONG typeEnd = (LONG)e->ce_DataLength - domainLength;

        if (typeEnd < 2 || !label_at(target, e->ce_DataLength, (UWORD)typeEnd) ||
            !mdnsWireNameEqual(target + typeEnd, op->so_Name + op->so_DomainOffset)) {
            return;
        }
        m.kind = DNSSD_EVENT_TYPE;
        m.flags = added ? 0 : DNSSD_EVENTF_REMOVED;
        m.service_type = span(type, wire_part(target, 0, typeEnd, type));
        m.domain = span(target + typeEnd, (ULONG)domainLength);
        push_event(op->so_Daemon->md_DNSSD, op, &m);
        return;
    }
    if (e->ce_DataLength < 2 || target[0] == 0 ||
        baseLength < 0 || 1 + target[0] + baseLength != e->ce_DataLength ||
        !mdnsWireNameEqual(target + 1 + target[0], base)) {
        return;
    }
    m.kind = added ? DNSSD_EVENT_ADD : DNSSD_EVENT_REMOVE;
    instance_parts(&m, target, 1 + target[0],
        1 + target[0] + op->so_DomainOffset - op->so_TypeOffset, type);
    push_event(op->so_Daemon->md_DNSSD, op, &m);
}

/* A signature of what a resolve knows: it is said again only when that
 * changes. */
static ULONG
resolve_signature(const struct ServerOperation *op)
{
    ULONG sum = 1 + op->so_Port + ((ULONG)op->so_Priority << 8) +
        ((ULONG)op->so_Weight << 16) + op->so_Found * 7919UL;
    LONG i, n = mdnsWireNameLength(op->so_Target);

    for (i = 0; i < n; i++) {
        sum = sum * 31 + op->so_Target[i];
    }
    for (i = 0; op->so_HaveTXT && i < op->so_TXTLength; i++) {
        sum = sum * 31 + op->so_TXT[i];
    }
    for (i = 0; i < op->so_Found * (LONG)sizeof(op->so_Seen[0]); i++) {
        sum += ((const UBYTE *)op->so_Seen)[i] * (ULONG)(i + 1);
    }
    return sum == 0 ? 1 : sum;
}

/*
 * Resolve: what is known, once the service has its SRV, its TXT (every
 * DNS-SD service has one, RFC 6763 section 6) and an address - or, when
 * the time is up (final), without the TXT. Said again when it changes.
 */
static void
resolve_report_when(struct ServerOperation *op, BOOL final)
{
    struct dnssd_model_service m;
    UBYTE type[256];
    ULONG signature;

    if (!op->so_HaveSRV || op->so_Found == 0 ||
        (!op->so_HaveTXT && !final)) {
        return;
    }
    signature = resolve_signature(op);
    if (signature == op->so_Reported) {
        return;
    }
    op->so_Reported = signature;
    memset(&m, 0, sizeof(m));
    m.kind = DNSSD_EVENT_RESOLVED;
    m.interface_index = op->so_Interface;
    m.ttl = op->so_TTL;
    m.rrtype = DNS_TYPE_SRV;
    m.port = op->so_Port;
    m.priority = op->so_Priority;
    m.weight = op->so_Weight;
    instance_parts(&m, op->so_Name, op->so_TypeOffset, op->so_DomainOffset, type);
    m.target = span(op->so_Target, (ULONG)mdnsWireNameLength(op->so_Target));
    if (op->so_HaveTXT) {
        m.txt = span(op->so_TXT, op->so_TXTLength);
    }
    m.addresses = (const struct dnssd_model_address *)op->so_Seen;
    m.address_count = op->so_Found;
    push_event(op->so_Daemon->md_DNSSD, op, &m);
}

static void
resolve_report(struct ServerOperation *op)
{
    resolve_report_when(op, FALSE);
}

static void
resolve_address(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    struct DNSSDAddress address;
    LONG i;

    if (!address_of(e, &address) ||
        (op->so_Interface != 0 && op->so_Interface != e->ce_Link)) {
        return;
    }
    i = seen_index(op, &address);
    if (added && i < 0 && op->so_Found < LOOKUP_ADDRESSES) {
        op->so_Seen[op->so_Found++] = address;
    } else if (!added && i >= 0) {
        op->so_Seen[i] = op->so_Seen[--op->so_Found];
    } else {
        return;
    }
    resolve_report(op);
}

static void
resolve_srv(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    struct MDNSDaemon *md = op->so_Daemon;
    const UBYTE *d = e->ce_Data;

    if ((op->so_Interface != 0 && op->so_Interface != e->ce_Link) ||
        e->ce_DataLength < 7) {
        return;
    }
    if (!added) {
        if (op->so_HaveSRV && mdnsWireNameEqual(op->so_Target, d + 6)) {
            struct dnssd_model_service m;
            UBYTE type[256];

            op->so_HaveSRV = FALSE;
            op->so_Reported = 0;
            memset(&m, 0, sizeof(m));
            m.kind = DNSSD_EVENT_REMOVE;
            m.interface_index = e->ce_Link;
            m.rrtype = DNS_TYPE_SRV;
            instance_parts(&m, op->so_Name, op->so_TypeOffset,
                op->so_DomainOffset, type);
            push_event(md->md_DNSSD, op, &m);
        }
        return;
    }
    op->so_Priority = (UWORD)((d[0] << 8) | d[1]);
    op->so_Weight = (UWORD)((d[2] << 8) | d[3]);
    op->so_Port = (UWORD)((d[4] << 8) | d[5]);
    op->so_TTL = e->ce_TTL;
    if (!op->so_HaveSRV || !mdnsWireNameEqual(op->so_Target, d + 6)) {
        /* another host: its addresses are asked for */
        memcpy(op->so_Target, d + 6, e->ce_DataLength - 6);
        op->so_HaveSRV = TRUE;
        op->so_Found = 0;
        if (op->so_Watch[2] != NULL) {
            mdnsCacheUnwatch(md, op->so_Watch[2]);
        }
        if (op->so_Watch[3] != NULL) {
            mdnsCacheUnwatch(md, op->so_Watch[3]);
        }
        op->so_Watch[2] = mdnsCacheWatch(md, op->so_Target, DNS_TYPE_A,
            op->so_Interface, resolve_address, op);
        op->so_Watch[3] = mdnsCacheWatch(md, op->so_Target, DNS_TYPE_AAAA,
            op->so_Interface, resolve_address, op);
    }
    resolve_report(op);
}

static void
resolve_txt(APTR data, const struct MDNSCacheEntry *e, BOOL added)
{
    struct ServerOperation *op = data;
    UBYTE *copy;

    if (op->so_Interface != 0 && op->so_Interface != e->ce_Link) {
        return;
    }
    if (!added) {
        op->so_HaveTXT = FALSE;
        return;
    }
    if (op->so_HaveTXT && op->so_TXTLength == e->ce_DataLength &&
        memcmp(op->so_TXT, e->ce_Data, e->ce_DataLength) == 0) {
        return;
    }
    if ((copy = AllocVec(e->ce_DataLength ? e->ce_DataLength : 1, MEMF_ANY)) == NULL) {
        return;
    }
    if (op->so_TXT != NULL) {
        FreeVec(op->so_TXT);
    }
    op->so_TXT = copy;
    memcpy(op->so_TXT, e->ce_Data, e->ce_DataLength);
    op->so_TXTLength = e->ce_DataLength;
    op->so_HaveTXT = TRUE;
    resolve_report(op);
}

/* Register: the service is announced, under the name it has now; or the
 * name of the host it stands in for is another host's, and it ends. */
static void
register_notify(APTR data, const struct MDNSRegisteredService *rs, LONG result)
{
    struct ServerOperation *op = data;
    struct dnssd_model_service m;
    UBYTE wire[256], type[256];
    LONG used, at, typeAt, domainAt;

    if (result != MDNS_OK) {
        end_with(op->so_Daemon->md_DNSSD, op, DNSSD_ERR_CONFLICT);
        return;
    }
    /* the name as wire labels: the instance's label as it is */
    at = (LONG)strlen(rs->rs_Instance);
    if (at > 63) {
        return;
    }
    wire[0] = (UBYTE)at;
    memcpy(wire + 1, rs->rs_Instance, at);
    typeAt = 1 + at;
    if (mdnsNameToWire(rs->rs_Type, wire + typeAt, 256 - typeAt, &used) != 0) {
        return;
    }
    domainAt = typeAt + used - 1;
    if (mdnsNameToWire(rs->rs_Domain, wire + domainAt, 256 - domainAt,
            &used) != 0) {
        return;
    }
    memset(&m, 0, sizeof(m));
    m.kind = DNSSD_EVENT_REGISTERED;
    m.port = rs->rs_Port;
    m.rrtype = DNS_TYPE_SRV;
    instance_parts(&m, wire, typeAt, domainAt, type);
    push_event(op->so_Daemon->md_DNSSD, op, &m);
}

/*
 * Wire labels from at to the end (or to stop, an offset) as text, for the
 * responder's registration, which still keeps text: dots between labels,
 * a dot or backslash in a label escaped (RFC 1035 section 5.1). Its length.
 */
static ULONG
name_text(const UBYTE *wire, LONG at, LONG stop, char *text, ULONG max)
{
    ULONG n = 0;

    while (wire[at] != 0 && (stop < 0 || at < stop)) {
        UBYTE len = wire[at++], i;

        if (n != 0 && n < max) {
            text[n++] = '.';
        }
        for (i = 0; i < len; i++, at++) {
            if ((wire[at] == '.' || wire[at] == '\\') && n + 1 < max) {
                text[n++] = '\\';
            }
            if (n < max) {
                text[n++] = (char)wire[at];
            }
        }
    }
    return n;
}

/* Register: the message's name, TXT and subtypes, checked; the service. */
static LONG
start_register(struct MDNSDaemon *md, struct ServerOperation *op,
    const struct DNSSDIPCMsg *msg)
{
    char instance[64], type[MDNS_MAX_TYPE + 1], domain[MDNS_MAX_DOMAIN + 1];
    char subtypes[MDNS_MAX_SUBTYPES][MDNS_MAX_SUBTYPE + 1];
    const UBYTE *name = msg->dm_Data;
    UWORD count = 0;
    ULONG at;
    LONG rc;

    if (name[0] == 0 || name[0] > 63 || msg->dm_TypeOffset != 1 + name[0] ||
        msg->dm_Port == 0 || msg->dm_TXTLength > msg->dm_ExtraSize ||
        msg->dm_TXTLength > MDNS_TXT_DATA_MAX ||
        (msg->dm_ExtraSize != 0 && msg->dm_Extra == NULL)) {
        return DNSSD_ERR_BADPARAM;
    }
    memcpy(instance, name + 1, name[0]);
    instance[name[0]] = '\0';
    if (strlen(instance) != name[0] ||
        name_text(name, msg->dm_TypeOffset, msg->dm_DomainOffset, type,
            sizeof(type) - 1) >= sizeof(type) - 1 ||
        name_text(name, msg->dm_DomainOffset, -1, domain,
            sizeof(domain) - 1) >= sizeof(domain) - 1) {
        return DNSSD_ERR_BADNAME;
    }
    type[name_text(name, msg->dm_TypeOffset, msg->dm_DomainOffset, type,
        sizeof(type) - 1)] = '\0';
    domain[name_text(name, msg->dm_DomainOffset, -1, domain,
        sizeof(domain) - 1)] = '\0';
    for (at = msg->dm_TXTLength; at < msg->dm_ExtraSize &&
            msg->dm_Extra[at] != 0; at += 1 + msg->dm_Extra[at]) {
        UBYTE len = msg->dm_Extra[at];

        if (count == MDNS_MAX_SUBTYPES || len > MDNS_MAX_SUBTYPE ||
            at + 1 + len > msg->dm_ExtraSize) {
            return DNSSD_ERR_BADPARAM;
        }
        memcpy(subtypes[count], msg->dm_Extra + at + 1, len);
        subtypes[count++][len] = '\0';
    }
    if (msg->dm_Host[0] != '\0' &&
        (msg->dm_HostAddressCount == 0 || msg->dm_HostAddresses == NULL)) {
        return DNSSD_ERR_BADPARAM;
    }
    rc = mdnsRegAddNative(md, instance, type, domain, subtypes, count,
        msg->dm_Port, msg->dm_Extra, msg->dm_TXTLength,
        msg->dm_Host[0] != '\0' ? (const char *)msg->dm_Host : NULL,
        msg->dm_HostAddresses, msg->dm_HostAddressCount,
        register_notify, op, &op->so_Service);
    return rc == MDNS_OK ? DNSSD_OK : rc == MDNSERR_DUPLICATE ? DNSSD_ERR_EXISTS :
        rc == MDNSERR_NOMEM ? DNSSD_ERR_NOMEM : DNSSD_ERR_BADPARAM;
}

/* Is the name in a message wire labels that end where it says? */
static BOOL
valid_name(const struct DNSSDIPCMsg *msg)
{
    LONG length;

    if (msg->dm_Data == NULL || msg->dm_DataSize < 2 ||
        msg->dm_DataSize > 255) {
        return FALSE;
    }
    length = mdnsWireNameLength(msg->dm_Data);
    return length == (LONG)msg->dm_DataSize;
}

static BOOL
label_at(const UBYTE *name, ULONG size, UWORD offset)
{
    ULONG at = 0;

    while (at < size && name[at] != 0) {
        if (at == offset) {
            return TRUE;
        }
        at += name[at] + 1;
    }
    return FALSE;
}

static LONG
start_operation(struct MDNSDaemon *md, struct ServerSession *ss, LONG sslot,
    struct DNSSDIPCMsg *msg)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    struct ServerOperation *op;
    ULONG timeout = msg->dm_Timeout;
    UBYTE kind;
    LONG i;

    switch (msg->dm_Command) {
    case DNSSD_IPC_LOOKUP:
        kind = OP_LOOKUP;
        if (timeout == 0) {
            timeout = 3;
        }
        if (msg->dm_Family != DNSSD_FAMILY_ANY &&
            msg->dm_Family != DNSSD_FAMILY_IPV4 &&
            msg->dm_Family != DNSSD_FAMILY_IPV6) {
            return DNSSD_ERR_BADPARAM;
        }
        break;
    case DNSSD_IPC_QUERY:
        kind = OP_QUERY;
        break;
    case DNSSD_IPC_BROWSE:
        kind = OP_BROWSE;
        timeout = 0;
        break;
    case DNSSD_IPC_REGISTER:
        kind = OP_REGISTER;
        timeout = 0;
        break;
    default:
        kind = OP_RESOLVE;
        if (timeout == 0) {
            timeout = 5;
        }
        break;
    }
    if (!valid_name(msg) || timeout > 60) {
        return DNSSD_ERR_BADPARAM;
    }
    if ((kind == OP_BROWSE || kind == OP_RESOLVE || kind == OP_REGISTER) &&
        (!label_at(msg->dm_Data, msg->dm_DataSize, msg->dm_TypeOffset) ||
         !label_at(msg->dm_Data, msg->dm_DataSize, msg->dm_DomainOffset) ||
         msg->dm_DomainOffset <= msg->dm_TypeOffset)) {
        return DNSSD_ERR_BADPARAM;
    }
    /* multicast DNS answers in "local" (RFC 6762 section 3): a service
     * in another domain is wide-area DNS-SD's, which this is not */
    if (kind == OP_BROWSE || kind == OP_RESOLVE || kind == OP_REGISTER) {
        static const UBYTE local[] = { 5, 'l', 'o', 'c', 'a', 'l', 0 };

        if (!mdnsWireNameEqual(msg->dm_Data + msg->dm_DomainOffset, local)) {
            return DNSSD_ERR_BADPARAM;
        }
    }
    if (ss->ss_Operations >= SESSION_OPERATIONS) {
        return DNSSD_ERR_BUSY;
    }
    for (i = 0; i < SERVER_OPERATIONS && sv->sv_Operations[i] != NULL; i++) {
    }
    if (i == SERVER_OPERATIONS) {
        return DNSSD_ERR_BUSY;
    }
    op = AllocVec(sizeof(*op), MEMF_ANY | MEMF_CLEAR);
    if (op == NULL) {
        return DNSSD_ERR_NOMEM;
    }
    op->so_Daemon = md;
    op->so_Generation = next_generation(sv);
    op->so_Slot = (UWORD)i;
    op->so_Kind = kind;
    op->so_Session = (UWORD)sslot;
    op->so_Interface = msg->dm_Interface;
    op->so_RRType = msg->dm_RRType;
    op->so_TypeOffset = msg->dm_TypeOffset;
    op->so_DomainOffset = msg->dm_DomainOffset;
    op->so_Deadline = timeout != 0 ? (mdnsTicks() + timeout * 50) | 1 : 0;
    memcpy(op->so_Name, msg->dm_Data, msg->dm_DataSize);
    sv->sv_Operations[i] = op;
    ss->ss_Operations++;
    msg->dm_Operation = make_id(op->so_Generation, i);

    /* a watch tells at once what is known: the events may come now */
    switch (kind) {
    case OP_LOOKUP:
        if (msg->dm_Family != DNSSD_FAMILY_IPV6) {
            op->so_Watch[0] = mdnsCacheWatch(md, op->so_Name, DNS_TYPE_A,
                op->so_Interface, lookup_notify, op);
        }
        if (msg->dm_Family != DNSSD_FAMILY_IPV4) {
            op->so_Watch[1] = mdnsCacheWatch(md, op->so_Name, DNS_TYPE_AAAA,
                op->so_Interface, lookup_notify, op);
        }
        break;
    case OP_QUERY:
        op->so_Watch[0] = mdnsCacheWatch(md, op->so_Name, op->so_RRType,
            op->so_Interface, query_notify, op);
        break;
    case OP_REGISTER: {
        LONG rc = start_register(md, op, msg);

        if (rc != DNSSD_OK) {
            end_operation(sv, op);
            msg->dm_Operation = 0;
            return rc;
        }
        return DNSSD_OK;
    }
    case OP_BROWSE: {
        static const UBYTE services[] = {
            9, '_', 's', 'e', 'r', 'v', 'i', 'c', 'e', 's',
            7, '_', 'd', 'n', 's', '-', 's', 'd', 4, '_', 'u', 'd', 'p', 0
        };

        op->so_Enumerate = op->so_TypeOffset == 0 &&
            op->so_DomainOffset == sizeof(services) - 1 &&
            memcmp(op->so_Name, services, sizeof(services) - 1) == 0;
    }
        /* fall through: a subtype's PTRs name instances of the base type: the base is
         * what follows "<subtype>._sub" (RFC 6763 section 7.1) */
        op->so_Watch[0] = mdnsCacheWatch(md, op->so_Name, DNS_TYPE_PTR,
            op->so_Interface, browse_notify, op);
        break;
    default:
        op->so_Watch[0] = mdnsCacheWatch(md, op->so_Name, DNS_TYPE_SRV,
            op->so_Interface, resolve_srv, op);
        op->so_Watch[1] = mdnsCacheWatch(md, op->so_Name, DNS_TYPE_TXT,
            op->so_Interface, resolve_txt, op);
        break;
    }
    if (op->so_Watch[0] == NULL && op->so_Watch[1] == NULL) {
        end_operation(sv, op);
        msg->dm_Operation = 0;
        return DNSSD_ERR_NOMEM;
    }
    return DNSSD_OK;
}

/* Operations whose time is up end: a lookup or resolve that found
 * something with DNSSD_OK, else DNSSD_ERR_TIMEOUT; a query with
 * DNSSD_ERR_TIMEOUT. */
void
dnssdServerTick(struct MDNSDaemon *md)
{
    struct DNSSDServer *sv = md->md_DNSSD;
    ULONG now = mdnsTicks();
    LONG i;

    if (sv == NULL) {
        return;
    }
    for (i = 0; i < SERVER_OPERATIONS; i++) {
        struct ServerOperation *op = sv->sv_Operations[i];

        if (op != NULL && op->so_Deadline != 0 &&
            (LONG)(now - op->so_Deadline) >= 0) {
            BOOL found;

            if (op->so_Kind == OP_RESOLVE) {
                resolve_report_when(op, TRUE);  /* without a TXT, if none came */
            }
            found = op->so_Kind == OP_LOOKUP ? op->so_Found != 0 :
                op->so_Kind == OP_RESOLVE ? op->so_HaveSRV && op->so_Found != 0 :
                FALSE;

            end_with(sv, op, found ? DNSSD_OK : DNSSD_ERR_TIMEOUT);
        }
    }
}

static void
close_session(struct DNSSDServer *sv, struct ServerSession *ss, LONG slot)
{
    LONG i;

    for (i = 0; i < SERVER_OPERATIONS; i++) {
        struct ServerOperation *op = sv->sv_Operations[i];

        if (op != NULL && op->so_Session == slot) {
            end_operation(sv, op);
        }
    }
    drop_queue(ss);
    ss->ss_Notify = NULL;           /* given back: the client has it */
    ss->ss_Active = 0;
}

/* The READ of a session: the next event, or the notification lent again. */
static LONG
read_event(struct ServerSession *ss, struct DNSSDIPCMsg *msg, ULONG incarnation,
    ULONG session)
{
    struct QueuedEvent *qe;

    if (ss->ss_Lost) {
        struct dnssd_model_service m;
        size_t written;

        memset(&m, 0, sizeof(m));
        m.kind = DNSSD_EVENT_LOST;
        m.flags = DNSSD_MODEL_FLAG_LOST;
        m.incarnation = incarnation;
        m.session = session;
        m.sequence = ++ss->ss_Sequence;
        if (dnssd_model_build(&m, msg->dm_Data, msg->dm_Data == NULL ? 0 :
                msg->dm_DataMax, &written) != DNSSD_MODEL_OK) {
            ss->ss_Sequence--;
            msg->dm_DataSize = (ULONG)written;
            return DNSSD_ERR_TOOSMALL;
        }
        msg->dm_DataSize = (ULONG)written;
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
                ss->ss_Sequence = 0;
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
            msg->dm_StatusOperations += sv->sv_Operations[i] != NULL;
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
        msg->dm_Result = read_event(ss, msg, md->md_DaemonID, msg->dm_Session);
        break;
    case DNSSD_IPC_UPDATE: {
        ULONG oslot = (msg->dm_Operation & 0xffff) - 1;
        struct ServerOperation *op = oslot < SERVER_OPERATIONS ?
            sv->sv_Operations[oslot] : NULL;

        msg->dm_Result = DNSSD_ERR_STALE;
        if (op != NULL && op->so_Session == slot && op->so_Kind == OP_REGISTER &&
            op->so_Generation == (msg->dm_Operation >> 16)) {
            BOOL txt = (msg->dm_Flags & DNSSD_IPCF_TXT) != 0;
            BOOL addrs = (msg->dm_Flags & DNSSD_IPCF_ADDRESSES) != 0;

            msg->dm_Result = DNSSD_ERR_BADPARAM;
            if ((!txt || msg->dm_TXTLength <= msg->dm_ExtraSize) &&
                (!addrs || msg->dm_HostAddresses != NULL) &&
                mdnsRegUpdateRef(md, op->so_Service, txt ? msg->dm_Extra : NULL,
                    txt ? msg->dm_TXTLength : 0, msg->dm_Port,
                    msg->dm_HostAddresses,
                    addrs ? (LONG)msg->dm_HostAddressCount : -1) == MDNS_OK) {
                msg->dm_Result = DNSSD_OK;
            }
        }
        break;
    }
    case DNSSD_IPC_LOOKUP:
    case DNSSD_IPC_REGISTER:
    case DNSSD_IPC_BROWSE:
    case DNSSD_IPC_RESOLVE:
    case DNSSD_IPC_QUERY:
        msg->dm_Result = start_operation(md, ss, slot, msg);
        break;
    case DNSSD_IPC_CANCEL: {
        ULONG oslot = (msg->dm_Operation & 0xffff) - 1;
        struct ServerOperation *op;

        msg->dm_Result = DNSSD_OK;          /* idempotent: gone is cancelled */
        if (oslot >= SERVER_OPERATIONS) {
            msg->dm_Result = DNSSD_ERR_BADPARAM;
            break;
        }
        op = sv->sv_Operations[oslot];
        if (op != NULL && op->so_Session == slot &&
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
