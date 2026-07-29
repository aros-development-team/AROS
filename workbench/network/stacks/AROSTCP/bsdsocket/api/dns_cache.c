/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * dns_cache.c -- TTL-aware cache of raw resolver response packets.
 *
 * See dns_cache.h for the public contract. The design notes worth keeping in
 * mind while reading the code below:
 *
 *   - We cache the RAW response bytes. A hit is replayed through the SAME
 *     getanswer() path a fresh response would take, so there is no risk of a
 *     second parser drifting from the first.
 *
 *   - The slot table is a fixed-size array sized once at load time (statics
 *     are zeroed, so every slot starts empty). Per-entry name and response
 *     buffers are allocated on demand with bsd_malloc() and released with
 *     bsd_free(); the allocator may return NULL and every use checks for it.
 *
 *   - The table is shared across all resolver tasks and is guarded by its own
 *     SignalSemaphore. Allocation, the TTL walk and name lowercasing are all
 *     done with the lock DROPPED; the lock is held only for the table search
 *     and the small fixed copy in/out.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <netinet/in.h>

#include <arpa/nameser.h>
#include <api/resolv.h>
#include <kern/amiga_includes.h>
#include <api/dns_cache.h>

/*
 * Number of cache slots. Small and fixed -- resolver working sets are tiny.
 */
#define DNS_CACHE_SLOTS     64

/*
 * Longest response we are willing to store. res_search()/res_query() answer
 * into a PACKETSZ-class buffer, so this comfortably covers a UDP reply.
 */
#define DNS_CACHE_MAXRESP   1024

/*
 * Upper bound on the honoured TTL. A server may advertise an absurdly long
 * TTL; we clamp it so an entry can never linger for more than a day.
 */
#define DNS_CACHE_MAXTTL    86400

/*
 * A single cache slot. An empty slot has dce_Response == NULL.
 */
struct DnsCacheEntry {
    char  *dce_Name;        /* lowercased query name, NUL terminated       */
    UBYTE *dce_Response;    /* copied raw DNS response bytes               */
    int    dce_RespLen;     /* length of dce_Response in bytes             */
    int    dce_Type;        /* query type (T_A / T_AAAA / T_PTR)           */
    ULONG  dce_Expiry;      /* absolute expiry time in seconds             */
    ULONG  dce_Stamp;       /* insertion time in seconds (oldest eviction) */
};

/*
 * The shared table and its guard. dns_cache_ready gates the one-time
 * semaphore initialisation. dns_cache_enabled is the public on/off switch.
 */
static struct DnsCacheEntry dns_cache_table[DNS_CACHE_SLOTS];
static struct SignalSemaphore dns_cache_lock;
static BOOL dns_cache_ready = FALSE;

BOOL dns_cache_enabled = TRUE;

/*
 * Current time in whole seconds, taken from the same system time source the
 * rest of the resolver uses (GetSysTime(), i.e. timer.device). Absolute
 * expiries are computed against this so a lookup only needs one comparison.
 */
static ULONG
dns_cache_now(void)
{
    struct timeval tv;

    GetSysTime(&tv);
    return (ULONG)tv.tv_secs;
}

/*
 * Copy 'src' into 'dst' folding ASCII upper case to lower case. DNS name
 * comparison is ASCII-case-insensitive (RFC 4343), so a plain byte compare of
 * two lowercased names is a correct name match. 'len' bytes are copied and
 * 'dst' is NUL terminated at dst[len].
 */
static void
dns_cache_lowercopy(char *dst, const char *src, int len)
{
    int i;

    for(i = 0; i < len; i++) {
        char c = src[i];
        if(c >= 'A' && c <= 'Z')
            c += 'a' - 'A';
        dst[i] = c;
    }
    dst[len] = '\0';
}

/*
 * Walk a DNS response and return the smallest resource-record TTL found, in
 * seconds, or 0 if the packet carries no usable TTL (empty, malformed, or
 * every record TTL was zero). OPT (EDNS0) pseudo-records are skipped because
 * their "TTL" field is not a time-to-live.
 *
 * The walk is deliberately defensive: every step is bounded by the end of the
 * message so a truncated or hostile packet cannot run us off the end.
 */
static int
dns_cache_min_ttl(const UBYTE *msg, int msglen)
{
    const u_char *cp, *eom;
    HEADER *hp;
    int qdcount, rrcount, i, n, rdlen;
    long minttl = -1;

    if(msg == NULL || msglen < (int)sizeof(HEADER))
        return 0;

    hp = (HEADER *)msg;
    eom = (const u_char *)msg + msglen;
    cp = (const u_char *)msg + sizeof(HEADER);

    qdcount = ntohs(hp->qdcount);
    /* answer + authority + additional records all carry a TTL field */
    rrcount = ntohs(hp->ancount) + ntohs(hp->nscount) + ntohs(hp->arcount);

    /*
     * Skip the question section: each question is a name followed by the
     * fixed QFIXEDSZ (qtype + qclass).
     */
    while(qdcount-- > 0) {
        n = __dn_skipname(cp, eom);
        if(n < 0)
            return 0;
        cp += n + QFIXEDSZ;
        if(cp > eom)
            return 0;
    }

    /*
     * Walk each resource record: name, then the 10-byte fixed header
     * (type, class, ttl, rdlength), then rdlength bytes of rdata.
     */
    for(i = 0; i < rrcount; i++) {
        n = __dn_skipname(cp, eom);
        if(n < 0)
            break;
        cp += n;
        if(cp + 10 > eom)
            break;
        int type = _getshort((u_char *)cp);
        cp += sizeof(u_short);              /* type  */
        cp += sizeof(u_short);              /* class */
        u_long ttl = _getlong((u_char *)cp);
        cp += sizeof(u_int32_t);            /* ttl   */
        rdlen = _getshort((u_char *)cp);
        cp += sizeof(u_short);              /* rdlength */
        if(cp + rdlen > eom)
            break;
        cp += rdlen;

        if(type == T_OPT)                   /* EDNS0 meta-RR: no real TTL */
            continue;

        if(minttl < 0 || (long)ttl < minttl)
            minttl = (long)ttl;
    }

    if(minttl < 0)
        return 0;
    if(minttl > DNS_CACHE_MAXTTL)
        minttl = DNS_CACHE_MAXTTL;

    return (int)minttl;
}

/*
 * One-time initialisation of the semaphore. The slot table itself is a zeroed
 * static, so nothing else needs setting up. Serialised with Forbid()/Permit()
 * so two tasks racing on the very first lookup cannot both InitSemaphore().
 */
void
dns_cache_init(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](dns_cache.c) dns_cache_init()\n"));
#endif

    Forbid();
    if(!dns_cache_ready) {
        InitSemaphore(&dns_cache_lock);
        dns_cache_ready = TRUE;
    }
    Permit();
}

/*
 * Ensure the cache is ready before its lock is used. Cheap once initialised.
 */
static void
dns_cache_ensure(void)
{
    if(!dns_cache_ready)
        dns_cache_init();
}

int
dns_cache_lookup(const char *name, int qtype, UBYTE *buf, int buflen)
{
    char lname[MAXDNAME + 1];
    ULONG now;
    int namelen, i, ret = 0;

    if(!dns_cache_enabled)
        return 0;
    if(name == NULL || buf == NULL || buflen <= 0)
        return 0;

    namelen = strnlen(name, MAXDNAME);
    if(namelen >= (int)sizeof(lname))       /* too long to have been cached */
        return 0;

    dns_cache_ensure();
    dns_cache_lowercopy(lname, name, namelen);
    now = dns_cache_now();

    /*
     * Table search plus the small copy-out are the only work done under the
     * lock. A shared lock is enough -- lookups never mutate the table.
     */
    ObtainSemaphoreShared(&dns_cache_lock);
    for(i = 0; i < DNS_CACHE_SLOTS; i++) {
        struct DnsCacheEntry *e = &dns_cache_table[i];

        if(e->dce_Response == NULL || e->dce_Type != qtype || e->dce_Name == NULL)
            continue;
        if(strcmp(e->dce_Name, lname) != 0)
            continue;

        /* Key match. A past-expiry entry is treated as a miss. */
        if(e->dce_Expiry > now && e->dce_RespLen <= buflen) {
            memcpy(buf, e->dce_Response, e->dce_RespLen);
            ret = e->dce_RespLen;
        }
        break;
    }
    ReleaseSemaphore(&dns_cache_lock);

#if defined(__AROS__)
    D(bug("[AROSTCP](dns_cache.c) dns_cache_lookup('%s', %d) -> %d\n",
          name, qtype, ret));
#endif

    return ret;
}

void
dns_cache_insert(const char *name, int qtype, const UBYTE *resp, int resplen)
{
    struct DnsCacheEntry *e;
    char *newname, *oldname = NULL;
    UBYTE *newresp, *oldresp = NULL;
    ULONG now, expiry;
    int namelen, ttl, i;
    int match = -1, empty = -1, expired = -1, oldest = -1;

    if(!dns_cache_enabled)
        return;
    if(name == NULL || resp == NULL || resplen <= 0 || resplen > DNS_CACHE_MAXRESP)
        return;

    /*
     * Derive the entry lifetime from the packet. A response with no usable
     * (non-zero) TTL is not worth caching.
     */
    ttl = dns_cache_min_ttl(resp, resplen);
    if(ttl <= 0)
        return;

    namelen = strnlen(name, MAXDNAME);

    /*
     * Allocate and fill the copies with the lock DROPPED. bsd_malloc() can
     * return NULL, so each allocation is checked and any partial work undone.
     */
    newname = bsd_malloc(namelen + 1, M_TEMP, M_WAITOK);
    if(newname == NULL)
        return;
    newresp = bsd_malloc(resplen, M_TEMP, M_WAITOK);
    if(newresp == NULL) {
        bsd_free(newname, M_TEMP);
        return;
    }
    dns_cache_lowercopy(newname, name, namelen);
    memcpy(newresp, resp, resplen);

    dns_cache_ensure();
    now = dns_cache_now();
    expiry = now + (ULONG)ttl;

    /*
     * Pick a slot under the lock: reuse an entry with the same key, else an
     * empty slot, else an expired one, else evict the oldest. The evicted
     * buffers are detached here but freed after the lock is released.
     */
    ObtainSemaphore(&dns_cache_lock);
    for(i = 0; i < DNS_CACHE_SLOTS; i++) {
        e = &dns_cache_table[i];

        if(e->dce_Response == NULL) {
            if(empty < 0)
                empty = i;
            continue;
        }
        if(e->dce_Type == qtype && e->dce_Name != NULL &&
                strcmp(e->dce_Name, newname) == 0) {
            match = i;
            break;
        }
        if(expired < 0 && e->dce_Expiry <= now)
            expired = i;
        if(oldest < 0 || e->dce_Stamp < dns_cache_table[oldest].dce_Stamp)
            oldest = i;
    }

    if(match >= 0)
        i = match;
    else if(empty >= 0)
        i = empty;
    else if(expired >= 0)
        i = expired;
    else
        i = oldest;             /* table full: reuse the oldest entry */

    e = &dns_cache_table[i];
    oldname = e->dce_Name;
    oldresp = e->dce_Response;

    e->dce_Name = newname;
    e->dce_Response = newresp;
    e->dce_RespLen = resplen;
    e->dce_Type = qtype;
    e->dce_Expiry = expiry;
    e->dce_Stamp = now;
    ReleaseSemaphore(&dns_cache_lock);

    /* Release the evicted buffers with the lock dropped. */
    if(oldname != NULL)
        bsd_free(oldname, M_TEMP);
    if(oldresp != NULL)
        bsd_free(oldresp, M_TEMP);

#if defined(__AROS__)
    D(bug("[AROSTCP](dns_cache.c) dns_cache_insert('%s', %d) len=%d ttl=%d slot=%d\n",
          name, qtype, resplen, ttl, i));
#endif
}
