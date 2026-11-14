/*
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright 2026 Jaime Dias
 *
 * mdns_cache.c - the records heard on the links, and the questions that
 * keep them (RFC 6762 sections 5.2, 7.1, 10)
 *
 * Every answer mdnsd hears goes into one cache: owner name and the names in
 * the RDATA as uncompressed wire labels (so a dot inside an instance name
 * is never a separator), type, class, the link it came by, its TTL. What a
 * client wants is a watch: a question (name, type, one link or all) asked
 * on the links as RFC 6762 says - first within 20-120 ms, then after 1, 2,
 * 4 s... up to an hour, with the answers already known in it (section
 * 7.1) - and a function told of each record that matches as it comes and
 * as it goes. A record watched is asked for again at 80, 85, 90 and 95 % of
 * its life (section 5.2); one a cache-flush answer replaces, or a goodbye
 * (TTL 0), goes a second later (sections 10.2, 10.1).
 *
 * The host's own records come in as they are announced (mdnsReflectPacket())
 * and stay until their goodbye, which ends them a second later as any:
 * nobody refreshes them.
 *
 * Memory: each record is allocated by itself, within MDNS_CACHE_BYTES; when
 * that is full, a record nobody watches goes first, the one nearest its end;
 * then, only if none is left, a watched one.
 *
 * The questions due on a link at one time go out together, in one query
 * (more than one only when they do not fit), each with its known answers.
 *
 * This is mdnsd's only cache: dnssd.library's operations (dnssd_server.c)
 * and bonami.library's browses (mdns_discovery.c) are watches on it.
 */
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>
#include <netinet/in.h>
#include "mdnsd.h"

#define TICKS               50          /* mdnsTicks() per second */
#define QUERY_MAX_INTERVAL  (3600 * TICKS)

struct CacheWatch
{
    struct MinNode      cw_Node;
    struct CacheQuestion *cw_Question;
    MDNSCacheNotify     cw_Notify;
    APTR                cw_Data;
};

struct CacheQuestion
{
    struct MinNode  cq_Node;
    UBYTE           cq_Name[256];
    UWORD           cq_Type;
    BOOL            cq_Asking;          /* due in this turn */
    ULONG           cq_Link;            /* 0: every link */
    ULONG           cq_Next;            /* mdnsTicks() of the next query */
    ULONG           cq_Interval;
    struct MinList  cq_Watches;
};

/* a record whose refresh is asked for in this turn */
#define CACHE_ASKING        0x80

/* A query that goes out: its questions, each a name and a type. */
#define ASK_MAX             16
#define ASK_PACKET          1400        /* one Ethernet frame */

struct CacheAsk
{
    const UBYTE    *ca_Name;
    UWORD           ca_Type;
    LONG            ca_Offset;          /* where its name is in the packet */
};

struct MDNSCache
{
    struct MinList  mc_Records;
    struct MinList  mc_Questions;
    ULONG           mc_Bytes;
    ULONG           mc_Seed;
    BOOL            mc_Notifying;
};

static BOOL
due(ULONG now, ULONG at)
{
    return (LONG)(now - at) >= 0;
}

static ULONG
random_ticks(struct MDNSCache *c, ULONG low, ULONG high)
{
    c->mc_Seed = c->mc_Seed * 1103515245UL + 12345UL;
    return low + (c->mc_Seed >> 16) % (high - low + 1);
}

/*
 * The RDATA of a record at rdoffset (rdlength bytes) with the names in it
 * expanded, into data (MDNS_MAX_PACKET bytes); its length, or -1.
 */
static LONG
expand_rdata(UWORD type, const UBYTE *pkt, LONG length, LONG rdoffset,
    UWORD rdlength, UBYTE *data)
{
    LONG n;

    if (rdoffset + rdlength > length) {
        return -1;
    }
    switch (type) {
    /* a name in the data ends where the data does, or the record is
     * not what it says */
    case DNS_TYPE_PTR:
    case 5:     /* CNAME */
    case 2:     /* NS */
        if (mdnsSkipName(pkt, length, rdoffset) != rdoffset + rdlength ||
            mdnsWireNameRead(pkt, length, rdoffset, data) < 0) {
            return -1;
        }
        return mdnsWireNameLength(data);
    case DNS_TYPE_SRV:
        if (rdlength < 7 ||
            mdnsSkipName(pkt, length, rdoffset + 6) != rdoffset + rdlength) {
            return -1;
        }
        memcpy(data, pkt + rdoffset, 6);
        if (mdnsWireNameRead(pkt, length, rdoffset + 6, data + 6) < 0) {
            return -1;
        }
        n = mdnsWireNameLength(data + 6);
        return n < 0 ? -1 : 6 + n;
    default:
        if (rdlength > MDNS_MAX_PACKET) {
            return -1;
        }
        memcpy(data, pkt + rdoffset, rdlength);
        return rdlength;
    }
}

/* Is a record still there: not one at its end that the next turn takes. */
static BOOL
alive(const struct MDNSCacheEntry *e, ULONG now)
{
    return !due(now, e->ce_Expires) ||
        ((e->ce_Flags & MDNS_CACHE_LOCAL) && e->ce_TTL != 0);
}

static BOOL
matches(const struct CacheQuestion *q, const struct MDNSCacheEntry *e)
{
    return (q->cq_Link == 0 || q->cq_Link == e->ce_Link) &&
        (q->cq_Type == DNS_TYPE_ANY || q->cq_Type == e->ce_Type) &&
        mdnsWireNameEqual(q->cq_Name, e->ce_Name);
}

static void
tell(struct MDNSCache *c, const struct MDNSCacheEntry *e, BOOL added)
{
    struct CacheQuestion *q, *qn;
    struct CacheWatch *w, *wn;

    /* a watch may end itself (or another) from its function */
    c->mc_Notifying = TRUE;
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         (qn = (struct CacheQuestion *)q->cq_Node.mln_Succ) != NULL; q = qn) {
        if (!matches(q, e)) {
            continue;
        }
        for (w = (struct CacheWatch *)q->cq_Watches.mlh_Head;
             (wn = (struct CacheWatch *)w->cw_Node.mln_Succ) != NULL; w = wn) {
            if (w->cw_Notify != NULL) {
                w->cw_Notify(w->cw_Data, e, added);
            }
        }
    }
    c->mc_Notifying = FALSE;
    /* the watches ended while being told are freed now */
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         (qn = (struct CacheQuestion *)q->cq_Node.mln_Succ) != NULL; q = qn) {
        for (w = (struct CacheWatch *)q->cq_Watches.mlh_Head;
             (wn = (struct CacheWatch *)w->cw_Node.mln_Succ) != NULL; w = wn) {
            if (w->cw_Notify == NULL) {
                Remove((struct Node *)w);
                FreeVec(w);
            }
        }
        if (IsListEmpty((struct List *)&q->cq_Watches)) {
            Remove((struct Node *)q);
            FreeVec(q);
        }
    }
}

static void
drop(struct MDNSCache *c, struct MDNSCacheEntry *e)
{
    Remove((struct Node *)e);
    c->mc_Bytes -= e->ce_Size;
    tell(c, e, FALSE);
    FreeVec(e);
}

static BOOL watched(const struct MDNSCache *c, const struct MDNSCacheEntry *e);

/* Room for bytes more: of the records nobody watches, the one nearest its
 * end goes; when every one is watched, the watched one nearest its end. The
 * host's own records stay. */
static void
make_room(struct MDNSCache *c, ULONG bytes)
{
    while (c->mc_Bytes + bytes > MDNS_CACHE_BYTES) {
        struct MDNSCacheEntry *e, *oldest = NULL;
        BOOL oldestWatched = TRUE;

        for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
             e->ce_Node.mln_Succ != NULL;
             e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
            BOOL w;

            if (e->ce_Flags & MDNS_CACHE_LOCAL) {
                continue;
            }
            w = watched(c, e);
            if (oldest == NULL || (oldestWatched && !w) ||
                (w == oldestWatched &&
                 (LONG)(e->ce_Expires - oldest->ce_Expires) < 0)) {
                oldest = e;
                oldestWatched = w;
            }
        }
        if (oldest == NULL) {
            return;
        }
        drop(c, oldest);
    }
}

void
mdnsCacheRecord(const struct MDNSDaemon *md, const UBYTE *pkt, LONG length,
    LONG offset, ULONG link, BOOL local)
{
    struct MDNSCache *c = md->md_Cache;
    /* RDATA up to a packet's size (a TXT may be 8900 bytes, RFC 6763
     * section 6.2), in storage of its own: mdnsd has one task, and the
     * stack of a 68000's task is small */
    static UBYTE data[MDNS_MAX_PACKET];
    UBYTE name[256];
    struct MDNSCacheEntry *e, *next, *same = NULL;
    LONG consumed, namelen, datalen;
    UWORD type, rclass, rdlength;
    ULONG ttl, now = mdnsTicks(), expires;
    BOOL flush;

    if (c == NULL || link == 0 ||
        (consumed = mdnsWireNameRead(pkt, length, offset, name)) < 0 ||
        offset + consumed + 10 > length) {
        return;
    }
    offset += consumed;
    type = (UWORD)((pkt[offset] << 8) | pkt[offset + 1]);
    rclass = (UWORD)((pkt[offset + 2] << 8) | pkt[offset + 3]);
    ttl = ((ULONG)pkt[offset + 4] << 24) | ((ULONG)pkt[offset + 5] << 16) |
          ((ULONG)pkt[offset + 6] << 8) | pkt[offset + 7];
    rdlength = (UWORD)((pkt[offset + 8] << 8) | pkt[offset + 9]);
    flush = (rclass & 0x8000) != 0;
    rclass &= 0x7fff;
    if (rclass != DNS_CLASS_IN || type == DNS_TYPE_ANY || type == 41 /* OPT */ ||
        (datalen = expand_rdata(type, pkt, length, offset + 10, rdlength, data)) < 0) {
        return;
    }
    if (ttl > 0x7fffffffUL / TICKS) {
        ttl = 0x7fffffffUL / TICKS;
    }
    namelen = mdnsWireNameLength(name);
    /* a goodbye (TTL 0) ends the record a second from now */
    expires = now + (ttl == 0 ? TICKS : ttl * TICKS);

    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         (next = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) != NULL; e = next) {
        if (e->ce_Link != link || e->ce_Type != type ||
            !mdnsWireNameEqual(e->ce_Name, name)) {
            continue;
        }
        if (e->ce_DataLength == datalen &&
            memcmp(e->ce_Data, data, datalen) == 0) {
            same = e;
        } else if (flush && !local && !(e->ce_Flags & MDNS_CACHE_LOCAL) &&
                   due(now, e->ce_Received + TICKS) &&
                   (LONG)(e->ce_Expires - (now + TICKS)) > 0) {
            /* the sender holds the whole set: the others go in a second */
            e->ce_Expires = now + TICKS;
        }
    }
    if (same != NULL) {
        if (ttl == 0 && local != ((same->ce_Flags & MDNS_CACHE_LOCAL) != 0)) {
            return;
        }
        same->ce_Received = now;
        same->ce_TTL = ttl;
        same->ce_Expires = expires;
        same->ce_Refresh = 0;
        return;
    }
    if (ttl == 0) {
        return;
    }
    make_room(c, sizeof(*e) + namelen + datalen);
    e = AllocVec(sizeof(*e) + namelen + datalen, MEMF_ANY | MEMF_CLEAR);
    if (e == NULL) {
        return;
    }
    e->ce_Size = sizeof(*e) + namelen + datalen;
    e->ce_Name = (UBYTE *)(e + 1);
    e->ce_Data = e->ce_Name + namelen;
    memcpy(e->ce_Name, name, namelen);
    memcpy(e->ce_Data, data, datalen);
    e->ce_NameLength = (UWORD)namelen;
    e->ce_DataLength = (UWORD)datalen;
    e->ce_Type = type;
    e->ce_Link = link;
    e->ce_TTL = ttl;
    e->ce_Received = now;
    e->ce_Expires = expires;
    e->ce_Flags = local ? MDNS_CACHE_LOCAL : 0;
    AddTail((struct List *)&c->mc_Records, (struct Node *)e);
    c->mc_Bytes += e->ce_Size;
    tell(c, e, TRUE);
}

/*
 * A query on link (a slot) with count questions, and after them the answers
 * known there that have more than half their life left (RFC 6762 section
 * 7.1), each named by a pointer to its question's name. Answers that do not
 * fit are left out, which only brings more answers.
 */
static void
ask_send(struct MDNSDaemon *md, LONG slot, struct CacheAsk *asks, UWORD count)
{
    const struct MDNSCache *c = md->md_Cache;
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSCacheEntry *e;
    ULONG now = mdnsTicks();
    LONG at = 12;
    UWORD answers = 0, i;
    UWORD savedFamily = md->md_SendFamily;
    BOOL full = FALSE;

    if (pkt == NULL || count == 0) {
        return;
    }
    memset(pkt, 0, 12);
    pkt[4] = (UBYTE)(count >> 8);
    pkt[5] = (UBYTE)count;
    for (i = 0; i < count; i++) {
        LONG namelen = mdnsWireNameLength(asks[i].ca_Name);

        asks[i].ca_Offset = at;
        memcpy(pkt + at, asks[i].ca_Name, namelen);
        at += namelen;
        pkt[at++] = (UBYTE)(asks[i].ca_Type >> 8);
        pkt[at++] = (UBYTE)asks[i].ca_Type;
        pkt[at++] = 0;
        pkt[at++] = DNS_CLASS_IN;
    }
    for (i = 0; i < count && !full; i++) {
        for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
             e->ce_Node.mln_Succ != NULL;
             e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
            LONG left, rdlen;

            if (e->ce_Link != md->md_Ifs[slot].mi_Index ||
                (asks[i].ca_Type != DNS_TYPE_ANY && e->ce_Type != asks[i].ca_Type) ||
                !mdnsWireNameEqual(e->ce_Name, asks[i].ca_Name) ||
                (e->ce_Flags & MDNS_CACHE_LOCAL)) {
                continue;
            }
            left = (LONG)(e->ce_Expires - now) / TICKS;
            if (left <= 0 || (ULONG)left * 2 < e->ce_TTL) {
                continue;
            }
            rdlen = e->ce_DataLength;
            if (at + 2 + 10 + rdlen > ASK_PACKET) {
                full = TRUE;
                break;
            }
            pkt[at++] = (UBYTE)(0xc0 | (asks[i].ca_Offset >> 8));
            pkt[at++] = (UBYTE)asks[i].ca_Offset;
            pkt[at++] = (UBYTE)(e->ce_Type >> 8);
            pkt[at++] = (UBYTE)e->ce_Type;
            pkt[at++] = 0;
            pkt[at++] = DNS_CLASS_IN;
            pkt[at++] = (UBYTE)(left >> 24);
            pkt[at++] = (UBYTE)(left >> 16);
            pkt[at++] = (UBYTE)(left >> 8);
            pkt[at++] = (UBYTE)left;
            pkt[at++] = (UBYTE)(rdlen >> 8);
            pkt[at++] = (UBYTE)rdlen;
            memcpy(pkt + at, e->ce_Data, rdlen);
            at += rdlen;
            answers++;
        }
    }
    pkt[6] = (UBYTE)(answers >> 8);
    pkt[7] = (UBYTE)answers;
    md->md_SendFamily = 0;              /* both families of the link */
    mdnsNetSendOn(md, slot, pkt, at);
    md->md_SendFamily = savedFamily;
}

/* One more question for a query being made on slot: sent first when the
 * query is full; one already in it is not asked twice. */
static void
ask_add(struct MDNSDaemon *md, LONG slot, struct CacheAsk *asks, UWORD *count,
    LONG *size, const UBYTE *name, UWORD type)
{
    LONG namelen = mdnsWireNameLength(name);
    UWORD i;

    for (i = 0; i < *count; i++) {
        if (asks[i].ca_Type == type && mdnsWireNameEqual(asks[i].ca_Name, name)) {
            return;
        }
    }
    if (*count == ASK_MAX || *size + namelen + 4 > ASK_PACKET) {
        ask_send(md, slot, asks, *count);
        *count = 0;
        *size = 12;
    }
    asks[*count].ca_Name = name;
    asks[*count].ca_Type = type;
    (*count)++;
    *size += namelen + 4;
}

static BOOL
link_active(const struct MDNSDaemon *md, ULONG link)
{
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        if (md->md_Ifs[i].mi_Index == link) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
watched(const struct MDNSCache *c, const struct MDNSCacheEntry *e)
{
    const struct CacheQuestion *q;

    for (q = (const struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (const struct CacheQuestion *)q->cq_Node.mln_Succ) {
        if (matches(q, e)) {
            return TRUE;
        }
    }
    return FALSE;
}

/* The next of 80, 85, 90, 95 % of a record's life, with up to 2 % more
 * (RFC 6762 section 5.2). */
static ULONG
refresh_at(struct MDNSCache *c, const struct MDNSCacheEntry *e)
{
    ULONG life = e->ce_TTL * TICKS;
    ULONG percent = 80 + 5 * e->ce_Refresh;

    return e->ce_Received + life / 100 * percent +
        random_ticks(c, 0, life / 50 + 1);
}

void
mdnsCacheTick(struct MDNSDaemon *md)
{
    struct MDNSCache *c = md->md_Cache;
    struct MDNSCacheEntry *e, *next;
    struct CacheQuestion *q;
    ULONG now = mdnsTicks();
    BOOL asking = FALSE;
    LONG i;

    if (c == NULL) {
        return;
    }
    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         (next = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) != NULL; e = next) {
        /* one of the host's own goes with its goodbye (TTL 0), the others
         * when their time is up */
        if (!link_active(md, e->ce_Link) || !alive(e, now)) {
            drop(c, e);
            continue;
        }
        if (e->ce_Flags & MDNS_CACHE_LOCAL || e->ce_Refresh >= 4 ||
            e->ce_TTL < 2) {
            continue;
        }
        if (e->ce_Next == 0) {
            e->ce_Next = refresh_at(c, e) | 1;
        }
        if (due(now, e->ce_Next)) {
            e->ce_Refresh++;
            e->ce_Next = 0;
            if (watched(c, e)) {
                e->ce_Flags |= CACHE_ASKING;
                asking = TRUE;
            }
        }
    }
    if (md->md_Socket < 0) {
        for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
             e->ce_Node.mln_Succ != NULL;
             e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
            e->ce_Flags &= ~CACHE_ASKING;
        }
        return;
    }
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
        q->cq_Asking = due(now, q->cq_Next);
        asking |= q->cq_Asking;
    }
    if (!asking) {
        return;
    }
    /* what is due on each link, together */
    for (i = 0; i < md->md_IfCount; i++) {
        struct CacheAsk asks[ASK_MAX];
        UWORD count = 0;
        LONG size = 12;
        ULONG link = md->md_Ifs[i].mi_Index;

        for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
             q->cq_Node.mln_Succ != NULL;
             q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
            if (q->cq_Asking && (q->cq_Link == 0 || q->cq_Link == link)) {
                ask_add(md, i, asks, &count, &size, q->cq_Name, q->cq_Type);
            }
        }
        for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
             e->ce_Node.mln_Succ != NULL;
             e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
            if ((e->ce_Flags & CACHE_ASKING) && e->ce_Link == link) {
                ask_add(md, i, asks, &count, &size, e->ce_Name, e->ce_Type);
            }
        }
        ask_send(md, i, asks, count);
    }
    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         e->ce_Node.mln_Succ != NULL;
         e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
        e->ce_Flags &= ~CACHE_ASKING;
    }
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
        if (!q->cq_Asking) {
            continue;
        }
        q->cq_Asking = FALSE;
        q->cq_Next = now + q->cq_Interval;
        q->cq_Interval = q->cq_Interval * 2 > QUERY_MAX_INTERVAL ?
            QUERY_MAX_INTERVAL : q->cq_Interval * 2;
    }
}

/*
 * The next record after "after" (NULL: the first) of name and type on link
 * (0: any) that is still there; NULL when there is none. Its pointer holds
 * until the cache next changes.
 */
const struct MDNSCacheEntry *
mdnsCacheFind(struct MDNSDaemon *md, const struct MDNSCacheEntry *after,
    const UBYTE *name, UWORD type, ULONG link)
{
    struct MDNSCache *c = md->md_Cache;
    const struct MDNSCacheEntry *e;
    ULONG now = mdnsTicks();

    if (c == NULL) {
        return NULL;
    }
    e = after != NULL ? (const struct MDNSCacheEntry *)after->ce_Node.mln_Succ :
        (const struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
    for (; e->ce_Node.mln_Succ != NULL;
         e = (const struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
        if ((link == 0 || e->ce_Link == link) && e->ce_Type == type &&
            alive(e, now) && (name == NULL || mdnsWireNameEqual(e->ce_Name, name))) {
            return e;
        }
    }
    return NULL;
}

/* The latest of name and type on link (the last heard, the last of
 * several heard at once), or NULL. */
const struct MDNSCacheEntry *
mdnsCacheLatest(struct MDNSDaemon *md, const UBYTE *name, UWORD type,
    ULONG link)
{
    const struct MDNSCacheEntry *e = NULL, *found = NULL;

    while ((e = mdnsCacheFind(md, e, name, type, link)) != NULL) {
        if (found == NULL || (LONG)(e->ce_Received - found->ce_Received) >= 0) {
            found = e;
        }
    }
    return found;
}

/* The name a watch asks for. */
const UBYTE *
mdnsCacheWatchName(APTR watch)
{
    return ((struct CacheWatch *)watch)->cw_Question->cq_Name;
}

APTR
mdnsCacheWatch(const struct MDNSDaemon *md, const UBYTE *name, UWORD type,
    ULONG link, MDNSCacheNotify notify, APTR data)
{
    struct MDNSCache *c = md->md_Cache;
    struct CacheQuestion *q;
    struct CacheWatch *w;
    struct MDNSCacheEntry *e, *next;
    LONG namelen = mdnsWireNameLength(name);

    if (c == NULL || namelen < 0 || notify == NULL) {
        return NULL;
    }
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
        if (q->cq_Type == type && q->cq_Link == link &&
            mdnsWireNameEqual(q->cq_Name, name)) {
            break;
        }
    }
    w = AllocVec(sizeof(*w), MEMF_ANY | MEMF_CLEAR);
    if (w == NULL) {
        return NULL;
    }
    if (q->cq_Node.mln_Succ == NULL) {
        q = AllocVec(sizeof(*q), MEMF_ANY | MEMF_CLEAR);
        if (q == NULL) {
            FreeVec(w);
            return NULL;
        }
        memcpy(q->cq_Name, name, namelen);
        q->cq_Type = type;
        q->cq_Link = link;
        q->cq_Interval = TICKS;
        q->cq_Next = mdnsTicks() + random_ticks(c, 1, 6);  /* 20-120 ms */
        NEWLIST((struct List *)&q->cq_Watches);
        AddTail((struct List *)&c->mc_Questions, (struct Node *)q);
    }
    w->cw_Question = q;
    w->cw_Notify = notify;
    w->cw_Data = data;
    AddTail((struct List *)&q->cq_Watches, (struct Node *)w);
    /* what is known already, at once */
    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         (next = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) != NULL; e = next) {
        if (matches(q, e) && alive(e, mdnsTicks())) {
            notify(data, e, TRUE);
        }
    }
    return w;
}

void
mdnsCacheUnwatch(const struct MDNSDaemon *md, APTR handle)
{
    const struct MDNSCache *c = md->md_Cache;
    struct CacheWatch *w = handle;
    struct CacheQuestion *q;

    if (c == NULL || w == NULL) {
        return;
    }
    if (c->mc_Notifying) {
        w->cw_Notify = NULL;            /* freed after the telling */
        return;
    }
    q = w->cw_Question;
    Remove((struct Node *)w);
    FreeVec(w);
    if (IsListEmpty((struct List *)&q->cq_Watches)) {
        Remove((struct Node *)q);
        FreeVec(q);
    }
}

/* The links changed: what each watch wants is asked for again at once. */
void
mdnsCacheLinksChanged(const struct MDNSDaemon *md)
{
    struct MDNSCache *c = md->md_Cache;
    struct CacheQuestion *q;

    if (c == NULL) {
        return;
    }
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
        q->cq_Interval = TICKS;
        q->cq_Next = mdnsTicks() + random_ticks(c, 1, 6);
    }
}

BOOL
mdnsCacheInit(struct MDNSDaemon *md)
{
    struct MDNSCache *c = AllocVec(sizeof(*c), MEMF_ANY | MEMF_CLEAR);

    if (c == NULL) {
        return FALSE;
    }
    NEWLIST((struct List *)&c->mc_Records);
    NEWLIST((struct List *)&c->mc_Questions);
    c->mc_Seed = mdnsTicks() ^ (ULONG)(IPTR)c;
    md->md_Cache = c;
    return TRUE;
}

void
mdnsCacheFree(struct MDNSDaemon *md)
{
    struct MDNSCache *c = md->md_Cache;
    struct Node *n;

    if (c == NULL) {
        return;
    }
    while ((n = RemHead((struct List *)&c->mc_Records)) != NULL) {
        FreeVec(n);
    }
    while ((n = RemHead((struct List *)&c->mc_Questions)) != NULL) {
        struct CacheQuestion *q = (struct CacheQuestion *)n;
        struct Node *w;

        while ((w = RemHead((struct List *)&q->cq_Watches)) != NULL) {
            FreeVec(w);
        }
        FreeVec(q);
    }
    FreeVec(c);
    md->md_Cache = NULL;
}
