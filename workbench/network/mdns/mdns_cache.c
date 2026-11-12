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
 * and stay until their goodbye: nobody refreshes them.
 *
 * Memory: each record is allocated by itself, within MDNS_CACHE_BYTES; when
 * that is full, the record nearest its end goes first.
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
    ULONG           cq_Link;            /* 0: every link */
    ULONG           cq_Next;            /* mdnsTicks() of the next query */
    ULONG           cq_Interval;
    struct MinList  cq_Watches;
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

LONG
mdnsWireNameLength(const UBYTE *name)
{
    LONG at = 0;

    while (at < 255 && name[at] != 0) {
        at += name[at] + 1;
    }
    return at < 255 ? at + 1 : -1;
}

static UBYTE
lower(UBYTE c)
{
    return c >= 'A' && c <= 'Z' ? (UBYTE)(c + 32) : c;
}

BOOL
mdnsWireNameEqual(const UBYTE *a, const UBYTE *b)
{
    LONG la = mdnsWireNameLength(a), lb = mdnsWireNameLength(b), i;

    if (la < 0 || la != lb) {
        return FALSE;
    }
    for (i = 0; i < la; i++) {
        if (lower(a[i]) != lower(b[i])) {
            return FALSE;
        }
    }
    return TRUE;
}

/* A name at offset of pkt, compressed or not, into name as wire labels;
 * the bytes it takes at offset, or -1. */
LONG
mdnsWireNameRead(const UBYTE *pkt, LONG length, LONG offset, UBYTE *name)
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
            name[at++] = n;
            if (n == 0) {
                return consumed;
            }
            memcpy(name + at, pkt + offset, n);
            at += n;
            offset += n;
            if (!jumped) {
                consumed += n;
            }
        }
    }
    return -1;
}

/* A text name ("host.local", dots separate labels, no escapes) as wire
 * labels; FALSE if it does not make a name. */
BOOL
mdnsWireNameFromText(const char *text, UBYTE *name)
{
    LONG at = 0, start;

    while (*text != '\0') {
        start = at++;
        while (*text != '\0' && *text != '.') {
            if (at >= 254 || at - start > 63) {
                return FALSE;
            }
            name[at++] = (UBYTE)*text++;
        }
        if (at - start - 1 == 0) {
            return FALSE;
        }
        name[start] = (UBYTE)(at - start - 1);
        if (*text == '.') {
            text++;
        }
    }
    name[at] = 0;
    return at > 0;
}

/*
 * The RDATA of a record at rdoffset (rdlength bytes) with the names in it
 * expanded, into data (at most 512 bytes); its length, or -1.
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
    case DNS_TYPE_PTR:
    case 5:     /* CNAME */
    case 2:     /* NS */
        if (mdnsWireNameRead(pkt, length, rdoffset, data) < 0) {
            return -1;
        }
        return mdnsWireNameLength(data);
    case DNS_TYPE_SRV:
        if (rdlength < 7) {
            return -1;
        }
        memcpy(data, pkt + rdoffset, 6);
        if (mdnsWireNameRead(pkt, length, rdoffset + 6, data + 6) < 0) {
            return -1;
        }
        n = mdnsWireNameLength(data + 6);
        return n < 0 ? -1 : 6 + n;
    default:
        if (rdlength > 512) {
            return -1;
        }
        memcpy(data, pkt + rdoffset, rdlength);
        return rdlength;
    }
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

/* Room for bytes more: the records nearest their end go. */
static void
make_room(struct MDNSCache *c, ULONG bytes)
{
    while (c->mc_Bytes + bytes > MDNS_CACHE_BYTES) {
        struct MDNSCacheEntry *e, *oldest = NULL;

        for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
             e->ce_Node.mln_Succ != NULL;
             e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
            if (!(e->ce_Flags & MDNS_CACHE_LOCAL) &&
                (oldest == NULL || (LONG)(e->ce_Expires - oldest->ce_Expires) < 0)) {
                oldest = e;
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
    UBYTE name[256], data[512];
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

/* The question on link (a slot) with the answers known there that have
 * more than half their life left (RFC 6762 section 7.1). */
static void
ask(struct MDNSDaemon *md, const UBYTE *name, UWORD type, LONG slot)
{
    const struct MDNSCache *c = md->md_Cache;
    UBYTE *pkt = md->md_TxBuf;
    struct MDNSCacheEntry *e;
    ULONG now = mdnsTicks();
    LONG at, namelen = mdnsWireNameLength(name);
    UWORD answers = 0;
    UWORD savedFamily = md->md_SendFamily;

    if (pkt == NULL || namelen < 0) {
        return;
    }
    memset(pkt, 0, 12);
    pkt[5] = 1;                         /* one question */
    memcpy(pkt + 12, name, namelen);
    at = 12 + namelen;
    pkt[at++] = (UBYTE)(type >> 8);
    pkt[at++] = (UBYTE)type;
    pkt[at++] = 0;
    pkt[at++] = DNS_CLASS_IN;
    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         e->ce_Node.mln_Succ != NULL;
         e = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) {
        LONG left, rdlen;

        if (e->ce_Link != md->md_Ifs[slot].mi_Index ||
            (type != DNS_TYPE_ANY && e->ce_Type != type) ||
            !mdnsWireNameEqual(e->ce_Name, name) ||
            (e->ce_Flags & MDNS_CACHE_LOCAL)) {
            continue;
        }
        left = (LONG)(e->ce_Expires - now) / TICKS;
        if (left <= 0 || (ULONG)left * 2 < e->ce_TTL) {
            continue;
        }
        rdlen = e->ce_DataLength;
        if (at + 2 + 10 + rdlen > MDNS_MAX_PACKET - 64 || at + 12 > 1400) {
            break;          /* the rest would make it more than one packet */
        }
        pkt[at++] = 0xc0;               /* the question's name */
        pkt[at++] = 12;
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
    pkt[6] = (UBYTE)(answers >> 8);
    pkt[7] = (UBYTE)answers;
    md->md_SendFamily = 0;              /* both families of the link */
    mdnsNetSendOn(md, slot, pkt, at);
    md->md_SendFamily = savedFamily;
}

static void
ask_question(struct MDNSDaemon *md, const struct CacheQuestion *q)
{
    LONG i;

    for (i = 0; i < md->md_IfCount; i++) {
        if (q->cq_Link == 0 || q->cq_Link == md->md_Ifs[i].mi_Index) {
            ask(md, q->cq_Name, q->cq_Type, i);
        }
    }
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

    if (c == NULL) {
        return;
    }
    for (e = (struct MDNSCacheEntry *)c->mc_Records.mlh_Head;
         (next = (struct MDNSCacheEntry *)e->ce_Node.mln_Succ) != NULL; e = next) {
        if (!link_active(md, e->ce_Link) ||
            (!(e->ce_Flags & MDNS_CACHE_LOCAL) && due(now, e->ce_Expires))) {
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
                LONG i;

                for (i = 0; i < md->md_IfCount; i++) {
                    if (md->md_Ifs[i].mi_Index == e->ce_Link) {
                        ask(md, e->ce_Name, e->ce_Type, i);
                    }
                }
            }
        }
    }
    if (md->md_Socket < 0) {
        return;
    }
    for (q = (struct CacheQuestion *)c->mc_Questions.mlh_Head;
         q->cq_Node.mln_Succ != NULL;
         q = (struct CacheQuestion *)q->cq_Node.mln_Succ) {
        if (!due(now, q->cq_Next)) {
            continue;
        }
        ask_question(md, q);
        q->cq_Next = now + q->cq_Interval;
        q->cq_Interval = q->cq_Interval * 2 > QUERY_MAX_INTERVAL ?
            QUERY_MAX_INTERVAL : q->cq_Interval * 2;
    }
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
        if (matches(q, e)) {
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
