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
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>

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

/* Forward declaration */
struct rtentry *rtalloc1(struct sockaddr *, int);

/* ------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------ */
struct llinfo_nd6 llinfo_nd6;		/* sentinel for neighbor cache */
struct nd6stat nd6stat = {0};

int nd6_maxndopt   = 10;		/* max options in a single ND msg */
int nd6_maxqueuelen = 1;		/* max packets held per entry */
int nd6_inuse      = 0;		/* neighbor cache entries in use */
int nd6_allocated  = 0;		/* total neighbor cache allocations */

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
 * Neighbor Cache Operations
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_lookup - look up or create a neighbor cache entry.
 *
 * dst    - target IPv6 address
 * create - if non-zero, create a new entry if not found
 * ifp    - interface to use (used for scope, may be NULL)
 *
 * Returns the routing entry, or NULL.
 *
 * Based on FreeBSD 4.11 nd6_lookup():
 *   - If the found route doesn't have RTF_LLINFO (e.g. default route),
 *     return NULL to prevent using non-neighbor routes.
 * ------------------------------------------------------------------ */
struct rtentry *
nd6_lookup(struct in6_addr *dst, int create, struct ifnet *ifp)
{
    struct rtentry *rt;
    struct sockaddr_in6 sin6;

    bzero(&sin6, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len    = sizeof(sin6);
    sin6.sin6_addr   = *dst;

    rt = rtalloc1((struct sockaddr *)&sin6, create);
    if(rt) {
        if((rt->rt_flags & RTF_LLINFO) == 0) {
            /*
             * Found a route but it's not a neighbor cache
             * entry (e.g. default route, gateway route).
             * Don't use it for NDP.
             */
            if(create) {
                rt->rt_refcnt--;
                return NULL;
            }
        }
        rt->rt_refcnt--;
    }

    return rt;
}

/* ------------------------------------------------------------------
 * nd6_free - delete a neighbor cache entry.
 *
 * Removes the route entry and its associated llinfo_nd6.
 * Based on FreeBSD 4.11 nd6_free() (simplified — no default router
 * list management yet).
 * ------------------------------------------------------------------ */
void
nd6_free(struct rtentry *rt)
{
    if(rt == NULL)
        return;

    rtrequest(RTM_DELETE, rt_key(rt), (struct sockaddr *)0,
              rt_mask(rt), 0, (struct rtentry **)0);
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
    {
        struct rtentry *rt = nd6_lookup(&addr->sin6_addr, 0, ifp);
        if(rt && rt->rt_llinfo)
            return 1;
    }

    return 0;
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
nd6_cache_lladdr(struct ifnet *ifp, struct in6_addr *from,
                 char *lladdr, int lladdrlen, int type, int is_router)
{
    struct rtentry *rt;
    struct llinfo_nd6 *ln;
    struct sockaddr_dl *sdl;
    struct timeval _tv;
    int do_update = 0;
    int olladdr = 0;	/* old lladdr present */
    int llchange = 0;	/* lladdr changed */

    rt = nd6_lookup(from, 1, ifp);
    if(rt == NULL)
        return ENOBUFS;

    /*
     * Validate the route: must be an LLINFO host route, not
     * a gateway route.  (FreeBSD 4.11 check)
     */
    if((rt->rt_flags & (RTF_GATEWAY | RTF_LLINFO)) != RTF_LLINFO) {
        nd6_free(rt);
        return EINVAL;
    }

    ln = (struct llinfo_nd6 *)rt->rt_llinfo;
    if(ln == NULL) {
        nd6_free(rt);
        return EINVAL;
    }
    if(rt->rt_gateway == NULL) {
        nd6_free(rt);
        return EINVAL;
    }

    /*
     * Ensure gateway is AF_LINK.  Normally nd6_rtrequest(RTM_RESOLVE)
     * converts it, but defend against routes created by other paths.
     */
    if(rt->rt_gateway && rt->rt_gateway->sa_family != AF_LINK)
        nd6_storelladdr(rt);

    sdl = (rt->rt_gateway && rt->rt_gateway->sa_family == AF_LINK)
          ? (struct sockaddr_dl *)rt->rt_gateway : NULL;

    /* check if we already have an address */
    if(sdl && sdl->sdl_alen > 0)
        olladdr = 1;

    /* check if address changed */
    if(olladdr && lladdr && lladdrlen > 0) {
        llchange = bcmp(lladdr, LLADDR(sdl), lladdrlen);
    }

    /* store new link-layer address */
    if(lladdr && lladdrlen > 0 && sdl) {
        bcopy(lladdr, LLADDR(sdl), lladdrlen);
        sdl->sdl_alen = lladdrlen;
        do_update = 1;
    }

    /* state transitions based on RFC 4861 */
    GetSysTime(&_tv);

    if(ln->ln_state == ND6_LLINFO_INCOMPLETE) {
        if(do_update) {
            ln->ln_state = ND6_LLINFO_STALE;
            ln->ln_expire = _tv.tv_sec + ND6_REACHABLE_TIME / 1000;
        }
    } else {
        if(do_update && llchange)
            ln->ln_state = ND6_LLINFO_STALE;
        else if(do_update && !olladdr)
            ln->ln_state = ND6_LLINFO_STALE;
    }

    if(is_router)
        ln->ln_router = 1;

    /* flush held packet if address was resolved */
    if(do_update && ln->ln_hold) {
        struct mbuf *held = ln->ln_hold;
        ln->ln_hold = NULL;

        if(sdl && sdl->sdl_alen > 0) {
            struct sockaddr_in6 dst_sa;
            bzero(&dst_sa, sizeof(dst_sa));
            dst_sa.sin6_family = AF_INET6;
            dst_sa.sin6_len    = sizeof(dst_sa);
            dst_sa.sin6_addr   = *from;
            (*ifp->if_output)(ifp, held,
                              (struct sockaddr *)&dst_sa, rt);
        } else {
            m_freem(held);
        }
    }

    return 0;
}

/* ------------------------------------------------------------------
 * nd6_resolve - resolve an IPv6 address to a link-layer address.
 *
 * Called by interface output routines needing a link-layer address.
 *
 * Returns 0 on success (lladdr filled), EAGAIN if resolution in progress.
 * ------------------------------------------------------------------ */
int
nd6_resolve(struct ifnet *ifp, struct rtentry *rt, struct mbuf *m,
            struct sockaddr *dst, u_char *lladdr)
{
    struct llinfo_nd6 *ln;
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)dst;

    if(rt == NULL) {
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
        /* caller frees mbuf on error */
        return EHOSTUNREACH;
    }

    ln = (struct llinfo_nd6 *)rt->rt_llinfo;
    if(ln == NULL) {
        /* allocate neighbor cache entry */
        MALLOC(ln, struct llinfo_nd6 *, sizeof(*ln), M_PCB, M_NOWAIT);
        if(ln == NULL) {
            /* caller frees mbuf on error */
            return ENOBUFS;
        }
        bzero(ln, sizeof(*ln));
        rt->rt_llinfo = (caddr_t)ln;
        ln->ln_rt    = rt;
        ln->ln_state = ND6_LLINFO_INCOMPLETE;
        /* insert into list */
        ln->ln_next = llinfo_nd6.ln_next;
        ln->ln_prev = &llinfo_nd6;
        llinfo_nd6.ln_next->ln_prev = ln;
        llinfo_nd6.ln_next = ln;
    }

    switch(ln->ln_state) {
    case ND6_LLINFO_REACHABLE:
    case ND6_LLINFO_STALE:
    case ND6_LLINFO_DELAY:
    case ND6_LLINFO_PROBE:
        /* have a link-layer address — ensure gateway is AF_LINK */
        if(rt->rt_gateway &&
                rt->rt_gateway->sa_family != AF_LINK)
            nd6_storelladdr(rt);
        if(rt->rt_gateway && rt->rt_gateway->sa_family == AF_LINK) {
            struct sockaddr_dl *sdl =
                (struct sockaddr_dl *)rt->rt_gateway;
            if(sdl->sdl_alen > 0) {
                bcopy(LLADDR(sdl), lladdr, sdl->sdl_alen);
                /* trigger NUD: STALE → DELAY on data traffic */
                if(ln->ln_state == ND6_LLINFO_STALE) {
                    struct timeval _tv;
                    GetSysTime(&_tv);
                    ln->ln_state = ND6_LLINFO_DELAY;
                    ln->ln_expire = _tv.tv_sec +
                                    ND6_DELAY_FIRST_PROBE_TIME;
                }
                return 0;
            }
        }
        break;

    case ND6_LLINFO_INCOMPLETE:
        /* queue the packet and send NS */
        if(ln->ln_hold) {
            m_freem(ln->ln_hold);
            nd6stat.nd6s_res_holdq_full++;
        }
        ln->ln_hold = m;
        if(ln->ln_asked < ND6_MAX_MULTICAST_SOLICIT) {
            ln->ln_asked++;
            nd6_ns_output(ifp, NULL, &sin6->sin6_addr, ln, 0);
        }
        return EAGAIN;

    case ND6_LLINFO_NOSTATE:
    default:
        ln->ln_state = ND6_LLINFO_INCOMPLETE;
        ln->ln_asked = 1;
        if(ln->ln_hold) {
            m_freem(ln->ln_hold);
            nd6stat.nd6s_res_holdq_full++;
        }
        ln->ln_hold = m;
        nd6_ns_output(ifp, NULL, &sin6->sin6_addr, ln, 0);
        return EAGAIN;
    }

    /* caller frees mbuf on error */
    return EHOSTUNREACH;
}

/* ------------------------------------------------------------------
 * nd6_nud_hint - upper-layer reachability hint (e.g. TCP ACK received).
 *
 * If the neighbor is in STALE/DELAY/PROBE, promote to REACHABLE.
 * ------------------------------------------------------------------ */
void
nd6_nud_hint(struct rtentry *rt)
{
    struct llinfo_nd6 *ln;
    struct timeval _tv;

    if(rt == NULL)
        return;
    ln = (struct llinfo_nd6 *)rt->rt_llinfo;
    if(ln == NULL)
        return;

    if(ln->ln_state != ND6_LLINFO_REACHABLE &&
            ln->ln_state != ND6_LLINFO_INCOMPLETE &&
            ln->ln_state != ND6_LLINFO_NOSTATE) {
        GetSysTime(&_tv);
        ln->ln_state  = ND6_LLINFO_REACHABLE;
        ln->ln_expire = _tv.tv_sec + ND6_REACHABLE_TIME / 1000;
    }
}

/* ==================================================================
 * NUD Timer (RFC 4861 §7.3)
 * ================================================================== */

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
    struct timeval _tv;
    long now;
    static int nd6_timer_ticks = 0;

    GetSysTime(&_tv);
    now = _tv.tv_sec;

    nd6_timer_ticks++;

    /* log first 10 calls, then every ~30 seconds */
    if(nd6_timer_ticks <= 10 || (nd6_timer_ticks % 60) == 0) {
        D(bug("[AROSTCP:ND6] %s: tick=%d now=%ld\n",
              __func__, nd6_timer_ticks, now));
    }

    /* NUD state machine for each neighbor cache entry */
    for(ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = nln) {
        nln = ln->ln_next;

        switch(ln->ln_state) {
        case ND6_LLINFO_INCOMPLETE:
            if(ln->ln_asked >= ND6_MAX_MULTICAST_SOLICIT) {
                /* unreachable: free held packet */
                nd6stat.nd6s_nud_incomplete++;
                if(ln->ln_hold) {
                    m_freem(ln->ln_hold);
                    ln->ln_hold = NULL;
                }
                ln->ln_state = ND6_LLINFO_NOSTATE;
            } else {
                /* retransmit NS */
                if(ln->ln_rt && ln->ln_rt->rt_ifp) {
                    struct sockaddr_in6 dst;
                    bzero(&dst, sizeof(dst));
                    dst.sin6_family = AF_INET6;
                    dst.sin6_len    = sizeof(dst);
                    dst.sin6_addr =
                        ((struct sockaddr_in6 *)
                         rt_key(ln->ln_rt))->sin6_addr;
                    nd6_ns_output(ln->ln_rt->rt_ifp,
                                  NULL, &dst.sin6_addr, ln, 0);
                }
                ln->ln_asked++;
            }
            break;

        case ND6_LLINFO_REACHABLE:
            if(ln->ln_expire && now > ln->ln_expire)
                ln->ln_state = ND6_LLINFO_STALE;
            break;

        case ND6_LLINFO_STALE:
            /* stays STALE until data traffic triggers DELAY */
            break;

        case ND6_LLINFO_DELAY:
            if(ln->ln_expire && now > ln->ln_expire) {
                ln->ln_state = ND6_LLINFO_PROBE;
                ln->ln_asked = 0;
            }
            break;

        case ND6_LLINFO_PROBE:
            if(ln->ln_asked >= ND6_MAX_UNICAST_SOLICIT) {
                /* NUD failed */
                nd6stat.nd6s_nud_probe++;
                if(ln->ln_hold) {
                    m_freem(ln->ln_hold);
                    ln->ln_hold = NULL;
                }
                ln->ln_state = ND6_LLINFO_NOSTATE;
            } else {
                /* send unicast NS */
                if(ln->ln_rt && ln->ln_rt->rt_ifp) {
                    struct sockaddr_in6 dst;
                    bzero(&dst, sizeof(dst));
                    dst.sin6_family = AF_INET6;
                    dst.sin6_len    = sizeof(dst);
                    dst.sin6_addr =
                        ((struct sockaddr_in6 *)
                         rt_key(ln->ln_rt))->sin6_addr;
                    nd6_ns_output(ln->ln_rt->rt_ifp,
                                  NULL, &dst.sin6_addr, ln, 0);
                }
                ln->ln_asked++;
            }
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
    D(bug("[AROSTCP:ND6] %s: sub-timers done\n", __func__));

    /* RS retransmission for interfaces still soliciting */
    {
        extern struct ifnet *ifnet;
        struct ifnet *iifp;
        struct nd_ifinfo *ndi;
        for(iifp = ifnet; iifp; iifp = iifp->if_next) {
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

    sprintf(namebuf, "%s%d", ifp->if_name, ifp->if_unit);
    namelen = strlen(namebuf);

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
 * Manages neighbor cache (llinfo_nd6) entries attached to routing
 * entries.  Based on FreeBSD 4.11 / KAME nd6_rtrequest().
 *
 * RTM_ADD with RTF_CLONING: convert gateway to AF_LINK (sockaddr_dl)
 *   for the prefix route.  When cloned, children inherit AF_LINK.
 *   Since AROSTCP lacks rt_setgate(), we convert in-place via
 *   nd6_storelladdr() (buffer is large enough: ROUNDUP(28) = 32).
 *
 * RTM_ADD without RTF_CLONING: manually-added host route with
 *   known link-layer address.  State = REACHABLE.
 *
 * RTM_RESOLVE: cloned route needing resolution.
 *   State = NOSTATE (will become INCOMPLETE when NS is sent).
 *
 * RTM_DELETE: free held packet, unlink from list.
 * ------------------------------------------------------------------ */
void
nd6_rtrequest(int req, struct rtentry *rt, struct sockaddr *sa)
{
    struct sockaddr *gate;
    struct llinfo_nd6 *ln;
    struct ifnet *ifp;
    struct sockaddr_dl *sdl;

    if(rt == NULL)
        return;
    gate = rt->rt_gateway;
    ln = (struct llinfo_nd6 *)rt->rt_llinfo;
    ifp = rt->rt_ifp;

    switch(req) {
    case RTM_ADD:
        /*
         * For RTF_CLONING prefix routes (and RTF_LLINFO host
         * routes added directly), convert gateway to AF_LINK.
         * This matches FreeBSD 4.11's:
         *   rt_setgate(rt, rt_key(rt), &null_sdl);
         */
        if((rt->rt_flags & (RTF_CLONING | RTF_LLINFO)) != 0) {
            nd6_storelladdr(rt);
            gate = rt->rt_gateway;
            if(ln) {
                struct timeval _tv;
                GetSysTime(&_tv);
                ln->ln_expire = _tv.tv_sec ? _tv.tv_sec : 1;
            }
            if(rt->rt_flags & RTF_CLONING)
                break;	/* prefix route: no llinfo needed */
        }
    /* FALLTHROUGH */

    case RTM_RESOLVE:
        if(ifp &&
                (ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) == 0) {
            /*
             * Validate/convert gateway to AF_LINK.
             * For cloned routes the parent's AF_LINK gateway
             * was already copied; for manual routes or routes
             * from other paths, convert now.
             */
            if(gate->sa_family != AF_LINK) {
                nd6_storelladdr(rt);
                gate = rt->rt_gateway;
            }
            sdl = (struct sockaddr_dl *)gate;
            sdl->sdl_type = ifp->if_type;
            sdl->sdl_index = ifp->if_index;
        }
        if(ln != NULL)
            break;	/* already have neighbor entry (route change) */

        /*
         * Allocate neighbor cache entry (llinfo_nd6).
         */
        MALLOC(ln, struct llinfo_nd6 *, sizeof(*ln), M_PCB, M_NOWAIT);
        if(ln == NULL) {
            log(LOG_DEBUG, "nd6_rtrequest: malloc failed\n");
            break;
        }
        nd6_inuse++;
        nd6_allocated++;
        bzero(ln, sizeof(*ln));
        rt->rt_llinfo = (caddr_t)ln;
        ln->ln_rt = rt;

        if(req == RTM_ADD) {
            /*
             * Manually added route: gateway should already
             * contain a valid link-layer address.
             */
            ln->ln_state = ND6_LLINFO_REACHABLE;
        } else {
            /*
             * RTM_RESOLVE: cloned from prefix route.
             * rt_expire is 0 from rtrequest(), set a
             * non-zero expire so NUD timer can process it.
             */
            struct timeval _tv;
            ln->ln_state = ND6_LLINFO_NOSTATE;
            GetSysTime(&_tv);
            ln->ln_expire = _tv.tv_sec ? _tv.tv_sec : 1;
        }
        rt->rt_flags |= RTF_LLINFO;

        /* insert into global list */
        ln->ln_next = llinfo_nd6.ln_next;
        ln->ln_prev = &llinfo_nd6;
        llinfo_nd6.ln_next->ln_prev = ln;
        llinfo_nd6.ln_next = ln;

        /*
         * If rt_key(rt) is one of our own addresses on this
         * interface, mark permanently reachable and fill in
         * our own MAC.  (FreeBSD 4.11: in6ifa_ifpwithaddr check)
         */
        if(ifp && gate->sa_family == AF_LINK) {
            struct ifaddr *ifa;
            struct sockaddr_in6 *rt_sin6 =
                (struct sockaddr_in6 *)rt_key(rt);

            for(ifa = ifp->if_addrlist; ifa;
                    ifa = ifa->ifa_next) {
                if(ifa->ifa_addr->sa_family != AF_INET6)
                    continue;
                if(IN6_ARE_ADDR_EQUAL(
                            &((struct sockaddr_in6 *)
                              ifa->ifa_addr)->sin6_addr,
                            &rt_sin6->sin6_addr))
                    break;
            }
            if(ifa) {
                caddr_t mac = nd6_ifptomac(ifp);
                ln->ln_expire = 0; /* permanent */
                ln->ln_state = ND6_LLINFO_REACHABLE;
                if(mac) {
                    sdl = (struct sockaddr_dl *)gate;
                    bcopy(mac, LLADDR(sdl),
                          ifp->if_addrlen);
                    sdl->sdl_alen = ifp->if_addrlen;
                }
            }
        }
        break;

    case RTM_DELETE:
        if(ln == NULL)
            break;
        nd6_inuse--;
        /* free held packet */
        if(ln->ln_hold) {
            m_freem(ln->ln_hold);
            ln->ln_hold = NULL;
        }
        /* unlink from list */
        ln->ln_prev->ln_next = ln->ln_next;
        ln->ln_next->ln_prev = ln->ln_prev;
        rt->rt_llinfo = NULL;
        rt->rt_flags &= ~RTF_LLINFO;
        FREE(ln, M_PCB);
        break;
    }
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
