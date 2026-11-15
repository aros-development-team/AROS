/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME Neighbor Discovery implementation.
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * nd6.c - IPv6 Neighbor Discovery core (RFC 4861).
 *
 * Implements:
 *   - Neighbor cache management (lookup, resolve, cache_lladdr)
 *   - NDP option parsing (nd6_option_init, nd6_options)
 *   - Neighbor Unreachability Detection (NUD) timer
 *   - Per-interface ND state (nd6_ifattach, nd6_ifinfo)
 *   - Routing table hooks (nd6_rtrequest)
 *   - Interface helpers (nd6_ifptomac, nd6_setmtu, nd6_ioctl)
 *
 * NS/NA handling is in nd6_nbr.c; RS/RA/redirect in nd6_rtr.c.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>
#include <stdio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <kern/amiga_netdb.h>
#include <kern/amiga_includes.h>        /* AvailMem() */
#include <sys/sysctl.h>                 /* sysctl_int() */
#include <net/netmodule.h>          /* api_addrchange_retry() */
#include <protos/net/route_protos.h>
#include <net/netcore_map.h>        /* always the last include */

static void nd6_solicit(struct llinfo_nd6 *, int);

extern void icmp6_error(struct mbuf *, int, int, int);

/* ------------------------------------------------------------------
 * The packets queued for a neighbour while its address is resolved
 * (RFC 4861 section 7.2.2): up to nd6_maxqueuelen, oldest first, chained
 * through m_nextpkt. One was not enough: the fragments of a datagram are
 * queued one after the other, and only the last one would have been sent.
 * All entries together hold at most nd6_maxheld (nd6_held now): past it,
 * the oldest packet held anywhere gives way - the first of the entry that
 * has waited longest, the one nearest the end of the list (new entries go
 * to its head, and one is only taken over when it holds nothing).
 * ------------------------------------------------------------------ */
static int nd6_held;            /* packets held by all entries */

static void
nd6_hold_drop_first(struct llinfo_nd6 *ln)
{
    struct mbuf *old = ln->ln_hold;

    ln->ln_hold = old->m_nextpkt;
    old->m_nextpkt = NULL;
    m_freem(old);
    ln->ln_holdlen--;
    nd6_held--;
    nd6stat.nd6s_res_holdq_full++;
}

void
nd6_hold_add(struct llinfo_nd6 *ln, struct mbuf *m)
{
    struct mbuf **mp;

    if(ln->ln_holdlen >= nd6_maxqueuelen && ln->ln_hold != NULL)
        nd6_hold_drop_first(ln);            /* full: its oldest gives way */
    if(nd6_held >= nd6_maxheld) {
        struct llinfo_nd6 *o;

        for(o = llinfo_nd6.ln_prev; o != &llinfo_nd6; o = o->ln_prev)
            if(o->ln_hold != NULL) {
                nd6_hold_drop_first(o);     /* the stack's oldest */
                break;
            }
    }
    m->m_nextpkt = NULL;
    for(mp = &ln->ln_hold; *mp != NULL; mp = &(*mp)->m_nextpkt)
        ;
    *mp = m;
    ln->ln_holdlen++;
    nd6_held++;
}

/*
 * Drop the queued packets. With unreachable set, address resolution has
 * failed: each gets an ICMPv6 Destination Unreachable, address unreachable
 * (section 7.2.2 says MUST), which comes back to the socket that sent it.
 */
void
nd6_hold_free(struct llinfo_nd6 *ln, int unreachable)
{
    struct mbuf *m, *next;
    struct ifnet *ifp = ln->ln_ifp;

    for(m = ln->ln_hold; m != NULL; m = next) {
        next = m->m_nextpkt;
        m->m_nextpkt = NULL;
        if(unreachable && ifp != NULL) {
            /* our own packet: the error comes from our address on ifp */
            m->m_pkthdr.rcvif = ifp;
            icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_ADDR, 0);
        } else
            m_freem(m);
    }
    nd6_held -= ln->ln_holdlen;
    ln->ln_hold = NULL;
    ln->ln_holdlen = 0;
}

/* The address is resolved: send the queued packets, in order. */
void
nd6_hold_send(struct llinfo_nd6 *ln, struct ifnet *ifp, struct sockaddr *dst)
{
    struct mbuf *m = ln->ln_hold, *next;

    nd6_held -= ln->ln_holdlen;
    ln->ln_hold = NULL;
    ln->ln_holdlen = 0;
    for(; m != NULL; m = next) {
        next = m->m_nextpkt;
        m->m_nextpkt = NULL;
        (*ifp->if_output)(ifp, m, dst, NULL);
    }
}

/* ------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------ */
struct llinfo_nd6 llinfo_nd6;		/* sentinel for neighbor cache */
struct nd6stat nd6stat = {0};

int nd6_maxndopt   = 10;		/* max options in a single ND msg */
int nd6_maxqueuelen = ND6_MAXQUEUELEN;	/* packets held per entry */
int nd6_inuse      = 0;		/* neighbor cache entries in use */
int nd6_allocated  = 0;		/* total neighbor cache allocations */
int nd6_maxnbrs    = ND6_MAXNBRS;	/* entries per interface: nd6_init() sizes it */
int nd6_maxheld    = ND6_MAXNBRS;	/* packets held by all entries: likewise */

/*
 * The netvars (ip6_input.c's table): net.inet6.icmp6.nd6_maxqlen, as
 * BSD's, and AROS's nd6_maxnbrs and nd6_maxheld; each checked when set.
 */
static int
nd6_sysctl_range(void *oldp, size_t *oldlenp, void *newp, size_t newlen,
    int *var, int min, int max)
{
    int v = *var, error;

    error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
    if(error == 0 && newp && (v < min || v > max))
        error = EINVAL;
    if(error == 0)
        *var = v;
    return (error);
}

int
nd6_sysctl_maxqueuelen(const struct netvar *nv, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen)
{
    return nd6_sysctl_range(oldp, oldlenp, newp, newlen, &nd6_maxqueuelen, 1, 64);
}

int
nd6_sysctl_maxnbrs(const struct netvar *nv, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen)
{
    return nd6_sysctl_range(oldp, oldlenp, newp, newlen, &nd6_maxnbrs, ND6_MINNBRS, 4096);
}

int
nd6_sysctl_maxheld(const struct netvar *nv, void *oldp, size_t *oldlenp,
    void *newp, size_t newlen)
{
    return nd6_sysctl_range(oldp, oldlenp, newp, newlen, &nd6_maxheld, 1, 4096);
}

#define ND6_NBRHASH 64			/* a power of two */
static struct llinfo_nd6 *nd6_nbrhash[ND6_NBRHASH];	/* (ifp, address), chained */
static int nd6_nbrs_noinfo;		/* entries on interfaces past ND6_MAXIFS */

/* per-interface ND info array, indexed by ifp->if_index */
#define ND6_MAXIFS 16
static struct nd_ifinfo nd6_ndi[ND6_MAXIFS];

/* ------------------------------------------------------------------
 * nd6_init - initialise ND6 subsystem.
 * ------------------------------------------------------------------ */
void
nd6_init(void)
{
    int i;

    /* initialise neighbor cache sentinel */
    llinfo_nd6.ln_next = &llinfo_nd6;
    llinfo_nd6.ln_prev = &llinfo_nd6;
    llinfo_nd6.ln_state = ND6_LLINFO_NOSTATE;
    bzero(nd6_nbrhash, sizeof(nd6_nbrhash));

    /* the cache's limits, from the memory there is: one entry per
     * ND6_MEM_PER_NBR bytes free, ND6_MINNBRS to ND6_MAXNBRS per interface,
     * and as many packets held by all of them */
    {
        ULONG n = AvailMem(MEMF_ANY) / ND6_MEM_PER_NBR;

        nd6_maxnbrs = n < ND6_MINNBRS ? ND6_MINNBRS : n > ND6_MAXNBRS ? ND6_MAXNBRS : (int)n;
        nd6_maxheld = nd6_maxnbrs;
        nd6_held = 0;
    }

    /* initialise per-interface ND info */
    for(i = 0; i < ND6_MAXIFS; i++) {
        nd6_ndi[i].basereachable = ND6_REACHABLE_TIME;
        nd6_ndi[i].reachable    = ND6_REACHABLE_TIME;
        nd6_ndi[i].retrans      = ND6_RETRANS_TIMER;
        nd6_ndi[i].chlim        = 64;
        nd6_ndi[i].flags        = ND6_IFF_PERFORMNUD |
                                  ND6_IFF_ACCEPT_RTADV |
                                  ND6_IFF_AUTO_LINKLOCAL;
        nd6_ndi[i].initialized  = 0;
    }
}

/* ------------------------------------------------------------------
 * nd6_deinit - IPv6 stops: the reverse of nd6_init() and of what
 * neighbour discovery made since.
 *
 * The neighbour cache is this module's own memory, not the routing
 * table's: every entry is freed, with the packets that waited for its
 * address (nd6_nbr_free()), and nothing of the core leads to it. Then the
 * per-interface information, the counters and the file's other variables
 * are what the file gives them.
 * ------------------------------------------------------------------ */
static int nd6_timer_ticks = 0;     /* nd6_timer()'s, for its trace */

void
nd6_deinit(void)
{
    struct llinfo_nd6 *ln;
    spl_t s = splnet();

    while(llinfo_nd6.ln_next != NULL && (ln = llinfo_nd6.ln_next) != &llinfo_nd6)
        nd6_nbr_free(ln);
    bzero((caddr_t)&llinfo_nd6, sizeof(llinfo_nd6));
    bzero((caddr_t)nd6_ndi, sizeof(nd6_ndi));
    bzero((caddr_t)&nd6stat, sizeof(nd6stat));
    nd6_maxndopt = 10;
    nd6_maxqueuelen = ND6_MAXQUEUELEN;
    nd6_held = 0;
    nd6_inuse = 0;
    nd6_allocated = 0;
    nd6_timer_ticks = 0;
    splx(s);
}

/* ------------------------------------------------------------------
 * nd6_ifattach - initialise ND state for a network interface.
 * ------------------------------------------------------------------ */
void
nd6_ifattach(struct ifnet *ifp)
{
    struct nd_ifinfo *ndi;

    D(bug("[AROSTCP:ND6] %s: ifp=%s%d index=%d\n",
          __func__, ifp->if_name, ifp->if_unit, ifp->if_index));

    if(ifp->if_index >= ND6_MAXIFS) {
        D(bug("[AROSTCP:ND6] %s: index %d >= ND6_MAXIFS(%d), skip\n",
              __func__, ifp->if_index, ND6_MAXIFS));
        return;
    }

    ndi = &nd6_ndi[ifp->if_index];
    if(ndi->initialized) {
        D(bug("[AROSTCP:ND6] %s: already initialized for index %d\n",
              __func__, ifp->if_index));
        return;
    }

    ndi->basereachable = ND6_REACHABLE_TIME;
    ndi->reachable     = ND6_REACHABLE_TIME;
    ndi->retrans       = ND6_RETRANS_TIMER;
    ndi->chlim         = 64;
    ndi->linkmtu       = 0;		/* use ifp->if_mtu */
    ndi->maxmtu        = ifp->if_mtu;
    ndi->flags         = ND6_IFF_PERFORMNUD | ND6_IFF_ACCEPT_RTADV |
                         ND6_IFF_AUTO_LINKLOCAL;
    ndi->rtr_soliciting  = 0;
    ndi->rtr_solicit_timer = 0;
    ndi->recalctm        = 0;
    ndi->initialized     = 1;

    D(bug("[AROSTCP:ND6] %s: initialized for %s%d (reachable=%d retrans=%d)\n",
          __func__, ifp->if_name, ifp->if_unit,
          ndi->reachable, ndi->retrans));
}

/* ------------------------------------------------------------------
 * nd6_ifdetach - release the per-interface ND state when an interface is
 * removed: its neighbour cache entries (each holds the ifp), the routers
 * and prefixes heard on it, and its nd6_ndi slot, which holds only scalar
 * config; the if_index itself is never reused.
 * ------------------------------------------------------------------ */
void
nd6_ifdetach(struct ifnet *ifp)
{
    nd6_purge_routers(ifp);     /* the routers heard on it and their routes */
    nd6_purge_prefixes(ifp);    /* the prefixes heard on it */
    nd6_purge_neighbors(ifp, 0);    /* last: choosing another router probes */
    if(ifp->if_index < ND6_MAXIFS)
        bzero((caddr_t)&nd6_ndi[ifp->if_index], sizeof(nd6_ndi[ifp->if_index]));
}

/* ------------------------------------------------------------------
 * nd6_ifinfo - return per-interface ND info for ifp.
 * ------------------------------------------------------------------ */
struct nd_ifinfo *
nd6_ifinfo(struct ifnet *ifp)
{
    if(ifp->if_index >= ND6_MAXIFS)
        return NULL;
    return &nd6_ndi[ifp->if_index];
}

/* ------------------------------------------------------------------
 * nd6_ifptomac - return pointer to interface MAC address.
 * ------------------------------------------------------------------ */
caddr_t
nd6_ifptomac(struct ifnet *ifp)
{
    struct sockaddr_dl *sdl;
    struct ifaddr *ifa;

    for(ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr->sa_family == AF_LINK) {
            sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            return LLADDR(sdl);
        }
    }
    return NULL;
}

/* ==================================================================
 * NDP Option Parsing (RFC 4861 §4.6)
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_option_init - prepare to iterate NDP options.
 *
 * opt     - pointer to first option (after ND message header)
 * optlen  - total bytes of options
 * ndopts  - output structure to fill
 * ------------------------------------------------------------------ */
void
nd6_option_init(void *opt, int optlen, union nd_opts *ndopts)
{
    bzero(ndopts, sizeof(*ndopts));
    if(optlen <= 0)
        return;
    ndopts->nd_opts_search = (struct nd_opt_hdr *)opt;
    ndopts->nd_opts_last   = (struct nd_opt_hdr *)((u_int8_t *)opt + optlen);
    ndopts->nd_opts_done   = 0;
}

/* ------------------------------------------------------------------
 * nd6_options - parse all NDP options into the ndopts structure.
 *
 * Returns 0 on success, -1 on malformed options.
 * ------------------------------------------------------------------ */
int
nd6_options(union nd_opts *ndopts)
{
    struct nd_opt_hdr *nd_opt;
    u_int8_t *cp, *end;
    int count = 0;

    if(ndopts->nd_opts_search == NULL)
        return 0;

    cp  = (u_int8_t *)ndopts->nd_opts_search;
    end = (u_int8_t *)ndopts->nd_opts_last;

    while(cp + 2 <= end) {
        nd_opt = (struct nd_opt_hdr *)cp;
        if(nd_opt->nd_opt_len == 0)
            return -1;	/* bad: length 0 */
        if(cp + nd_opt->nd_opt_len * 8 > end)
            return -1;	/* bad: overflows buffer */
        if(++count > nd6_maxndopt)
            return -1;	/* too many options */

        switch(nd_opt->nd_opt_type) {
        case ND_OPT_SOURCE_LINKADDR:
            ndopts->nd_opts_src_lladdr = nd_opt;
            break;
        case ND_OPT_TARGET_LINKADDR:
            ndopts->nd_opts_tgt_lladdr = nd_opt;
            break;
        case ND_OPT_PREFIX_INFORMATION:
            if(ndopts->nd_opts_pi == NULL)
                ndopts->nd_opts_pi =
                    (struct nd_opt_prefix_info *)nd_opt;
            ndopts->nd_opts_pi_end =
                (struct nd_opt_prefix_info *)nd_opt;
            break;
        case ND_OPT_REDIRECTED_HEADER:
            ndopts->nd_opts_rh =
                (struct nd_opt_rd_hdr *)nd_opt;
            break;
        case ND_OPT_MTU:
            ndopts->nd_opts_mtu =
                (struct nd_opt_mtu *)nd_opt;
            break;
        default:
            /* ignore unknown options (RFC 4861 §4.6) */
            break;
        }

        cp += nd_opt->nd_opt_len * 8;
    }

    ndopts->nd_opts_done = 1;
    return 0;
}

/* ==================================================================
 * Neighbor Cache (RFC 4861 section 5.1)
 *
 * One entry per neighbour, and a neighbour is an IPv6 address on a link:
 * the key is (interface, address), so fe80::1 on one link and fe80::1 on
 * another are two entries, each with its own link-layer address and
 * state. The entries are kept apart from the routing table, as FreeBSD's
 * lltable has kept them since 8.0: the route gives the next hop and the
 * interface, and nd6_resolve() looks the next hop up here, on that
 * interface. No address is stored with its zone embedded (Readme 6.6).
 *
 * Everything here runs at an spl (output at splnet or splimp, input and
 * the timers in the daemon at splsoftclock: one mutex), and nothing waits
 * inside, so there is no reference count: a pointer to an entry is used
 * only within the call that found it.
 * ================================================================== */

static long
nd6_secs(void)
{
    struct timeval tv;

    GetUpTime(&tv);
    return tv.tv_sec;
}

static int
nd6_nbrhashof(const struct ifnet *ifp, const struct in6_addr *addr)
{
    u_int32_t h = ifp->if_index;
    int i;

    for(i = 0; i < 16; i++)
        h = h * 31 + addr->s6_addr[i];
    return (int)(h & (ND6_NBRHASH - 1));
}

/* The count nd6_maxnbrs bounds for ifp's entries: its own, or for an
 * interface with no nd6_ndi slot (an index past ND6_MAXIFS) one they share. */
static int *
nd6_nbrcount(struct ifnet *ifp)
{
    struct nd_ifinfo *ndi = nd6_ifinfo(ifp);

    return ndi != NULL ? &ndi->nbrs : &nd6_nbrs_noinfo;
}

static void
nd6_nbr_unhash(struct llinfo_nd6 *ln)
{
    struct llinfo_nd6 **lp = &nd6_nbrhash[nd6_nbrhashof(ln->ln_ifp, &ln->ln_addr)];

    for(; *lp != NULL; lp = &(*lp)->ln_hnext)
        if(*lp == ln) {
            *lp = ln->ln_hnext;
            break;
        }
    ln->ln_hnext = NULL;
}

/* An entry that has gone STALE is dropped if it is not used for a day. */
void
nd6_nbr_stale(struct llinfo_nd6 *ln)
{
    ln->ln_state  = ND6_LLINFO_STALE;
    ln->ln_expire = nd6_secs() + ND6_GCTIME;
}

/*
 * One of ifp's own addresses is a neighbour reachable for good, at the
 * interface's own link-layer address, as KAME's nd6_rtrequest() made it
 * and FreeBSD's lltable keeps it (LLE_IFADDR): a router that is one of
 * our addresses is reachable. Neighbour Discovery leaves it as it is, and
 * it goes with the address (nd6_ifaddr_gone()).
 */
static void
nd6_nbr_ifaddr(struct llinfo_nd6 *ln)
{
    struct ifaddr *ifa;
    caddr_t mac;

    for(ifa = ln->ln_ifp->if_addrlist; ifa != NULL; ifa = ifa->ifa_next)
        if(ifa->ifa_addr->sa_family == AF_INET6 &&
                bcmp(&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr,
                     &ln->ln_addr, sizeof(ln->ln_addr)) == 0)
            break;
    if(ifa == NULL)
        return;
    ln->ln_flags |= ND6_LNF_IFADDR;
    ln->ln_state  = ND6_LLINFO_REACHABLE;
    ln->ln_expire = 0;
    if((mac = nd6_ifptomac(ln->ln_ifp)) != NULL &&
            ln->ln_ifp->if_addrlen <= ND6_LLADDRLEN) {
        bcopy(mac, ln->ln_lladdr, ln->ln_ifp->if_addrlen);
        ln->ln_alen = ln->ln_ifp->if_addrlen;
    }
}

/* An address of ifp is deleted: so is its entry. */
void
nd6_ifaddr_gone(struct ifnet *ifp, const struct in6_addr *addr)
{
    struct llinfo_nd6 *ln = nd6_nbr_lookup(ifp, addr, 0);

    if(ln != NULL && (ln->ln_flags & ND6_LNF_IFADDR))
        nd6_nbr_free(ln);
}

/*
 * The entry a full cache gives up for a new neighbour on ifp: one without
 * packets waiting, and set by hand never - one that has not been resolved
 * (NOSTATE) first, else the STALE one unused the longest. Never one with
 * packets waiting for it to be resolved (INCOMPLETE).
 */
static struct llinfo_nd6 *
nd6_nbr_victim(const struct ifnet *ifp)
{
    struct llinfo_nd6 *ln, *best = NULL;

    for(ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = ln->ln_next) {
        if(ln->ln_ifp != ifp || ln->ln_hold != NULL ||
                (ln->ln_flags & ND6_LNF_KEEP))
            continue;
        if(ln->ln_state == ND6_LLINFO_NOSTATE)
            return ln;
        if(ln->ln_state == ND6_LLINFO_STALE &&
                (best == NULL || ln->ln_expire < best->ln_expire))
            best = ln;
    }
    return best;
}

/*
 * A new entry for addr on ifp, NOSTATE. When the interface has nd6_maxnbrs
 * already, an idle one is taken over (nd6_nbr_victim()), left where it is
 * in the list, so that nd6_timer() walking it is not disturbed; with none,
 * there is no entry: the caller fails with ENOBUFS.
 */
static struct llinfo_nd6 *
nd6_nbr_new(struct ifnet *ifp, const struct in6_addr *addr, int h)
{
    int *count = nd6_nbrcount(ifp);
    struct llinfo_nd6 *ln, *next, *prev;

    if(*count >= nd6_maxnbrs) {
        if((ln = nd6_nbr_victim(ifp)) == NULL) {
            nd6stat.nd6s_nbr_full++;
            return NULL;
        }
        nd6stat.nd6s_nbr_reused++;
        nd6_nbr_unhash(ln);
        next = ln->ln_next;
        prev = ln->ln_prev;
        bzero(ln, sizeof(*ln));
        ln->ln_next = next;
        ln->ln_prev = prev;
    } else {
        MALLOC(ln, struct llinfo_nd6 *, sizeof(*ln), M_PCB, M_NOWAIT);
        if(ln == NULL) {
            nd6stat.nd6s_nbr_nomem++;
            return NULL;
        }
        bzero(ln, sizeof(*ln));
        /* at the head: nd6_timer() walking the list meets it next time */
        ln->ln_next = llinfo_nd6.ln_next;
        ln->ln_prev = &llinfo_nd6;
        llinfo_nd6.ln_next->ln_prev = ln;
        llinfo_nd6.ln_next = ln;
        (*count)++;
        nd6_inuse++;
        nd6_allocated++;
    }
    ln->ln_ifp    = ifp;
    ln->ln_addr   = *addr;
    ln->ln_state  = ND6_LLINFO_NOSTATE;
    ln->ln_expire = nd6_secs() + ND6_GCTIME;
    ln->ln_hnext  = nd6_nbrhash[h];
    nd6_nbrhash[h] = ln;
    nd6_nbr_ifaddr(ln);
    return ln;
}

/* ------------------------------------------------------------------
 * nd6_nbr_lookup - the neighbour cache entry of addr on ifp; with create,
 * a new one (NOSTATE) if there is none. NULL if there is none, or none
 * could be made (the cache full, or no memory).
 * ------------------------------------------------------------------ */
struct llinfo_nd6 *
nd6_nbr_lookup(struct ifnet *ifp, const struct in6_addr *addr, int create)
{
    struct llinfo_nd6 *ln;
    int h;

    if(ifp == NULL)
        return NULL;
    h = nd6_nbrhashof(ifp, addr);
    for(ln = nd6_nbrhash[h]; ln != NULL; ln = ln->ln_hnext)
        if(ln->ln_ifp == ifp &&
                bcmp(&ln->ln_addr, addr, sizeof(*addr)) == 0)
            return ln;
    return create ? nd6_nbr_new(ifp, addr, h) : NULL;
}

/* ------------------------------------------------------------------
 * nd6_nbr_free - delete a neighbour cache entry; the packets waiting for
 * it are dropped.
 * ------------------------------------------------------------------ */
void
nd6_nbr_free(struct llinfo_nd6 *ln)
{
    int *count = nd6_nbrcount(ln->ln_ifp);

    nd6_hold_free(ln, 0);
    nd6_nbr_unhash(ln);
    ln->ln_prev->ln_next = ln->ln_next;
    ln->ln_next->ln_prev = ln->ln_prev;
    if(*count > 0)
        (*count)--;
    nd6_inuse--;
    FREE(ln, M_PCB);
}

/* ------------------------------------------------------------------
 * nd6_purge_neighbors - the interface is going away (all its entries
 * go), or its link went down (a new network when it comes back: what was
 * learned goes; with keep, the entries set by hand and our own addresses'
 * stay, as they do through anything else but their removal - ND6_LNF_KEEP).
 * ------------------------------------------------------------------ */
void
nd6_purge_neighbors(const struct ifnet *ifp, int keep)
{
    struct llinfo_nd6 *ln, *nln;

    for(ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = nln) {
        nln = ln->ln_next;
        if(ln->ln_ifp == ifp && !(keep && (ln->ln_flags & ND6_LNF_KEEP)))
            nd6_nbr_free(ln);
    }
}

/* ------------------------------------------------------------------
 * nd6_nbr_set - a neighbour's link-layer address set by hand (the routing
 * socket's RTM_ADD with RTF_LLINFO, as ndp -s): a static entry, REACHABLE
 * for good, that Neighbor Discovery leaves as it is.
 * ------------------------------------------------------------------ */
int
nd6_nbr_set(struct ifnet *ifp, const struct in6_addr *addr,
            const u_char *lladdr, int lladdrlen)
{
    struct llinfo_nd6 *ln;
    struct sockaddr_in6 dst;

    if(lladdrlen <= 0 || lladdrlen > ND6_LLADDRLEN ||
            lladdrlen != ifp->if_addrlen ||
            IN6_IS_ADDR_MULTICAST(addr) || IN6_IS_ADDR_UNSPECIFIED(addr))
        return EINVAL;
    if((ln = nd6_nbr_lookup(ifp, addr, 1)) == NULL)
        return ENOBUFS;
    if(ln->ln_flags & ND6_LNF_IFADDR)
        return EEXIST;      /* one of our own addresses */
    bcopy(lladdr, ln->ln_lladdr, lladdrlen);
    ln->ln_alen   = lladdrlen;
    ln->ln_state  = ND6_LLINFO_REACHABLE;
    ln->ln_flags |= ND6_LNF_STATIC;
    ln->ln_expire = 0;
    if(ln->ln_hold != NULL) {
        bzero(&dst, sizeof(dst));
        dst.sin6_family = AF_INET6;
        dst.sin6_len    = sizeof(dst);
        dst.sin6_addr   = *addr;
        nd6_hold_send(ln, ifp, (struct sockaddr *)&dst);
    }
    return 0;
}

/* ------------------------------------------------------------------
 * nd6_nbr_sdl - ln's link-layer address, as the sockaddr_dl naming its
 * interface that the routing socket and GetRouteInfo() hand out.
 * ------------------------------------------------------------------ */
void
nd6_nbr_sdl(const struct llinfo_nd6 *ln, struct sockaddr_dl *sdl)
{
    const struct ifnet *ifp = ln->ln_ifp;
    char name[IFNAMSIZ];
    int nlen;

    bzero(sdl, sizeof(*sdl));
    nlen = if_fullname(ifp, name, sizeof(name));    /* the core's, as nd6_storelladdr() */
    sdl->sdl_family = AF_LINK;
    sdl->sdl_index  = ifp->if_index;
    sdl->sdl_type   = ifp->if_type;
    sdl->sdl_nlen   = nlen;
    sdl->sdl_alen   = ln->ln_alen;
    bcopy(name, sdl->sdl_data, nlen);
    bcopy(ln->ln_lladdr, sdl->sdl_data + nlen, ln->ln_alen);
    sdl->sdl_len = (u_char)(sdl->sdl_data - (char *)sdl) + nlen + ln->ln_alen;
}

/* ------------------------------------------------------------------
 * nd6_is_addr_neighbor - check if addr is on the same link as ifp.
 * ------------------------------------------------------------------ */
int
nd6_is_addr_neighbor(struct sockaddr_in6 *addr, struct ifnet *ifp)
{
    struct in6_ifaddr *ia;

    /* link-local addresses are always neighbors */
    if(IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr))
        return 1;

    /* check if address matches any prefix on this interface */
    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(ia->ia6_ifp != ifp)
            continue;
        /* compare prefix bits */
        if(ia->ia6_plen > 0) {
            int plen = ia->ia6_plen;
            int i;
            int match = 1;
            for(i = 0; i < 16 && plen > 0; i++, plen -= 8) {
                u_int8_t mask;
                if(plen >= 8)
                    mask = 0xff;
                else
                    mask = (0xff << (8 - plen)) & 0xff;
                if((ia->ia_addr.sin6_addr.s6_addr[i] & mask) !=
                        (addr->sin6_addr.s6_addr[i] & mask)) {
                    match = 0;
                    break;
                }
            }
            if(match)
                return 1;
        }
    }

    /* check neighbor cache */
    return nd6_nbr_lookup(ifp, &addr->sin6_addr, 0) != NULL;
}

/* ------------------------------------------------------------------
 * nd6_cache_lladdr - update neighbor cache with a link-layer address.
 *
 * Called when we learn a neighbor's link-layer address from NS, NA, RA,
 * or Redirect messages.
 *
 * type     - ND message type that triggered this (ND_NEIGHBOR_SOLICIT, etc.)
 * is_router - set ln_router flag
 *
 * Returns 0 on success, errno on failure.
 * ------------------------------------------------------------------ */
int
nd6_cache_lladdr(struct ifnet *ifp, const struct in6_addr *from,
                 char *lladdr, int lladdrlen, int type, int is_router)
{
    struct llinfo_nd6 *ln;
    int do_update = 0;
    int olladdr;		/* old lladdr present */
    int llchange = 0;	/* lladdr changed */

    ln = nd6_nbr_lookup(ifp, from, 1);
    if(ln == NULL)
        return ENOBUFS;
    if(ln->ln_flags & ND6_LNF_KEEP)
        return 0;           /* set by hand, or ours: left as it is */
    if(lladdrlen > ND6_LLADDRLEN)
        lladdr = NULL;

    olladdr = ln->ln_alen > 0;

    /* check if address changed */
    if(olladdr && lladdr && lladdrlen > 0)
        llchange = bcmp(lladdr, ln->ln_lladdr, lladdrlen);

    /* store new link-layer address */
    if(lladdr && lladdrlen > 0) {
        bcopy(lladdr, ln->ln_lladdr, lladdrlen);
        ln->ln_alen = lladdrlen;
        do_update = 1;
    }

    /* state transitions based on RFC 4861 */
    if(ln->ln_state == ND6_LLINFO_INCOMPLETE) {
        if(do_update)
            nd6_nbr_stale(ln);
    } else if(do_update && (llchange || !olladdr))
        nd6_nbr_stale(ln);

    if(is_router)
        ln->ln_router = 1;

    /* flush held packet if address was resolved */
    if(do_update && ln->ln_hold) {
        struct sockaddr_in6 dst_sa;

        bzero(&dst_sa, sizeof(dst_sa));
        dst_sa.sin6_family = AF_INET6;
        dst_sa.sin6_len    = sizeof(dst_sa);
        dst_sa.sin6_addr   = *from;
        nd6_hold_send(ln, ifp, (struct sockaddr *)&dst_sa);
    }
    return 0;
}

/* ------------------------------------------------------------------
 * nd6_resolve - resolve the next hop of a packet to a link-layer address.
 *
 * Called by interface output routines needing a link-layer address, with
 * the packet's next hop on ifp: the destination itself, or for a route
 * through a router (RTF_GATEWAY) the router's address. It is looked up in
 * ifp's entries (nd6_nbr_lookup()), so one entry serves every route
 * through a router, and a link-local address is resolved on the link the
 * packet leaves by.
 *
 * Returns 0 on success (lladdr filled), EAGAIN if resolution in progress
 * (the packet is queued), else an error and the caller frees the packet.
 * ------------------------------------------------------------------ */
int
nd6_resolve(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
            u_char *lladdr)
{
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)dst;
    struct llinfo_nd6 *ln;

    /* multicast: map directly (RFC 2464 §7) */
    if(IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
        lladdr[0] = 0x33;
        lladdr[1] = 0x33;
        lladdr[2] = sin6->sin6_addr.s6_addr[12];
        lladdr[3] = sin6->sin6_addr.s6_addr[13];
        lladdr[4] = sin6->sin6_addr.s6_addr[14];
        lladdr[5] = sin6->sin6_addr.s6_addr[15];
        return 0;
    }

    if((ln = nd6_nbr_lookup(ifp, &sin6->sin6_addr, 1)) == NULL)
        return ENOBUFS;

    switch(ln->ln_state) {
    case ND6_LLINFO_REACHABLE:
    case ND6_LLINFO_STALE:
    case ND6_LLINFO_DELAY:
    case ND6_LLINFO_PROBE:
        if(ln->ln_alen > 0) {
            bcopy(ln->ln_lladdr, lladdr, ln->ln_alen);
            /* trigger NUD: STALE → DELAY on data traffic */
            if(ln->ln_state == ND6_LLINFO_STALE) {
                ln->ln_state = ND6_LLINFO_DELAY;
                ln->ln_expire = nd6_secs() + ND6_DELAY_FIRST_PROBE_TIME;
            }
            return 0;
        }
        break;

    case ND6_LLINFO_INCOMPLETE:
        /* queue the packet; nd6_timer() asks again, a RetransTimer apart */
        nd6_hold_add(ln, m);
        return EAGAIN;

    case ND6_LLINFO_NOSTATE:
    default:
        ln->ln_state = ND6_LLINFO_INCOMPLETE;
        ln->ln_asked = 0;
        nd6_hold_add(ln, m);
        nd6_solicit(ln, 0);
        return EAGAIN;
    }

    /* caller frees mbuf on error */
    return EHOSTUNREACH;
}

/* ------------------------------------------------------------------
 * nd6_nud_hint - upper-layer reachability hint (e.g. TCP ACK received)
 * for the neighbour addr on ifp.
 *
 * If the neighbor is in STALE/DELAY/PROBE, promote to REACHABLE.
 * ------------------------------------------------------------------ */
void
nd6_nud_hint(struct ifnet *ifp, const struct in6_addr *addr)
{
    struct llinfo_nd6 *ln = nd6_nbr_lookup(ifp, addr, 0);

    if(ln == NULL || (ln->ln_flags & ND6_LNF_KEEP))
        return;
    if(ln->ln_state != ND6_LLINFO_REACHABLE &&
            ln->ln_state != ND6_LLINFO_INCOMPLETE &&
            ln->ln_state != ND6_LLINFO_NOSTATE) {
        ln->ln_state  = ND6_LLINFO_REACHABLE;
        ln->ln_expire = nd6_secs() + ND6_REACHABLE_TIME / 1000;
    }
}

/* ==================================================================
 * NUD Timer (RFC 4861 §7.3)
 * ================================================================== */


/*
 * nd6_solicit - send ln's neighbour a Neighbor Solicitation, as address
 * resolution or unreachability detection asks (RFC 4861 sections 7.2.2,
 * 7.3.3): to its solicited-node group while it is INCOMPLETE, to itself
 * in PROBE (nd6_ns_output() tells them apart). The next one, if it must be
 * asked again, is a RetransTimer later (the interface's, from Router
 * Advertisements, 1 s by default), counted in nd6_timer() ticks - one
 * more when it is sent between them (from_timer 0), so that the next one
 * is never sooner.
 */
static void
nd6_solicit(struct llinfo_nd6 *ln, int from_timer)
{
    struct ifnet *ifp = ln->ln_ifp;
    const struct nd_ifinfo *ndi;
    u_int32_t retrans = ND6_RETRANS_TIMER;
    struct in6_addr dst = ln->ln_addr;

    if((ndi = nd6_ifinfo(ifp)) != NULL && ndi->retrans != 0)
        retrans = ndi->retrans;
    nd6_ns_output(ifp, NULL, &dst, ln, 0);
    ln->ln_asked++;
    ln->ln_ntick = (retrans * PR_SLOWHZ + 999) / 1000 + (from_timer ? 0 : 1);
}

/*
 * The neighbour of ln did not answer: the entry is deleted (RFC 4861
 * section 7.3.3), and the default router list is told. The packets
 * waiting, if any, are answered with errors (unreachable) first.
 */
static void
nd6_nbr_unreachable(struct llinfo_nd6 *ln, int unreachable)
{
    struct in6_addr addr = ln->ln_addr;
    struct ifnet *ifp = ln->ln_ifp;

    nd6_hold_free(ln, unreachable);
    nd6_nbr_free(ln);
    defrouter_unreachable(&addr, ifp);
}

/* ------------------------------------------------------------------
 * nd6_probe - find out whether addr on ifp answers: address resolution,
 * as for a packet to it, without the packet. Unless it is under way or
 * the address is known already.
 * ------------------------------------------------------------------ */
void
nd6_probe(struct ifnet *ifp, const struct in6_addr *addr)
{
    struct llinfo_nd6 *ln = nd6_nbr_lookup(ifp, addr, 1);

    if(ln != NULL && ln->ln_state == ND6_LLINFO_NOSTATE) {
        ln->ln_state = ND6_LLINFO_INCOMPLETE;
        ln->ln_asked = 0;
        nd6_solicit(ln, 0);
    }
}

/* ------------------------------------------------------------------
 * nd6_timer - periodic ND6 timer (called from slowtimo path).
 *
 * Drives the NUD state machine for all neighbor cache entries,
 * plus DAD, prefix, and default router timers.
 * ------------------------------------------------------------------ */
void
nd6_timer(void *arg)
{
    struct llinfo_nd6 *ln, *nln;
    long now;

    now = nd6_secs();

    nd6_timer_ticks++;

    /* log first 10 calls, then every ~30 seconds */
    if(nd6_timer_ticks <= 10 || (nd6_timer_ticks % 60) == 0) {
        D(bug("[AROSTCP:ND6] %s: tick=%d now=%ld\n",
              __func__, nd6_timer_ticks, now));
    }

    /*
     * NUD state machine for each neighbor cache entry. Only ln itself is
     * freed here; what that sets off (choosing another default router,
     * which may probe it and so make an entry, or take an idle one over in
     * place) never frees another, so nln stays valid.
     */
    for(ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = nln) {
        nln = ln->ln_next;

        switch(ln->ln_state) {
        case ND6_LLINFO_INCOMPLETE:
            if(--ln->ln_ntick > 0)
                break;          /* a RetransTimer since the last one */
            if(ln->ln_asked >= ND6_MAX_MULTICAST_SOLICIT) {
                /* address resolution failed (section 7.2.2) */
                nd6stat.nd6s_nud_incomplete++;
                nd6_nbr_unreachable(ln, 1);
            } else
                nd6_solicit(ln, 1);
            break;

        case ND6_LLINFO_REACHABLE:
            if(ln->ln_expire && now > ln->ln_expire)
                nd6_nbr_stale(ln);
            break;

        case ND6_LLINFO_NOSTATE:
        case ND6_LLINFO_STALE:
            /* stays until data traffic triggers DELAY, or a day unused */
            if(ln->ln_expire && now > ln->ln_expire && ln->ln_hold == NULL &&
                    !(ln->ln_flags & ND6_LNF_KEEP))
                nd6_nbr_free(ln);
            break;

        case ND6_LLINFO_DELAY:
            if(ln->ln_expire && now > ln->ln_expire) {
                /* the first probe now, unicast (section 7.3.3) */
                ln->ln_state = ND6_LLINFO_PROBE;
                ln->ln_asked = 0;
                nd6_solicit(ln, 1);
            }
            break;

        case ND6_LLINFO_PROBE:
            if(--ln->ln_ntick > 0)
                break;
            if(ln->ln_asked >= ND6_MAX_UNICAST_SOLICIT) {
                /* NUD failed */
                nd6stat.nd6s_nud_probe++;
                nd6_nbr_unreachable(ln, 0);
            } else
                nd6_solicit(ln, 1);
            break;

        default:
            break;
        }
    }

    /* run sub-timers */
    D(bug("[AROSTCP:ND6] %s: calling nd6_dad_timer\n", __func__));
    nd6_dad_timer();
    D(bug("[AROSTCP:ND6] %s: calling nd6_defrouter_timer\n", __func__));
    nd6_defrouter_timer();
    D(bug("[AROSTCP:ND6] %s: calling nd6_prefix_timer\n", __func__));
    nd6_prefix_timer();
    ndb_ra_timer();             /* DNS servers and domains from RAs */
    api_addrchange_retry();     /* an address change signal still to send */
    D(bug("[AROSTCP:ND6] %s: sub-timers done\n", __func__));

    /* RS retransmission for interfaces still soliciting */
    {
        struct ifnet *iifp;
        struct nd_ifinfo *ndi;
        for(iifp = ifnet_list; iifp; iifp = iifp->if_next) {
            if(iifp->if_index >= ND6_MAXIFS)
                continue;
            ndi = &nd6_ndi[iifp->if_index];
            if(!ndi->initialized)
                continue;
            if(ndi->rtr_soliciting > 0 &&
                    ndi->rtr_soliciting < ND6_MAX_RTR_SOLICITATIONS &&
                    ndi->rtr_solicit_timer != 0 &&
                    now >= ndi->rtr_solicit_timer) {
                nd6_rs_output(iifp);
                ndi->rtr_soliciting++;
                ndi->rtr_solicit_timer = now +
                                         ND6_RTR_SOLICITATION_INTERVAL;
            }
        }
    }
    if(nd6_timer_ticks <= 10) {
        D(bug("[AROSTCP:ND6] %s: tick=%d returning\n",
              __func__, nd6_timer_ticks));
    }
}

/* ==================================================================
 * Routing Table Integration
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_storelladdr - convert route gateway to AF_LINK sockaddr_dl.
 *
 * When a route is cloned from an RTF_CLONING prefix route via
 * RTM_RESOLVE, the gateway is copied from the parent and has
 * sa_family == AF_INET6.  NDP needs an AF_LINK (sockaddr_dl)
 * gateway so that nd6_cache_lladdr() and nd6_resolve() can store
 * and retrieve the resolved link-layer address.
 *
 * The gateway buffer is ROUNDUP(sizeof(sockaddr_in6)) = 32 bytes
 * on LP64, which is large enough for the sockaddr_dl header (8 bytes)
 * plus a typical interface name (4-8 chars) plus a 6-byte MAC.
 * ------------------------------------------------------------------ */
void
nd6_storelladdr(struct rtentry *rt)
{
    struct ifnet *ifp = rt->rt_ifp;
    struct sockaddr_dl *sdl;
    int namelen;
    char namebuf[IFNAMSIZ];
    int sdl_len;
    int buflen;

    if(rt->rt_gateway == NULL)
        return;
    if(rt->rt_gateway->sa_family == AF_LINK)
        return;		/* already converted */

    /*
     * Compute available gateway buffer size.  rtrequest() allocated
     * ROUNDUP(original_gateway->sa_len) bytes.  For AF_INET6 gateways
     * from cloned routes, that is ROUNDUP(28) = 32 on LP64.
     */
#define	ND6_ROUNDUP(a)	\
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
    buflen = ND6_ROUNDUP(rt->rt_gateway->sa_len);
    if(buflen < (int)sizeof(struct sockaddr_dl) - 46 + IFNAMSIZ + 6) {
        /* sanity: buffer must hold at least the fixed header */
        buflen = ND6_ROUNDUP(sizeof(struct sockaddr_in6));
    }

    namelen = if_fullname(ifp, namebuf, sizeof(namebuf));

    /* sdl_len includes space for name + MAC (even though MAC is empty) */
    sdl_len = 8 + namelen + ifp->if_addrlen;
    if(sdl_len > buflen) {
        D(bug("[AROSTCP:ND6] %s: sdl_len %d > buflen %d, truncating\n",
              __func__, sdl_len, buflen));
        sdl_len = buflen;
    }

    sdl = (struct sockaddr_dl *)rt->rt_gateway;
    bzero(sdl, buflen);
    sdl->sdl_len    = sdl_len;
    sdl->sdl_family = AF_LINK;
    sdl->sdl_index  = ifp->if_index;
    sdl->sdl_type   = ifp->if_type;
    sdl->sdl_nlen   = namelen;
    sdl->sdl_alen   = 0;		/* filled by nd6_cache_lladdr */
    sdl->sdl_slen   = 0;
    bcopy(namebuf, sdl->sdl_data, namelen);
}

/* ------------------------------------------------------------------
 * nd6_rtrequest - called when a route entry changes.
 *
 * Neighbours are not routes (the neighbour cache above): this only gives
 * a route to a link (a prefix on it, or a host route made with no router)
 * an AF_LINK gateway naming its interface, as FreeBSD 4.11's
 *   rt_setgate(rt, rt_key(rt), &null_sdl);
 * did. Since AROSTCP lacks rt_setgate(), it is converted in place by
 * nd6_storelladdr() (the buffer is large enough: ROUNDUP(28) = 32). A
 * route through a router (RTF_GATEWAY) keeps the router's address, and a
 * loopback or point-to-point route its own.
 * ------------------------------------------------------------------ */
void
nd6_rtrequest(int req, struct rtentry *rt, struct sockaddr *sa)
{
    const struct ifnet *ifp;

    if(rt == NULL || req != RTM_ADD || (rt->rt_flags & RTF_GATEWAY) ||
            rt->rt_gateway == NULL)
        return;
    ifp = rt->rt_ifp;
    if(ifp == NULL || (ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)))
        return;
    nd6_storelladdr(rt);
}

/* ==================================================================
 * Interface Helpers
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_setmtu - update link MTU from RA option.
 * ------------------------------------------------------------------ */
void
nd6_setmtu(struct ifnet *ifp)
{
    struct nd_ifinfo *ndi = nd6_ifinfo(ifp);
    if(ndi && ndi->linkmtu > 0 && ndi->linkmtu <= ifp->if_mtu)
        ifp->if_mtu = ndi->linkmtu;
}

/* ------------------------------------------------------------------
 * nd6_ioctl - handle ND6-related ioctls.
 * ------------------------------------------------------------------ */
int
nd6_ioctl(u_long cmd, caddr_t data, struct ifnet *ifp)
{
    /* placeholder for SIOCGND6INFO / SIOCSND6INFO */
    return EOPNOTSUPP;
}

#endif /* INET6 */

/*
 * The neighbour cache is not in the routing table: the administration
 * module's routing socket and GetRouteInfo() reach it through IPv6's
 * nfo_control (NFC_LLINFO_*, net/netmodule.h), as FreeBSD's lltable is
 * reached from its lla_rt_output().
 */
#define NBR_ROUNDUP(a) \
    ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

/*
 * A routing message with RTF_LLINFO about an IPv6 address: RTM_ADD sets a
 * neighbour's link-layer address by hand (ndp -s; the gateway is a
 * sockaddr_dl), RTM_DELETE deletes its entry, RTM_GET reads it. The
 * interface is the gateway's (sdl_index), else rtm_index, else the
 * destination's zone (sin6_scope_id); RTM_GET and RTM_DELETE with none take
 * the address on any interface.
 */
int
nd6_llinfo_rtmsg(struct nfc_llinfo_rtmsg *req)
{
    struct rt_msghdr *rtm = *req->nr_rtm;
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)req->nr_dst;
    struct sockaddr_dl *sdl = NULL;
    struct llinfo_nd6 *ln = NULL;
    struct ifnet *ifp = NULL, *i;
    int index = 0, found = 0;

    if(req->nr_dst->sa_len < sizeof(*sin6))
        return EINVAL;
    if(req->nr_gate != NULL && req->nr_gate->sa_family == AF_LINK)
        sdl = (struct sockaddr_dl *)req->nr_gate;
    if(sdl != NULL && sdl->sdl_index != 0)
        index = sdl->sdl_index;
    else if(rtm->rtm_index != 0)
        index = rtm->rtm_index;
    else
        index = sin6->sin6_scope_id;
    if(index != 0) {
        for(ifp = ifnet_list; ifp != NULL && ifp->if_index != index; ifp = ifp->if_next)
            ;
        if(ifp == NULL)
            return ENXIO;
    }

    switch(rtm->rtm_type) {
    case RTM_ADD:
        if(ifp == NULL || sdl == NULL)
            return EINVAL;
        return nd6_nbr_set(ifp, &sin6->sin6_addr, (u_char *)LLADDR(sdl),
                           sdl->sdl_alen);

    case RTM_DELETE:
        for(i = ifp ? ifp : ifnet_list; i != NULL; i = ifp ? NULL : i->if_next)
            if((ln = nd6_nbr_lookup(i, &sin6->sin6_addr, 0)) != NULL) {
                nd6_nbr_free(ln);
                found = 1;
            }
        return found ? 0 : ESRCH;

    case RTM_GET: {
        struct sockaddr_in6 a;
        struct sockaddr_dl d;
        caddr_t cp;
        int len;

        for(i = ifp ? ifp : ifnet_list; i != NULL && ln == NULL; i = ifp ? NULL : i->if_next)
            ln = nd6_nbr_lookup(i, &sin6->sin6_addr, 0);
        if(ln == NULL)
            return ESRCH;
        bzero(&a, sizeof(a));
        a.sin6_family = AF_INET6;
        a.sin6_len    = sizeof(a);
        a.sin6_addr   = ln->ln_addr;
        if(IN6_IS_ADDR_LINKLOCAL(&ln->ln_addr))
            a.sin6_scope_id = ln->ln_ifp->if_index;
        nd6_nbr_sdl(ln, &d);
        len = sizeof(*rtm) + NBR_ROUNDUP(sizeof(a)) + NBR_ROUNDUP(d.sdl_len);
        if(len > rtm->rtm_msglen) {
            struct rt_msghdr *new_rtm;

            R_Malloc(new_rtm, struct rt_msghdr *, len);
            if(new_rtm == NULL)
                return ENOBUFS;
            bcopy(rtm, new_rtm, sizeof(*rtm));
            bsd_free(rtm, M_RTABLE);
            *req->nr_rtm = rtm = new_rtm;
        }
        rtm->rtm_msglen = len;
        rtm->rtm_flags  = RTF_UP | RTF_HOST | RTF_LLINFO |
                          ((ln->ln_flags & ND6_LNF_STATIC) ? RTF_STATIC : 0);
        rtm->rtm_index  = ln->ln_ifp->if_index;
        rtm->rtm_addrs  = RTA_DST | RTA_GATEWAY;
        bzero(&rtm->rtm_rmx, sizeof(rtm->rtm_rmx));
        rtm->rtm_rmx.rmx_expire = rt_expire_out(ln->ln_expire);
        cp = (caddr_t)(rtm + 1);
        bcopy(&a, cp, sizeof(a));
        cp += NBR_ROUNDUP(sizeof(a));
        bcopy(&d, cp, d.sdl_len);
        return 0;
    }

    default:
        return EOPNOTSUPP;
    }
}

/*
 * The neighbour cache as the rows a KAME stack's cloned neighbour routes
 * gave: RTF_UP | RTF_HOST | RTF_LLINFO (RTF_STATIC if set by hand), the
 * address (a link-local one with its zone in sin6_scope_id), the link-layer
 * address as a sockaddr_dl naming the interface (sdl_alen 0 while
 * unresolved), the interface index, and the expiry time as rmx_expire (0:
 * set by hand, kept for good). One row per (interface, address): fe80::1
 * on two links is two rows.
 */
void
nd6_llinfo_rows(struct nfc_llinfo_rows *req)
{
    struct llinfo_nd6 *ln;

    for(ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = ln->ln_next) {
        struct sockaddr_in6 sin6;
        struct sockaddr_dl sdl;
        struct rt_msghdr *rtm;
        char *cp;
        int entry_size;

        nd6_nbr_sdl(ln, &sdl);
        entry_size = sizeof(struct rt_msghdr) + NBR_ROUNDUP(sizeof(sin6)) +
                     NBR_ROUNDUP(sdl.sdl_len);
        if(req->nr_buf == NULL) {
            req->nr_size += entry_size;
            continue;
        }
        rtm = (struct rt_msghdr *)req->nr_buf;
        cp = (char *)(rtm + 1);
        bzero(&sin6, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        sin6.sin6_len    = sizeof(sin6);
        sin6.sin6_addr   = ln->ln_addr;
        if(IN6_IS_ADDR_LINKLOCAL(&ln->ln_addr))
            sin6.sin6_scope_id = ln->ln_ifp->if_index;
        bzero(rtm, sizeof(*rtm));
        rtm->rtm_version = RTM_VERSION;
        rtm->rtm_type    = RTM_GET;
        rtm->rtm_flags   = RTF_UP | RTF_HOST | RTF_LLINFO |
                           ((ln->ln_flags & ND6_LNF_STATIC) ? RTF_STATIC : 0);
        rtm->rtm_index   = ln->ln_ifp->if_index;
        rtm->rtm_rmx.rmx_expire = rt_expire_out(ln->ln_expire);
        bcopy(&sin6, cp, sizeof(sin6));
        cp += NBR_ROUNDUP(sizeof(sin6));
        bcopy(&sdl, cp, sdl.sdl_len);
        rtm->rtm_addrs  = RTA_DST | RTA_GATEWAY;
        rtm->rtm_msglen = entry_size;
        req->nr_buf += entry_size;
    }
}

/*
 * IPv6's resolver for the link type that carries it (sana_linktype_resolver(),
 * net/if_sana.c): the route the packet follows does not matter - dst is the
 * next hop, whose entry is in the neighbour cache of ifp - so it is
 * nd6_resolve().
 */
int
nd6_lt_resolve(struct ifnet *ifp, struct rtentry *rt, struct mbuf *m,
               struct sockaddr *dst, u_char *lladdr)
{
    (void)rt;
    return (nd6_resolve(ifp, m, dst, lladdr));
}
