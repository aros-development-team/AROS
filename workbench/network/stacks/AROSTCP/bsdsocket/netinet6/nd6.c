/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 Neighbor Discovery implementation.
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * nd6.c - IPv6 Neighbor Discovery Protocol (RFC 4861).
 *
 * Implements:
 *   - Neighbor cache management
 *   - Neighbor Solicitation (NS) / Neighbor Advertisement (NA) I/O
 *   - Router Solicitation / Advertisement handling
 *   - Address resolution (mapping IPv6 address -> link-layer address)
 *   - Neighbor Unreachability Detection (NUD)
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

/* Forward declaration for route allocation function */
struct rtentry *rtalloc1(struct sockaddr *, int);

/* ------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------ */
struct llinfo_nd6 llinfo_nd6;           /* sentinel for neighbor cache */
struct nd_ifinfo *nd_ifinfo = NULL;     /* per-interface ND info */

int nd6_maxndopt   = 10;
int nd6_maxqueuelen = 1;

/* ------------------------------------------------------------------
 * nd6_init - initialise ND6.
 * ------------------------------------------------------------------ */
void
nd6_init(void)
{
    /* initialise sentinel */
    llinfo_nd6.ln_next = &llinfo_nd6;
    llinfo_nd6.ln_prev = &llinfo_nd6;
    llinfo_nd6.ln_state = ND6_LLINFO_NOSTATE;
}

/* ------------------------------------------------------------------
 * nd6_ifattach - initialise ND state for a new interface.
 * ------------------------------------------------------------------ */
void
nd6_ifattach(struct ifnet *ifp)
{
    /* nothing needed in this implementation yet */
    (void)ifp;
}

/* ------------------------------------------------------------------
 * nd6_ifptomac - return pointer to interface MAC address.
 * ------------------------------------------------------------------ */
caddr_t
nd6_ifptomac(struct ifnet *ifp)
{
    struct sockaddr_dl *sdl;
    struct ifaddr *ifa;

    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_LINK) {
            sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            return LLADDR(sdl);
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------
 * nd6_lookup - look up or create a neighbor cache entry.
 *
 * dst    - target IPv6 address
 * create - if non-zero, create a new entry if not found
 * ifp    - interface to use
 *
 * Returns the routing entry, or NULL.
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
    if (rt)
        rt->rt_refcnt--;

    return rt;
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
    caddr_t mac;

    if (rt == NULL) {
        /* multicast: map directly */
        if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
            lladdr[0] = 0x33;
            lladdr[1] = 0x33;
            lladdr[2] = sin6->sin6_addr.s6_addr[12];
            lladdr[3] = sin6->sin6_addr.s6_addr[13];
            lladdr[4] = sin6->sin6_addr.s6_addr[14];
            lladdr[5] = sin6->sin6_addr.s6_addr[15];
            return 0;
        }
        m_freem(m);
        return EHOSTUNREACH;
    }

    ln = (struct llinfo_nd6 *)rt->rt_llinfo;
    if (ln == NULL) {
        /* allocate neighbor cache entry */
        MALLOC(ln, struct llinfo_nd6 *, sizeof(*ln), M_PCB, M_NOWAIT);
        if (ln == NULL) {
            m_freem(m);
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

    switch (ln->ln_state) {
    case ND6_LLINFO_REACHABLE:
    case ND6_LLINFO_STALE:
    case ND6_LLINFO_DELAY:
    case ND6_LLINFO_PROBE:
        /* have a link-layer address */
        if (rt->rt_gateway->sa_family == AF_LINK) {
            struct sockaddr_dl *sdl =
                (struct sockaddr_dl *)rt->rt_gateway;
            if (sdl->sdl_alen > 0) {
                bcopy(LLADDR(sdl), lladdr, sdl->sdl_alen);
                return 0;
            }
        }
        break;

    case ND6_LLINFO_INCOMPLETE:
        /* queue the packet and send NS */
        if (ln->ln_hold)
            m_freem(ln->ln_hold);
        ln->ln_hold = m;
        if (ln->ln_asked < ND6_MAX_MULTICAST_SOLICIT) {
            ln->ln_asked++;
            nd6_ns_output(ifp, NULL, &sin6->sin6_addr, ln, 0);
        }
        return EAGAIN;

    case ND6_LLINFO_NOSTATE:
    default:
        ln->ln_state = ND6_LLINFO_INCOMPLETE;
        ln->ln_asked = 1;
        if (ln->ln_hold)
            m_freem(ln->ln_hold);
        ln->ln_hold = m;
        nd6_ns_output(ifp, NULL, &sin6->sin6_addr, ln, 0);
        return EAGAIN;
    }

    m_freem(m);
    return EHOSTUNREACH;
}

/* ------------------------------------------------------------------
 * nd6_ns_output - send a Neighbor Solicitation.
 *
 * src     - source address (NULL = use link-local of ifp)
 * tgt     - target (address being resolved)
 * ln      - neighbor cache entry (may be NULL for DAD)
 * dad     - non-zero if this is a DAD probe (src = ::)
 * ------------------------------------------------------------------ */
void
nd6_ns_output(struct ifnet *ifp, struct in6_addr *src,
              struct in6_addr *tgt, struct llinfo_nd6 *ln, int dad)
{
    struct mbuf *m;
    struct ip6_hdr *ip6;
    struct nd_neighbor_solicit *nd_ns;
    struct nd_opt_hdr *nd_opt;
    struct sockaddr_in6 dst_sa;
    int hlen = sizeof(struct ip6_hdr);
    int icmp6len = sizeof(struct nd_neighbor_solicit);
    int optlen   = 0;
    caddr_t mac;
    int pktlen;

    /* include SLLA option unless DAD */
    mac = nd6_ifptomac(ifp);
    if (!dad && mac && ifp->if_addrlen == 6)
        optlen = 8;  /* 1 x 8-byte option */

    pktlen = hlen + icmp6len + optlen;

    m = m_gethdr(M_DONTWAIT, MT_DATA);
    if (m == NULL)
        return;

    m->m_pkthdr.len = m->m_len = pktlen;
    m->m_pkthdr.rcvif = NULL;

    ip6    = mtod(m, struct ip6_hdr *);
    nd_ns  = (struct nd_neighbor_solicit *)(ip6 + 1);

    /* IPv6 header */
    bzero(ip6, sizeof(*ip6));
    ip6->ip6_vfc  = IPV6_VERSION;
    ip6->ip6_nxt  = IPPROTO_ICMPV6;
    ip6->ip6_hlim = 255;                /* required by RFC 4861 */
    ip6->ip6_plen = htons(icmp6len + optlen);

    /* destination: solicited-node multicast ff02::1:ffXX:XXXX */
    ip6->ip6_dst.s6_addr[0]  = 0xff;
    ip6->ip6_dst.s6_addr[1]  = 0x02;
    ip6->ip6_dst.s6_addr[11] = 0x01;
    ip6->ip6_dst.s6_addr[12] = 0xff;
    ip6->ip6_dst.s6_addr[13] = tgt->s6_addr[13];
    ip6->ip6_dst.s6_addr[14] = tgt->s6_addr[14];
    ip6->ip6_dst.s6_addr[15] = tgt->s6_addr[15];

    /* source address */
    if (dad) {
        bzero(&ip6->ip6_src, sizeof(ip6->ip6_src));   /* :: for DAD */
    } else if (src) {
        ip6->ip6_src = *src;
    } else {
        struct in6_ifaddr *ia = in6_ifaof_ifpforlinklocal(ifp);
        if (ia)
            ip6->ip6_src = ia->ia_addr.sin6_addr;
    }

    /* ICMPv6 NS */
    bzero(nd_ns, sizeof(*nd_ns));
    nd_ns->nd_ns_type   = ND_NEIGHBOR_SOLICIT;
    nd_ns->nd_ns_target = *tgt;

    /* SLLA option */
    if (optlen) {
        nd_opt = (struct nd_opt_hdr *)(nd_ns + 1);
        nd_opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
        nd_opt->nd_opt_len  = 1;
        bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
    }

    /* checksum (simple placeholder; production needs pseudo-header) */
    nd_ns->nd_ns_cksum = 0;

    bzero(&dst_sa, sizeof(dst_sa));
    dst_sa.sin6_family = AF_INET6;
    dst_sa.sin6_len    = sizeof(dst_sa);
    dst_sa.sin6_addr   = ip6->ip6_dst;

    ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * nd6_ns_input - process an inbound Neighbor Solicitation.
 * ------------------------------------------------------------------ */
void
nd6_ns_input(struct mbuf *m, int off, int icmp6len)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct nd_neighbor_solicit *nd_ns;
    struct in6_addr tgt;
    struct in6_ifaddr *ia;
    struct ifnet *ifp = m->m_pkthdr.rcvif;

    if (m->m_len < off + (int)sizeof(*nd_ns) &&
        (m = m_pullup(m, off + sizeof(*nd_ns))) == NULL)
        return;

    ip6   = mtod(m, struct ip6_hdr *);
    nd_ns = (struct nd_neighbor_solicit *)(mtod(m, u_int8_t *) + off);
    tgt   = nd_ns->nd_ns_target;

    /* is target one of our addresses? */
    for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if (ia->ia6_ifp == ifp &&
            IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, &tgt))
            goto found;
    }
    m_freem(m);
    return;

found:
    /* send a Neighbor Advertisement in response */
    nd6_na_output(ifp, &ip6->ip6_src, &tgt,
                  ND_NA_FLAG_SOLICITED | ND_NA_FLAG_OVERRIDE, 1,
                  (struct sockaddr *)NULL);
    m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_na_output - send a Neighbor Advertisement.
 *
 * dst     - destination (unicast or all-nodes multicast)
 * tgt     - target address
 * flags   - ND_NA_FLAG_* (SOLICITED, ROUTER, OVERRIDE)
 * tlladdr - include TLLA option
 * ------------------------------------------------------------------ */
void
nd6_na_output(struct ifnet *ifp, struct in6_addr *dst,
              struct in6_addr *tgt, u_long flags, int tlladdr,
              struct sockaddr *sdl0)
{
    struct mbuf *m;
    struct ip6_hdr *ip6;
    struct nd_neighbor_advert *nd_na;
    struct nd_opt_hdr *nd_opt;
    caddr_t mac;
    int icmp6len = sizeof(*nd_na);
    int optlen   = 0;
    int hlen     = sizeof(struct ip6_hdr);
    int pktlen;

    mac = nd6_ifptomac(ifp);
    if (tlladdr && mac && ifp->if_addrlen == 6)
        optlen = 8;

    pktlen = hlen + icmp6len + optlen;

    m = m_gethdr(M_DONTWAIT, MT_DATA);
    if (m == NULL)
        return;
    m->m_pkthdr.len = m->m_len = pktlen;

    ip6   = mtod(m, struct ip6_hdr *);
    nd_na = (struct nd_neighbor_advert *)(ip6 + 1);

    bzero(ip6, sizeof(*ip6));
    ip6->ip6_vfc  = IPV6_VERSION;
    ip6->ip6_nxt  = IPPROTO_ICMPV6;
    ip6->ip6_hlim = 255;
    ip6->ip6_plen = htons(icmp6len + optlen);
    ip6->ip6_dst  = *dst;

    /* source: link-local of ifp */
    {
        struct in6_ifaddr *ia = in6_ifaof_ifpforlinklocal(ifp);
        if (ia)
            ip6->ip6_src = ia->ia_addr.sin6_addr;
    }

    bzero(nd_na, sizeof(*nd_na));
    nd_na->nd_na_type   = ND_NEIGHBOR_ADVERT;
    nd_na->nd_na_flags_reserved = htonl(flags);
    nd_na->nd_na_target = *tgt;

    if (optlen) {
        nd_opt = (struct nd_opt_hdr *)(nd_na + 1);
        nd_opt->nd_opt_type = ND_OPT_TARGET_LINKADDR;
        nd_opt->nd_opt_len  = 1;
        bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
    }

    ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * nd6_na_input - process an inbound Neighbor Advertisement.
 * ------------------------------------------------------------------ */
void
nd6_na_input(struct mbuf *m, int off, int icmp6len)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct nd_neighbor_advert *nd_na;
    struct in6_addr tgt;
    struct rtentry *rt;
    struct llinfo_nd6 *ln;
    struct sockaddr_dl *sdl;
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    struct mbuf *held;

    if (m->m_len < off + (int)sizeof(*nd_na) &&
        (m = m_pullup(m, off + sizeof(*nd_na))) == NULL)
        return;

    ip6   = mtod(m, struct ip6_hdr *);
    nd_na = (struct nd_neighbor_advert *)(mtod(m, u_int8_t *) + off);
    tgt   = nd_na->nd_na_target;

    rt = nd6_lookup(&tgt, 0, ifp);
    if (rt == NULL || (ln = (struct llinfo_nd6 *)rt->rt_llinfo) == NULL) {
        m_freem(m);
        return;
    }

    /* extract TLLA option and update neighbor cache */
    if (off + (int)sizeof(*nd_na) + 8 <= m->m_len) {
        struct nd_opt_hdr *nd_opt =
            (struct nd_opt_hdr *)(nd_na + 1);
        if (nd_opt->nd_opt_type == ND_OPT_TARGET_LINKADDR &&
            nd_opt->nd_opt_len == 1) {
            sdl = (struct sockaddr_dl *)rt->rt_gateway;
            if (sdl && sdl->sdl_family == AF_LINK) {
                bcopy((caddr_t)(nd_opt + 1), LLADDR(sdl),
                      ifp->if_addrlen);
                sdl->sdl_alen = ifp->if_addrlen;
            }
        }
    }

    ln->ln_state  = ND6_LLINFO_REACHABLE;
    {
        struct timeval _tv; GetSysTime(&_tv);
        ln->ln_expire = _tv.tv_sec + ND6_REACHABLE_TIME / 1000;
    }

    /* flush queued packet */
    held = ln->ln_hold;
    ln->ln_hold = NULL;
    if (held) {
        struct sockaddr_in6 dst_sa;
        u_char llbuf[32];
        dst_sa.sin6_family = AF_INET6;
        dst_sa.sin6_len    = sizeof(dst_sa);
        dst_sa.sin6_addr   = tgt;
        if (nd6_resolve(ifp, rt, held,
                        (struct sockaddr *)&dst_sa, llbuf) == 0)
            (*ifp->if_output)(ifp, held,
                              (struct sockaddr *)&dst_sa, rt);
    }

    m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_rs_input - process Router Solicitation (host behaviour: ignore).
 * ------------------------------------------------------------------ */
void
nd6_rs_input(struct mbuf *m, int off, int icmp6len)
{
    m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_ra_input - process Router Advertisement.
 *
 * Extracts default gateway and prefix information.
 * ------------------------------------------------------------------ */
void
nd6_ra_input(struct mbuf *m, int off, int icmp6len)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct nd_router_advert *nd_ra;
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    u_int8_t *opts, *end;
    int optlen;

    if (m->m_len < off + (int)sizeof(*nd_ra) &&
        (m = m_pullup(m, off + sizeof(*nd_ra))) == NULL)
        return;

    ip6   = mtod(m, struct ip6_hdr *);
    nd_ra = (struct nd_router_advert *)(mtod(m, u_int8_t *) + off);

    /* update hop limit if provided */
    if (nd_ra->nd_ra_curhoplimit)
        ip6_defhlim = nd_ra->nd_ra_curhoplimit;

    /* process options */
    opts = (u_int8_t *)(nd_ra + 1);
    end  = mtod(m, u_int8_t *) + m->m_len;

    while (opts + 2 <= end) {
        u_int8_t otype = opts[0];
        u_int8_t olen  = opts[1];

        if (olen == 0) break;
        optlen = olen * 8;
        if (opts + optlen > end) break;

        if (otype == ND_OPT_PREFIX_INFORMATION && optlen >= 32) {
            struct nd_opt_prefix_info *pi =
                (struct nd_opt_prefix_info *)opts;
            /* stateless address autoconfiguration (SLAAC) */
            if ((pi->nd_opt_pi_flags_reserved & ND_OPT_PI_FLAG_AUTO) &&
                pi->nd_opt_pi_prefix_len == 64) {
                struct in6_ifaddr *ia;
                struct sockaddr_in6 sin6;

                /* check we don't already have this prefix */
                bzero(&sin6, sizeof(sin6));
                sin6.sin6_family = AF_INET6;
                sin6.sin6_len    = sizeof(sin6);

                /* build address: prefix + EUI-64 from link-local */
                ia = in6_ifaof_ifpforlinklocal(ifp);
                if (ia) {
                    bcopy(&pi->nd_opt_pi_prefix,
                          &sin6.sin6_addr, 8);
                    bcopy(&ia->ia_addr.sin6_addr.s6_addr[8],
                          &sin6.sin6_addr.s6_addr[8], 8);

                    /* add address to interface */
                    {
                        struct in6_aliasreq ifra;
                        bzero(&ifra, sizeof(ifra));
                        bcopy(ifp->if_name, ifra.ifra_name,
                              sizeof(ifra.ifra_name));
                        ifra.ifra_addr    = sin6;
                        ifra.ifra_flags   = IN6_IFF_AUTOCONF;
                        ifra.ifra_lifetime.ia6t_vltime =
                            ntohl(pi->nd_opt_pi_valid_time);
                        ifra.ifra_lifetime.ia6t_pltime =
                            ntohl(pi->nd_opt_pi_preferred_time);
                        in6_control(NULL, SIOCAIFADDR_IN6,
                                    (caddr_t)&ifra, ifp);
                    }
                }
            }
        }
        opts += optlen;
    }

    m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_timer - periodic ND6 timer (called from amiga_main.c slowtimo).
 * ------------------------------------------------------------------ */
void
nd6_timer(void *arg)
{
    struct llinfo_nd6 *ln, *nln;
    struct timeval _tv;
    long now;
    GetSysTime(&_tv);
    now = _tv.tv_sec;

    for (ln = llinfo_nd6.ln_next; ln != &llinfo_nd6; ln = nln) {
        nln = ln->ln_next;

        switch (ln->ln_state) {
        case ND6_LLINFO_INCOMPLETE:
            if (ln->ln_asked >= ND6_MAX_MULTICAST_SOLICIT) {
                /* unreachable: free held packet */
                if (ln->ln_hold) {
                    m_freem(ln->ln_hold);
                    ln->ln_hold = NULL;
                }
                ln->ln_state = ND6_LLINFO_NOSTATE;
            }
            break;

        case ND6_LLINFO_REACHABLE:
            if (ln->ln_expire && now > ln->ln_expire)
                ln->ln_state = ND6_LLINFO_STALE;
            break;

        case ND6_LLINFO_DELAY:
            ln->ln_state = ND6_LLINFO_PROBE;
            ln->ln_asked = 0;
            /* FALLTHROUGH */
        case ND6_LLINFO_PROBE:
            if (ln->ln_asked >= ND6_MAX_UNICAST_SOLICIT) {
                /* NUD failed: evict entry */
                ln->ln_state = ND6_LLINFO_NOSTATE;
            } else {
                struct sockaddr_in6 dst;
                bzero(&dst, sizeof(dst));
                dst.sin6_family = AF_INET6;
                dst.sin6_len    = sizeof(dst);
                if (ln->ln_rt && ln->ln_rt->rt_ifp) {
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
}

#endif /* INET6 */
