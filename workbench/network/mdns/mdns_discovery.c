/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * Bounded, daemon-owned DNS-SD cache and per-operation event queues. DNS
 * names remain counted labels: dots/backslashes in an instance never become
 * separators. All entry points run in mdnsd's task and none calls client
 * code. The one piece of client memory kept is a lent MDNSEventRequest,
 * which is only given back: er_Result is set and the message is replied,
 * once. No client event buffer, Hook or task pointer is retained.
 */
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>
#include <netinet/in.h>
#include "mdnsd.h"

#define DISC_RECORDS 128
#define DISC_VIEWS 32
#define DISC_SESSIONS 16
#define DISC_QUEUE 8
#define DISC_TTL_MAX 4500
#define DISC_PENDING 8      /* A records held for an SRV yet to come */
#define DISC_PENDING_TICKS 50

struct DiscoveryRecord
{
    UBYTE key[256];
    UBYTE data[512];
    UWORD type, length;
    ULONG link, expires, received, refresh, ttl, serial;
    ULONG nextMissing, missingInterval;
    UWORD refreshStep;
};
struct PendingAddress
{
    UBYTE key[256];
    UBYTE data[4];
    UWORD rclass;
    ULONG link, ttl, received;
};
struct DiscoveryView
{
    UBYTE key[256];
    ULONG link;
    BOOL active, present;
    struct MDNSDiscoveryEvent event;
};
struct DiscoverySession
{
    APTR handle;
    ULONG owner;
    UBYTE key[256];
    BOOL resolve, failed, resolved;
    ULONG nextQuery, interval, deadline;
    UWORD head, count;
    struct MDNSDiscoveryEvent queue[DISC_QUEUE];
    struct MDNSEventRequest *request;   /* lent by the owner, or NULL */
};
struct MDNSDiscovery
{
    struct DiscoveryRecord records[DISC_RECORDS];
    struct DiscoveryView views[DISC_VIEWS];
    struct DiscoverySession sessions[DISC_SESSIONS];
    IPTR nextHandle;
    ULONG lastRefresh, serial;
    /* The views are rebuilt only when the cache or the operations changed,
     * or when the next record expires. */
    BOOL changed, expiring;
    ULONG nextExpiry;
    /* An A that came before the SRV naming its host: kept a second, so
     * that the order records arrive in does not matter. */
    struct PendingAddress pending[DISC_PENDING];
};

static BOOL due(ULONG now, ULONG deadline)
{
    return (LONG)(now - deadline) >= 0;
}
static UBYTE lower(UBYTE c)
{
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}
static LONG key_length(const UBYTE *key)
{
    LONG at = 0;

    while (at < 255 && key[at] != 0) {
        at += key[at] + 1;
    }
    return at < 256 ? at + 1 : 0;
}
static BOOL same_key(const UBYTE *a, const UBYTE *b)
{
    LONG length = key_length(a), i;

    if (length == 0 || length != key_length(b)) {
        return FALSE;
    }
    for (i = 0; i < length; i++) {
        if (lower(a[i]) != lower(b[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Decode compression with bounded traversal, preserving the labels/case.
 * Return bytes consumed at the original offset, independently of the target. */
static LONG decode(const UBYTE *pkt, LONG length, LONG offset, UBYTE *key)
{
    LONG at = 0, consumed = 0, jumps = 0;
    BOOL jumped = FALSE;

    while (offset >= 0 && offset < length && jumps++ < 128) {
        UBYTE n = pkt[offset++];

        if (!jumped) {
            consumed++;
        }
        if ((n & 0xc0) == 0xc0) {
            if (offset >= length) {
                return -1;
            }
            if (!jumped) {
                consumed++;
            }
            offset = ((n & 0x3f) << 8) | pkt[offset];
            jumped = TRUE;
        } else {
            if (n > 63 || at + n + 1 > 255 || offset + n > length) {
                return -1;
            }
            key[at++] = n;
            if (n == 0) {
                return consumed;
            }
            memcpy(key + at, pkt + offset, n);
            at += n;
            offset += n;
            if (!jumped) {
                consumed += n;
            }
        }
    }
    return -1;
}

static BOOL type_key(const char *type, UBYTE *key)
{
    LONG length = mdnsTextLength(type, MDNS_MAX_TYPE + 1), split = 0, i;

    while (split < length && type[split] != '.') {
        split++;
    }
    if (length > MDNS_MAX_TYPE || split < 2 || split > 16 ||
        type[0] != '_' || (Stricmp(type + split, "._udp") != 0 &&
        Stricmp(type + split, "._tcp") != 0)) {
        return FALSE;
    }
    for (i = 1; i < split; i++) {
        UBYTE c = lower((UBYTE)type[i]);

        if (!(c >= 'a' && c <= 'z') && !(c >= '0' && c <= '9') && c != '-') {
            return FALSE;
        }
    }
    key[0] = split;
    memcpy(key + 1, type, split);
    key[split + 1] = 4;
    memcpy(key + split + 2, type + split + 1, 4);
    key[split + 6] = 5;
    memcpy(key + split + 7, "local", 6);
    return TRUE;
}

/*
 * Gives a lent request back, exactly once. A request whose owner has gone
 * is dropped unread instead: that owner broke the contract, and its port
 * may be gone too. This check narrows, but cannot close, the window in
 * which an owner ends between it and ReplyMsg(); owners must stop first.
 */
static void wake(struct DiscoverySession *s, LONG result)
{
    struct MDNSEventRequest *request = s->request;

    if (request == NULL) {
        return;
    }
    s->request = NULL;
    if (FindTaskByPID(s->owner) == NULL) {
        return;
    }
    request->er_Result = result;
    ReplyMsg(&request->er_Msg);
}

/* An operation that ran out of room: the caller must stop and begin it
 * again. Nothing else is failed with it. */
static void fail(struct MDNSDiscovery *d, struct DiscoverySession *s)
{
    if (s->handle == NULL || s->failed) {
        return;
    }
    s->failed = TRUE;
    s->count = 0;
    d->changed = TRUE;
    wake(s, MDNSERR_BUSY);
}

/* Whether an operation still wants records: a browse until it is stopped
 * or failed, a resolve until it has its answer. */
static BOOL live(const struct DiscoverySession *s)
{
    return s->handle != NULL && !s->failed && !(s->resolve && s->resolved);
}

static void enqueue(struct DiscoverySession *s, const struct MDNSDiscoveryEvent *event)
{
    UWORD i;

    if (s->failed) {
        return;
    }
    /* A pending snapshot for the same instance/link is replaced atomically.
     * Overflow is explicit: the caller must stop and resubscribe. */
    for (i = 0; i < s->count; i++) {
        struct MDNSDiscoveryEvent *old = &s->queue[(s->head + i) % DISC_QUEUE];

        if (old->de_Interface == event->de_Interface &&
            Stricmp(old->de_Name, event->de_Name) == 0 &&
            Stricmp(old->de_Type, event->de_Type) == 0) {
            *old = *event;
            wake(s, MDNS_OK);
            return;
        }
    }
    if (s->count == DISC_QUEUE) {
        s->count = 0;
        s->failed = TRUE;
        wake(s, MDNSERR_BUSY);
        return;
    }
    s->queue[(s->head + s->count++) % DISC_QUEUE] = *event;
    wake(s, MDNS_OK);
}

static void notify(struct MDNSDiscovery *d, const struct DiscoveryView *view, ULONG code)
{
    LONG i;
    struct MDNSDiscoveryEvent event = view->event;

    event.de_Code = code;
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (s->handle != NULL && !s->resolve &&
            same_key(s->key, view->key + view->key[0] + 1)) {
            enqueue(s, &event);
        }
    }
}

static struct DiscoveryRecord *find_record(struct MDNSDiscovery *d,
    const UBYTE *key, ULONG link, UWORD type, ULONG now)
{
    LONG i;
    struct DiscoveryRecord *found = NULL;

    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];

        if (r->type == type && r->link == link && !due(now, r->expires) &&
            same_key(key, r->key) &&
            (found == NULL || (LONG)(r->serial - found->serial) > 0)) {
            found = r;
        }
    }
    return found;
}

/*
 * What an active operation can reach, and so what is cached, refreshed and
 * asked for: the PTRs of a browsed type; the SRV and TXT of an instance of
 * a browsed type, or of an instance being resolved; the A of a host that
 * such an SRV names. Everything else on the network is left alone
 * (RFC 6762 section 5.2 asks for queries only for what someone wants).
 */
static BOOL browsed(const struct MDNSDiscovery *d, const UBYTE *type)
{
    LONG i;

    for (i = 0; i < DISC_SESSIONS; i++) {
        const struct DiscoverySession *s = &d->sessions[i];

        if (live(s) && !s->resolve && same_key(s->key, type)) {
            return TRUE;
        }
    }
    return FALSE;
}
static BOOL wanted_instance(const struct MDNSDiscovery *d, const UBYTE *instance)
{
    LONG i;

    if (browsed(d, instance + instance[0] + 1)) {
        return TRUE;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        const struct DiscoverySession *s = &d->sessions[i];

        if (live(s) && s->resolve && same_key(s->key, instance)) {
            return TRUE;
        }
    }
    return FALSE;
}
static BOOL wanted_host(const struct MDNSDiscovery *d, const UBYTE *host,
    ULONG link, ULONG now)
{
    LONG i;

    for (i = 0; i < DISC_RECORDS; i++) {
        const struct DiscoveryRecord *r = &d->records[i];

        if (r->type == DNS_TYPE_SRV && r->link == link && !due(now, r->expires) &&
            same_key(r->data + 6, host) && wanted_instance(d, r->key)) {
            return TRUE;
        }
    }
    return FALSE;
}
static BOOL wanted(const struct MDNSDiscovery *d, UWORD type, const UBYTE *key,
    ULONG link, ULONG now)
{
    switch (type) {
    case DNS_TYPE_PTR:
        return browsed(d, key);
    case DNS_TYPE_SRV:
    case DNS_TYPE_TXT:
        return wanted_instance(d, key);
    case DNS_TYPE_A:
        return wanted_host(d, key, link, now);
    }
    return FALSE;
}

/* The cache has no room for a record operations want: those operations,
 * and only those, are failed. */
static void fail_wanting(struct MDNSDiscovery *d, UWORD type, const UBYTE *key,
    ULONG link, ULONG now)
{
    LONG i;

    if (type == DNS_TYPE_A) {
        for (i = 0; i < DISC_RECORDS; i++) {
            const struct DiscoveryRecord *r = &d->records[i];

            if (r->type == DNS_TYPE_SRV && r->link == link &&
                !due(now, r->expires) && same_key(r->data + 6, key)) {
                fail_wanting(d, DNS_TYPE_SRV, r->key, link, now);
            }
        }
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (!live(s)) {
            continue;
        }
        if (s->resolve ? type != DNS_TYPE_PTR && same_key(s->key, key) :
            same_key(s->key, type == DNS_TYPE_PTR ? key : key + key[0] + 1)) {
            fail(d, s);
        }
    }
}

static BOOL snapshot(struct MDNSDiscovery *d, const UBYTE *key, ULONG link,
    struct MDNSDiscoveryEvent *out, ULONG now)
{
    const struct DiscoveryRecord *srv = find_record(d, key, link, DNS_TYPE_SRV, now);
    const struct DiscoveryRecord *txt = find_record(d, key, link, DNS_TYPE_TXT, now);
    const struct DiscoveryRecord *addr;
    const UBYTE *type = key + key[0] + 1;
    LONG n;

    if (srv == NULL || txt == NULL || key[0] == 0) {
        return FALSE;
    }
    addr = find_record(d, srv->data + 6, link, DNS_TYPE_A, now);
    if (addr == NULL || (srv->data[4] == 0 && srv->data[5] == 0)) {
        return FALSE;
    }
    memset(out, 0, sizeof(*out));
    memcpy(out->de_Name, key + 1, key[0]);
    n = type[0];
    if (n + 5 > MDNS_MAX_TYPE || type[n + 1] != 4) {
        return FALSE;
    }
    memcpy(out->de_Type, type + 1, n);
    out->de_Type[n] = '.';
    memcpy(out->de_Type + n + 1, type + n + 2, 4);
    out->de_Port = ((UWORD)srv->data[4] << 8) | srv->data[5];
    memcpy(&out->de_Address, addr->data, 4);
    out->de_Interface = htonl(link);
    out->de_TXTLength = txt->length;
    memcpy(out->de_TXT, txt->data, txt->length);
    return TRUE;
}

static BOOL valid_instance(const UBYTE *key)
{
    UBYTE expected[256];
    char type[MDNS_MAX_TYPE + 1];
    const UBYTE *suffix;
    LONG n;

    if (key[0] == 0 || key[0] > 63 || memchr(key + 1, 0, key[0]) != NULL) {
        return FALSE;
    }
    suffix = key + key[0] + 1;
    n = suffix[0];
    if (n < 2 || n + 5 > MDNS_MAX_TYPE || suffix[n + 1] != 4) {
        return FALSE;
    }
    memcpy(type, suffix + 1, n);
    type[n] = '.';
    memcpy(type + n + 1, suffix + n + 2, 4);
    type[n + 5] = '\0';
    return type_key(type, expected) && same_key(expected, suffix);
}

/* Whether any browse or resolve wants records at all. */
static BOOL discovering(const struct MDNSDiscovery *d)
{
    LONG i;

    for (i = 0; i < DISC_SESSIONS; i++) {
        if (live(&d->sessions[i])) {
            return TRUE;
        }
    }
    return FALSE;
}
static void hold_address(struct MDNSDiscovery *d, UWORD rclass, ULONG ttl,
    const UBYTE *key, const UBYTE *data, ULONG link, ULONG now)
{
    struct PendingAddress *slot = &d->pending[0];
    LONG i;

    for (i = 0; i < DISC_PENDING; i++) {
        struct PendingAddress *a = &d->pending[i];

        if (a->link == link && same_key(a->key, key)) {
            slot = a;
            break;
        }
        if (a->link == 0 || due(now, a->received + DISC_PENDING_TICKS) ||
            (LONG)(a->received - slot->received) < 0) {
            slot = a;
        }
    }
    memcpy(slot->key, key, 256);
    memcpy(slot->data, data, 4);
    slot->rclass = rclass;
    slot->ttl = ttl;
    slot->link = link;
    slot->received = now;
}

static void cache(struct MDNSDiscovery *d, UWORD type, UWORD rclass, ULONG ttl,
    const UBYTE *key, const UBYTE *data, LONG size, ULONG link, ULONG now)
{
    LONG i;
    struct DiscoveryRecord *slot = NULL;
    ULONG missingAt = 0, missingInterval = 50;

    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        BOOL match = r->type == type && r->link == link && same_key(r->key, key);
        BOOL equalData = r->length == size && memcmp(r->data, data, size) == 0;
        BOOL identical;

        if (type == DNS_TYPE_PTR) {
            equalData = same_key(r->data, data);
        } else if (type == DNS_TYPE_SRV) {
            equalData = memcmp(r->data, data, 6) == 0 && same_key(r->data + 6, data + 6);
        }
        identical = match && equalData;

        if (identical) {
            slot = r;
        } else if (match && (rclass & 0x8000) != 0 &&
            type != DNS_TYPE_PTR && ttl != 0 &&
            due(now, r->received + 50) && !due(now + 50, r->expires)) {
            r->expires = now + 50;   /* cache flush: grace, not instant removal */
            d->changed = TRUE;
        }
    }
    if (ttl == 0) {
        if (slot != NULL && !due(now + 50, slot->expires)) {
            slot->expires = now + 50; /* another responder may rescue it */
            d->changed = TRUE;
        }
        return;
    }
    if (slot != NULL) {
        missingAt = slot->nextMissing;
        missingInterval = slot->missingInterval;
    }
    /* A record already held is renewed; a new one only when an operation
     * can reach it. */
    if (slot == NULL && !wanted(d, type, key, link, now)) {
        if (type == DNS_TYPE_A && discovering(d)) {
            hold_address(d, rclass, ttl, key, data, link, now);
        }
        return;
    }
    if (slot == NULL) {
        for (i = 0; i < DISC_RECORDS; i++) {
            if (d->records[i].type == 0 || due(now, d->records[i].expires)) {
                slot = &d->records[i];
                break;
            }
        }
    }
    if (slot == NULL) {
        /* Full: the oldest record no operation reaches any more gives way. */
        for (i = 0; i < DISC_RECORDS; i++) {
            struct DiscoveryRecord *r = &d->records[i];

            if ((slot == NULL || (LONG)(r->serial - slot->serial) < 0) &&
                !wanted(d, r->type, r->key, r->link, now)) {
                slot = r;
            }
        }
    }
    if (slot == NULL) {
        fail_wanting(d, type, key, link, now);
        return;
    }
    d->changed = TRUE;
    memset(slot, 0, sizeof(*slot));
    memcpy(slot->key, key, 256);
    memcpy(slot->data, data, size);
    slot->length = size;
    slot->link = link;
    slot->type = type;
    slot->ttl = ttl > DISC_TTL_MAX ? DISC_TTL_MAX : ttl;
    slot->received = now;
    slot->serial = ++d->serial;
    slot->nextMissing = missingAt;
    slot->missingInterval = missingInterval;
    slot->expires = now + slot->ttl * 50;
    slot->refresh = now + slot->ttl * (40 + now % 2); /* 80–82 percent */
    if (type == DNS_TYPE_SRV) {
        /* the address of its host may have come first */
        for (i = 0; i < DISC_PENDING; i++) {
            struct PendingAddress *a = &d->pending[i];

            if (a->link == link && !due(now, a->received + DISC_PENDING_TICKS) &&
                same_key(a->key, data + 6)) {
                struct PendingAddress held = *a;

                memset(a, 0, sizeof(*a));
                cache(d, DNS_TYPE_A, held.rclass, held.ttl, held.key, held.data,
                      4, held.link, now);
            }
        }
    }
}

void mdnsDiscoveryRecord(struct MDNSDaemon *md, const struct MDNSRecord *record,
    const UBYTE *pkt, LONG length, LONG ownerOffset)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    UBYTE key[256] = {0}, data[512] = {0};
    LONG size = record->rdlength, consumed;
    ULONG now = mdnsTicks();
    ULONG link = md->md_SendIf >= 0 ? md->md_Ifs[md->md_SendIf].mi_Addr : 0;

    if (d == NULL || (record->rclass & 0x7fff) != DNS_CLASS_IN ||
        link == 0 || decode(pkt, length, ownerOffset, key) < 0 ||
        record->rdoffset < 0 || record->rdoffset + size > length) {
        return;
    }
    if (record->type == DNS_TYPE_PTR) {
        consumed = decode(pkt, length, record->rdoffset, data);
        if (consumed != size || !valid_instance(data) ||
            !same_key(key, data + data[0] + 1)) {
            return;
        }
        size = key_length(data);
    } else if (record->type == DNS_TYPE_SRV) {
        if (!valid_instance(key) || size < 7) {
            return;
        }
        memcpy(data, record->rdata, 6);
        consumed = decode(pkt, length, record->rdoffset + 6, data + 6);
        if (consumed != size - 6) {
            return;
        }
        size = 6 + key_length(data + 6);
    } else if (record->type == DNS_TYPE_TXT) {
        LONG at = 0;

        if (!valid_instance(key) || size == 0 || size > MDNS_DISCOVERY_TXT_MAX) {
            return;
        }
        while (at < size) {
            at += record->rdata[at] + 1;
        }
        if (at != size) {
            return;
        }
        memcpy(data, record->rdata, size);
    } else if (record->type == DNS_TYPE_A) {
        if (size != 4) {
            return;
        }
        memcpy(data, record->rdata, 4);
    } else {
        return;
    }
    cache(d, record->type, record->rclass, record->ttl, key, data, size, link, now);
}



static void send_question(struct MDNSDaemon *md, const UBYTE *key, UWORD type, ULONG link)
{
    UBYTE pkt[1400];
    LONG n = key_length(key), i, pos;
    UWORD answers = 0;
    ULONG now = mdnsTicks();
    struct MDNSDiscovery *d = md->md_Discovery;

    if (link == 0) {
        for (i = 0; i < md->md_IfCount; i++) {
            send_question(md, key, type, md->md_Ifs[i].mi_Addr);
        }
        return;
    }
    memset(pkt, 0, sizeof(struct MDNSHeader));
    pkt[5] = 1;
    memcpy(pkt + 12, key, n);
    pos = 12 + n;
    pkt[pos++] = type >> 8;
    pkt[pos++] = type;
    pkt[pos++] = 0;
    pkt[pos++] = DNS_CLASS_IN;
    for (i = 0; i < DISC_RECORDS; i++) {
        const struct DiscoveryRecord *r = &d->records[i];
        LONG ownerLen = key_length(r->key);
        ULONG ttl;

        if (r->type != type || (link != 0 && r->link != link) ||
            !same_key(r->key, key) || due(now, r->expires) ||
            (LONG)(r->expires - now) <= (LONG)(r->ttl * 25)) {
            continue;
        }
        if (pos + ownerLen + 10 + r->length > (LONG)sizeof(pkt)) {
            break; /* omitting known answers only causes extra answers */
        }
        memcpy(pkt + pos, r->key, ownerLen);
        pos += ownerLen;
        pkt[pos++] = type >> 8;
        pkt[pos++] = type;
        pkt[pos++] = 0;
        pkt[pos++] = DNS_CLASS_IN;
        ttl = (r->expires - now) / 50;
        pkt[pos++] = ttl >> 24;
        pkt[pos++] = ttl >> 16;
        pkt[pos++] = ttl >> 8;
        pkt[pos++] = ttl;
        pkt[pos++] = r->length >> 8;
        pkt[pos++] = r->length;
        memcpy(pkt + pos, r->data, r->length);
        pos += r->length;
        answers++;
    }
    pkt[6] = answers >> 8;
    pkt[7] = answers;
    for (i = 0; i < md->md_IfCount; i++) {
        if (link == 0 || md->md_Ifs[i].mi_Addr == link) {
            mdnsNetSendOn(md, i, pkt, pos);
        }
    }
}

static struct DiscoveryView *view_of(struct MDNSDiscovery *d,
    const struct DiscoveryRecord *ptr)
{
    LONG j;

    for (j = 0; j < DISC_VIEWS; j++) {
        if (d->views[j].key[0] != 0 && d->views[j].link == ptr->link &&
            same_key(d->views[j].key, ptr->data)) {
            return &d->views[j];
        }
    }
    return NULL;
}

static void refresh_views(struct MDNSDaemon *md, ULONG now)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i, j;

    /* Which views the browsed PTRs still have; the others go first, so
     * that their room can be taken by new instances. */
    for (i = 0; i < DISC_VIEWS; i++) {
        d->views[i].active = FALSE;
    }
    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        struct DiscoveryView *view;

        if (r->type == DNS_TYPE_PTR && !due(now, r->expires) &&
            browsed(d, r->key) && (view = view_of(d, r)) != NULL) {
            view->active = TRUE;
        }
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *v = &d->views[i];

        if (v->key[0] != 0 && !v->active) {
            if (v->present) {
                notify(d, v, MEVENT_REMOVE);
            }
            memset(v, 0, sizeof(*v));
        }
    }
    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        struct DiscoveryView *view;

        if (r->type != DNS_TYPE_PTR || due(now, r->expires) ||
            !browsed(d, r->key) || view_of(d, r) != NULL) {
            continue;
        }
        for (view = NULL, j = 0; j < DISC_VIEWS; j++) {
            if (d->views[j].key[0] == 0) {
                view = &d->views[j];
                memcpy(view->key, r->data, 256);
                view->link = r->link;
                view->active = TRUE;
                break;
            }
        }
        if (view == NULL) {
            /* More instances of the type than there are views: those who
             * browse it are failed, once, and its records are no longer
             * wanted; nobody else is touched. */
            for (j = 0; j < DISC_SESSIONS; j++) {
                struct DiscoverySession *s = &d->sessions[j];

                if (live(s) && !s->resolve && same_key(s->key, r->key)) {
                    fail(d, s);
                }
            }
        }
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *v = &d->views[i];
        struct MDNSDiscoveryEvent event;
        BOOL present;

        if (v->key[0] == 0) {
            continue;
        }
        present = snapshot(d, v->key, v->link, &event, now);
        if (v->present && (!present || memcmp(&event, &v->event, sizeof(event)) != 0)) {
            notify(d, v, MEVENT_REMOVE);
        }
        if (present && (!v->present || memcmp(&event, &v->event, sizeof(event)) != 0)) {
            v->event = event;
            notify(d, v, MEVENT_ADD);
        }
        v->present = present;
    }
}

void mdnsDiscoveryTick(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    ULONG now = mdnsTicks();
    LONG i, j;
    BOOL interested = FALSE, rebuild;

    if (d == NULL) {
        return;
    }
    /* This runs on every turn of mdnsd's loop. The views and resolutions
     * are looked at again only when something they depend on changed. */
    rebuild = d->changed || (d->expiring && due(now, d->nextExpiry));
    if (rebuild) {
        d->changed = FALSE;
        refresh_views(md, now);
        d->expiring = FALSE;
        for (i = 0; i < DISC_RECORDS; i++) {
            const struct DiscoveryRecord *r = &d->records[i];

            if (r->type != 0 && !due(now, r->expires) &&
                (!d->expiring || (LONG)(r->expires - d->nextExpiry) < 0)) {
                d->nextExpiry = r->expires;
                d->expiring = TRUE;
            }
        }
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (s->handle == NULL) {
            continue;
        }
        if (s->resolve && !s->resolved && !s->failed) {
            struct MDNSDiscoveryEvent event;

            for (j = 0; rebuild && j < md->md_IfCount; j++) {
                if (snapshot(d, s->key, md->md_Ifs[j].mi_Addr, &event, now)) {
                    event.de_Code = MEVENT_RESOLVED;
                    enqueue(s, &event);
                    s->resolved = TRUE;
                    d->changed = TRUE;
                    break;
                }
            }
            if (!s->resolved && due(now, s->deadline)) {
                memset(&event, 0, sizeof(event));
                event.de_Code = MEVENT_RESOLVE_FAILED;
                enqueue(s, &event);
                s->resolved = TRUE;
                d->changed = TRUE;
            }
        }
        if (!s->failed && (!s->resolve || !s->resolved)) {
            interested = TRUE;
        }
        if (s->failed || (s->resolve && s->resolved) || md->md_IfCount == 0 ||
            md->md_Socket < 0 || !due(now, s->nextQuery)) {
            continue;
        }
        send_question(md, s->key, s->resolve ? DNS_TYPE_SRV : DNS_TYPE_PTR, 0);
        if (s->resolve) {
            send_question(md, s->key, DNS_TYPE_TXT, 0);
        }
        s->nextQuery = now + s->interval;
        if (s->interval < 180000) {
            s->interval *= 2;
        }
        if (s->interval > 180000) {
            s->interval = 180000;
        }
    }
    /* Resolve missing components at most once per second; independently
     * refresh records at 80/85/90/95 percent of their own TTL. */
    if (!interested || !due(now, d->lastRefresh + 50) || md->md_IfCount == 0 || md->md_Socket < 0) {
        return;
    }
    d->lastRefresh = now;
    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        BOOL missing = FALSE;

        if (r->type == 0 || due(now, r->expires) ||
            !wanted(d, r->type, r->key, r->link, now)) {
            continue;
        }
        if (r->refreshStep < 4 && due(now, r->refresh)) {
            send_question(md, r->key, r->type, r->link);
            r->refreshStep++;
            r->refresh = r->received + r->ttl * (40 + 2 * r->refreshStep) +
                (r->ttl * r->refreshStep) / 2 + r->ttl * (r->received % 2);
        }
        if (!due(now, r->nextMissing)) {
            continue;
        }
        if (r->type == DNS_TYPE_PTR) {
            if (find_record(d, r->data, r->link, DNS_TYPE_SRV, now) == NULL) {
                send_question(md, r->data, DNS_TYPE_SRV, r->link);
                missing = TRUE;
            }
            if (find_record(d, r->data, r->link, DNS_TYPE_TXT, now) == NULL) {
                send_question(md, r->data, DNS_TYPE_TXT, r->link);
                missing = TRUE;
            }
        } else if (r->type == DNS_TYPE_SRV &&
            find_record(d, r->data + 6, r->link, DNS_TYPE_A, now) == NULL) {
            send_question(md, r->data + 6, DNS_TYPE_A, r->link);
            missing = TRUE;
        }
        if (!missing) {
            r->missingInterval = 50;
        }
        r->nextMissing = now + r->missingInterval;
        if (missing && r->missingInterval < 180000) {
            r->missingInterval *= 2;
        }
        if (r->missingInterval > 180000) {
            r->missingInterval = 180000;
        }
    }
}

LONG mdnsDiscoveryMessage(struct MDNSDaemon *md, struct MDNSIPCMsg *msg)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    struct DiscoverySession *s = NULL;
    LONG i;
    BOOL create = msg->mi_Command == MDNS_IPC_BROWSE || msg->mi_Command == MDNS_IPC_RESOLVE;

    if (!create && msg->mi_DaemonID != md->md_DaemonID) {
        return MDNSERR_NOENGINE;
    }
    if (create && d == NULL) {
        d = AllocMem(sizeof(*d), MEMF_CLEAR);
        if (d == NULL) {
            return MDNSERR_NOMEM;
        }
        d->nextHandle = 1;
        md->md_Discovery = d;
    }
    if (d == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (create) {
        LONG owned = 0;

        for (i = 0; i < DISC_SESSIONS; i++) {
            if (d->sessions[i].handle != NULL && d->sessions[i].owner == msg->mi_TaskID) {
                owned++;
            }
        }
        if (owned >= 4) {
            return MDNSERR_BUSY;
        }
        for (i = 0; i < DISC_SESSIONS; i++) {
            if (d->sessions[i].handle == NULL) {
                s = &d->sessions[i];
                break;
            }
        }
        if (s == NULL || d->nextHandle == 0) {
            return MDNSERR_BUSY;
        }
        memset(s, 0, sizeof(*s));
        s->resolve = msg->mi_Command == MDNS_IPC_RESOLVE;
        if (!type_key(msg->mi_ServiceType, s->key)) {
            return MDNSERR_BADTYPE;
        }
        if (s->resolve) {
            LONG length = mdnsTextLength(msg->mi_Name, 64);
            LONG typeLen = key_length(s->key);

            if (length == 0 || length > 63) {
                return MDNSERR_BADPARAM;
            }
            memmove(s->key + length + 1, s->key, typeLen);
            s->key[0] = length;
            memcpy(s->key + 1, msg->mi_Name, length);
        }
        s->owner = msg->mi_TaskID;
        s->handle = (APTR)d->nextHandle++;
        s->interval = 50;
        s->nextQuery = mdnsTicks() + 1 + (mdnsTicks() + s->owner) % 6;
        s->deadline = mdnsTicks() + 250; /* bounded asynchronous resolution */
        msg->mi_Handle = s->handle;
        msg->mi_DaemonID = md->md_DaemonID;
        d->changed = TRUE;
        for (i = 0; i < DISC_VIEWS; i++) {
            const struct DiscoveryView *v = &d->views[i];

            if (!s->resolve && v->present && same_key(s->key, v->key + v->key[0] + 1)) {
                struct MDNSDiscoveryEvent event = v->event;

                event.de_Code = MEVENT_ADD;
                enqueue(s, &event);
            }
        }
        return MDNS_OK;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        if (d->sessions[i].handle == msg->mi_Handle &&
            d->sessions[i].owner == msg->mi_TaskID) {
            s = &d->sessions[i];
            break;
        }
    }
    if (s == NULL) {
        return MDNSERR_BADPARAM;
    }
    if (msg->mi_Command == MDNS_IPC_READ) {
        if (s->failed) {
            return MDNSERR_BUSY;
        }
        if (s->count == 0) {
            return MDNSERR_NOTREADY;
        }
        msg->mi_Event = s->queue[s->head];
        s->head = (s->head + 1) % DISC_QUEUE;
        s->count--;
        return MDNS_OK;
    }
    if (msg->mi_Command == MDNS_IPC_NOTIFY) {
        /* One lent request per operation. What is already waiting
         * answers it at once, before this call's own reply. */
        if (msg->mi_Request == NULL || s->request != NULL) {
            return MDNSERR_BADPARAM;
        }
        s->request = msg->mi_Request;
        if (s->failed) {
            wake(s, MDNSERR_BUSY);
        } else if (s->count != 0) {
            wake(s, MDNS_OK);
        }
        return MDNS_OK;
    }
    if ((msg->mi_Command == MDNS_IPC_STOPRESOLVE) != s->resolve) {
        return MDNSERR_BADPARAM;
    }
    /* The request goes back before the stop's own reply, so the owner has
     * it again when StopBrowse()/StopResolve() returns. */
    wake(s, MDNSERR_CANCELLED);
    memset(s, 0, sizeof(*s));
    d->changed = TRUE;
    return MDNS_OK;
}

/* mdnsd is ending: every lent request goes back with MDNSERR_NOENGINE. */
void mdnsDiscoveryCancel(struct MDNSDaemon *md)
{
    LONG i;

    if (md->md_Discovery == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        wake(&md->md_Discovery->sessions[i], MDNSERR_NOENGINE);
    }
}

/* The owner's last close of the library. A request it still lent is
 * dropped unread, like one of a vanished task: it should have stopped the
 * operation first, and its reply port may already be gone. */
void mdnsDiscoveryRelease(struct MDNSDaemon *md, ULONG owner)
{
    LONG i;

    if (md->md_Discovery == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        if (md->md_Discovery->sessions[i].owner == owner) {
            memset(&md->md_Discovery->sessions[i], 0, sizeof(struct DiscoverySession));
            md->md_Discovery->changed = TRUE;
        }
    }
}

/* On mdnsd's reap timer: the operations of a task that ended without
 * stopping them go, a request it lent with them, unread. */
void mdnsDiscoveryReap(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;

    if (d == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (s->handle != NULL && FindTaskByPID(s->owner) == NULL) {
            memset(s, 0, sizeof(*s));
            d->changed = TRUE;
        }
    }
}
void mdnsDiscoveryReset(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i;

    if (d == NULL) {
        return;
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        if (d->views[i].present) {
            notify(d, &d->views[i], MEVENT_REMOVE);
        }
    }
    memset(d->records, 0, sizeof(d->records));
    memset(d->views, 0, sizeof(d->views));
    memset(d->pending, 0, sizeof(d->pending));
    d->changed = TRUE;
    d->expiring = FALSE;
    for (i = 0; i < DISC_SESSIONS; i++) {
        d->sessions[i].nextQuery = mdnsTicks();
        d->sessions[i].interval = 50;
    }
}
void mdnsDiscoveryFree(struct MDNSDaemon *md)
{
    if (md->md_Discovery != NULL) {
        FreeMem(md->md_Discovery, sizeof(*md->md_Discovery));
        md->md_Discovery = NULL;
    }
}
