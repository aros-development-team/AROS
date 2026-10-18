/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * Bounded, daemon-owned DNS-SD cache and polling queues. DNS names remain
 * counted labels: dots/backslashes in an instance never become separators.
 * All entry points run in mdnsd's task; none calls client code or retains a
 * client event buffer, Hook, port or task pointer.
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

struct DiscoveryRecord
{
    UBYTE key[256];
    UBYTE data[512];
    UWORD type, length;
    ULONG link, expires, received, refresh, ttl;
    UWORD refreshStep;
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
};
struct MDNSDiscovery
{
    struct DiscoveryRecord records[DISC_RECORDS];
    struct DiscoveryView views[DISC_VIEWS];
    struct DiscoverySession sessions[DISC_SESSIONS];
    IPTR nextHandle;
    ULONG lastRefresh;
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

static void fail_all(struct MDNSDiscovery *d)
{
    LONG i;

    for (i = 0; i < DISC_SESSIONS; i++) {
        if (d->sessions[i].handle != NULL) {
            d->sessions[i].failed = TRUE;
            d->sessions[i].count = 0;
        }
    }
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
            return;
        }
    }
    if (s->count == DISC_QUEUE) {
        s->count = 0;
        s->failed = TRUE;
        return;
    }
    s->queue[(s->head + s->count++) % DISC_QUEUE] = *event;
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
            (found == NULL || (LONG)(r->received - found->received) > 0)) {
            found = r;
        }
    }
    return found;
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

void mdnsDiscoveryRecord(struct MDNSDaemon *md, const struct MDNSRecord *record,
    const UBYTE *pkt, LONG length, LONG ownerOffset)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    UBYTE key[256] = {0}, data[512] = {0};
    LONG i, size = record->rdlength, consumed;
    struct DiscoveryRecord *slot = NULL;
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
    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        BOOL match = r->type == record->type && r->link == link && same_key(r->key, key);
        BOOL equalData = r->length == size && memcmp(r->data, data, size) == 0;
        BOOL identical;

        if (record->type == DNS_TYPE_PTR) {
            equalData = same_key(r->data, data);
        } else if (record->type == DNS_TYPE_SRV) {
            equalData = memcmp(r->data, data, 6) == 0 && same_key(r->data + 6, data + 6);
        }
        identical = match && equalData;

        if (identical) {
            slot = r;
        } else if (match && (record->rclass & 0x8000) != 0 &&
            record->type != DNS_TYPE_PTR && record->ttl != 0 &&
            due(now, r->received + 50) && !due(now + 50, r->expires)) {
            r->expires = now + 50;   /* cache flush: grace, not instant removal */
        }
    }
    if (record->ttl == 0) {
        if (slot != NULL && !due(now + 50, slot->expires)) {
            slot->expires = now + 50; /* another responder may rescue it */
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
        fail_all(d);
        return;
    }
    memset(slot, 0, sizeof(*slot));
    memcpy(slot->key, key, sizeof(key));
    memcpy(slot->data, data, size);
    slot->length = size;
    slot->link = link;
    slot->type = record->type;
    slot->ttl = record->ttl > DISC_TTL_MAX ? DISC_TTL_MAX : record->ttl;
    slot->received = now;
    slot->expires = now + slot->ttl * 50;
    slot->refresh = now + slot->ttl * (40 + now % 2); /* 80–82 percent */
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

static void refresh_views(struct MDNSDaemon *md, ULONG now)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    LONG i, j;

    for (i = 0; i < DISC_VIEWS; i++) {
        d->views[i].active = FALSE;
    }
    for (i = 0; i < DISC_RECORDS; i++) {
        struct DiscoveryRecord *r = &d->records[i];
        struct DiscoveryView *view = NULL;

        if (r->type != DNS_TYPE_PTR || due(now, r->expires)) {
            continue;
        }
        for (j = 0; j < DISC_VIEWS; j++) {
            if (d->views[j].key[0] != 0 && d->views[j].link == r->link &&
                same_key(d->views[j].key, r->data)) {
                view = &d->views[j];
                break;
            }
        }
        if (view == NULL) {
            for (j = 0; j < DISC_VIEWS; j++) {
                if (d->views[j].key[0] == 0) {
                    view = &d->views[j];
                    memcpy(view->key, r->data, 256);
                    view->link = r->link;
                    break;
                }
            }
        }
        if (view == NULL) {
            fail_all(d);
            return;
        }
        view->active = TRUE;
    }
    for (i = 0; i < DISC_VIEWS; i++) {
        struct DiscoveryView *v = &d->views[i];
        struct MDNSDiscoveryEvent event;
        BOOL present;

        if (v->key[0] == 0) {
            continue;
        }
        present = v->active && snapshot(d, v->key, v->link, &event, now);
        if (v->present && (!present || memcmp(&event, &v->event, sizeof(event)) != 0)) {
            notify(d, v, MEVENT_REMOVE);
        }
        if (present && (!v->present || memcmp(&event, &v->event, sizeof(event)) != 0)) {
            v->event = event;
            notify(d, v, MEVENT_ADD);
        }
        v->present = present;
        if (!v->active) {
            memset(v, 0, sizeof(*v));
        }
    }
}

void mdnsDiscoveryTick(struct MDNSDaemon *md)
{
    struct MDNSDiscovery *d = md->md_Discovery;
    ULONG now = mdnsTicks();
    LONG i, j;
    BOOL interested = FALSE;

    if (d == NULL) {
        return;
    }
    refresh_views(md, now);
    for (i = 0; i < DISC_SESSIONS; i++) {
        struct DiscoverySession *s = &d->sessions[i];

        if (s->handle == NULL) {
            continue;
        }
        if (FindTaskByPID(s->owner) == NULL) {
            memset(s, 0, sizeof(*s));
            continue;
        }
        if (s->resolve && !s->resolved && !s->failed) {
            struct MDNSDiscoveryEvent event;

            for (j = 0; j < md->md_IfCount; j++) {
                if (snapshot(d, s->key, md->md_Ifs[j].mi_Addr, &event, now)) {
                    event.de_Code = MEVENT_RESOLVED;
                    enqueue(s, &event);
                    s->resolved = TRUE;
                    break;
                }
            }
            if (!s->resolved && due(now, s->deadline)) {
                memset(&event, 0, sizeof(event));
                event.de_Code = MEVENT_RESOLVE_FAILED;
                enqueue(s, &event);
                s->resolved = TRUE;
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

        if (r->type == 0 || due(now, r->expires)) {
            continue;
        }
        if (r->refreshStep < 4 && due(now, r->refresh)) {
            send_question(md, r->key, r->type, r->link);
            r->refreshStep++;
            r->refresh = r->received + r->ttl * (40 + 2 * r->refreshStep) +
                (r->ttl * r->refreshStep) / 2 + r->ttl * (r->received % 2);
        }
        if (r->type == DNS_TYPE_PTR) {
            if (find_record(d, r->data, r->link, DNS_TYPE_SRV, now) == NULL) {
                send_question(md, r->data, DNS_TYPE_SRV, r->link);
            }
            if (find_record(d, r->data, r->link, DNS_TYPE_TXT, now) == NULL) {
                send_question(md, r->data, DNS_TYPE_TXT, r->link);
            }
        } else if (r->type == DNS_TYPE_SRV &&
            find_record(d, r->data + 6, r->link, DNS_TYPE_A, now) == NULL) {
            send_question(md, r->data + 6, DNS_TYPE_A, r->link);
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
    if ((msg->mi_Command == MDNS_IPC_STOPRESOLVE) != s->resolve) {
        return MDNSERR_BADPARAM;
    }
    memset(s, 0, sizeof(*s));
    return MDNS_OK;
}

void mdnsDiscoveryRelease(struct MDNSDaemon *md, ULONG owner)
{
    LONG i;

    if (md->md_Discovery == NULL) {
        return;
    }
    for (i = 0; i < DISC_SESSIONS; i++) {
        if (md->md_Discovery->sessions[i].owner == owner) {
            memset(&md->md_Discovery->sessions[i], 0, sizeof(struct DiscoverySession));
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
