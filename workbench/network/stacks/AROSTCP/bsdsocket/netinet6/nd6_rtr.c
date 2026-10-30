/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME nd6_rtr.c:
 * Copyright (C) 1995-1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * nd6_rtr.c - IPv6 Router Discovery.
 *
 * Implements:
 *   - Router Solicitation output (RFC 4861 §6.3.7)
 *   - Router Advertisement input (RFC 4861 §6.3.4)
 *   - Default Router List management
 *   - Prefix List management
 *   - Redirect input (RFC 4861 §8.3)
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

/* Forward declarations */
int ip6_output(void *, ...);

#include <protos/netinet/in_cksum_protos.h>

/* Default router list (singly-linked) */
static struct nd_defrouter *nd6_defrtrlist = NULL;

/* Prefix list (singly-linked) */
static struct nd_prefix *nd6_prefixlist = NULL;

/* ==================================================================
 * Router preferences and routes (RFC 4191)
 * ================================================================== */

/*
 * The preference in an RA's flags or a Route Information Option's flags,
 * both in bits 3-4 (RFC 4191 section 2.1): 1 high, 0 medium, -1 low. The
 * reserved value is not handled here: an RA header counts it as medium,
 * and a Route Information Option that carries it is ignored (sections 2.2
 * and 2.3).
 */
static int
nd6_rtpref(u_int8_t flags)
{
    switch(flags & ND_RA_FLAG_RTPREF_MASK) {
    case ND_RA_FLAG_RTPREF_HIGH:
        return 1;
    case ND_RA_FLAG_RTPREF_LOW:
        return -1;
    default:
        return 0;
    }
}

/* Expiry time for a lifetime in seconds; 0 means it never expires. */
static long
nd6_rtexpire(u_int32_t lifetime)
{
    struct timeval _tv;

    if(lifetime == 0xffffffff)
        return 0;
    GetSysTime(&_tv);
    if(lifetime > (u_int32_t)(0x7fffffff - _tv.tv_sec))
        return 0;
    return _tv.tv_sec + (long)lifetime;
}

static int
nd6_rtexpired(long expire)
{
    struct timeval _tv;

    if(expire == 0)
        return 0;
    GetSysTime(&_tv);
    return expire < _tv.tv_sec;
}

/*
 * Add or delete the route to prefix/plen through the router gw on ifp. The
 * netmask is always given: for plen 0, the default route, a route to ::
 * without one would be a host route to the unspecified address, which the
 * routing table refuses (its key is the tree's own all-zeros node), rather
 * than the route every destination falls back to; as for a GW6 default
 * route from the interface database (kern/amiga_netdb.c). The route is put
 * on ifp, the interface the router advertised on: the routing table holds
 * fe80::/64 once, so the link-local gateway would otherwise resolve to
 * whichever interface registered that prefix first.
 */
static int
nd6_router_route(int cmd, struct in6_addr *prefix, int plen,
                 struct in6_addr *gw, struct ifnet *ifp)
{
    struct sockaddr_in6 dst, gate, mask;
    struct rtentry *rt = NULL;
    int error, i, bits;

    bzero(&dst, sizeof(dst));
    dst.sin6_family = AF_INET6;
    dst.sin6_len    = sizeof(dst);
    dst.sin6_addr   = *prefix;
    bzero(&gate, sizeof(gate));
    gate.sin6_family = AF_INET6;
    gate.sin6_len    = sizeof(gate);
    gate.sin6_addr   = *gw;
    bzero(&mask, sizeof(mask));
    mask.sin6_family = AF_INET6;
    mask.sin6_len    = sizeof(mask);
    for(i = 0, bits = plen; i < 16 && bits >= 8; i++, bits -= 8)
        mask.sin6_addr.s6_addr[i] = 0xff;
    if(i < 16 && bits > 0)
        mask.sin6_addr.s6_addr[i] = (0xff << (8 - bits)) & 0xff;

    error = rtrequest(cmd, (struct sockaddr *)&dst, (struct sockaddr *)&gate,
                      (struct sockaddr *)&mask, RTF_GATEWAY | RTF_DYNAMIC,
                      cmd == RTM_ADD ? &rt : NULL);
    if(error == 0 && rt != NULL) {
        if(rt->rt_ifp != ifp) {
            struct in6_ifaddr *ia = in6_ifaof_ifpforlinklocal(ifp);

            rt->rt_ifp = ifp;
            if(ia)
                rt->rt_ifa = &ia->ia_ifa;
        }
        rt->rt_refcnt--;
    }
    return error;
}

/* Routes learned from Route Information Options, one per (prefix, router) */
static struct nd_route_info *nd6_rtinfolist = NULL;

/* ==================================================================
 * Default Router List Management
 * ================================================================== */

/* ------------------------------------------------------------------
 * defrouter_lookup - find a default router by address and interface.
 * ------------------------------------------------------------------ */
struct nd_defrouter *
defrouter_lookup(struct in6_addr *addr, struct ifnet *ifp)
{
    struct nd_defrouter *dr;

    for(dr = nd6_defrtrlist; dr; dr = dr->next) {
        if(dr->ifp == ifp &&
                IN6_ARE_ADDR_EQUAL(&dr->rtaddr, addr))
            return dr;
    }
    return NULL;
}

/* ------------------------------------------------------------------
 * defrouter_insert - add or update a default router entry, with its
 * lifetime in seconds (0xffffffff, infinite, can come from a ::/0 Route
 * Information Option) and its RFC 4191 preference.
 * ------------------------------------------------------------------ */
static struct nd_defrouter *
defrouter_insert(struct in6_addr *addr, struct ifnet *ifp,
                 u_int32_t lifetime, u_int8_t raflags, int pref)
{
    struct nd_defrouter *dr, **pp;
    int count = 0;

    dr = defrouter_lookup(addr, ifp);
    if(dr) {
        /* update existing entry */
        dr->rtlifetime = lifetime;
        dr->flags  = raflags;
        dr->pref   = pref;
        dr->expire = nd6_rtexpire(lifetime);
        return dr;
    }

    for(dr = nd6_defrtrlist; dr; dr = dr->next)
        count++;
    if(count >= ND6_MAX_DEFROUTERS)
        return NULL;	/* too many */

    MALLOC(dr, struct nd_defrouter *, sizeof(*dr), M_PCB, M_NOWAIT);
    if(dr == NULL)
        return NULL;
    bzero(dr, sizeof(*dr));
    dr->rtaddr     = *addr;
    dr->ifp        = ifp;
    dr->rtlifetime = lifetime;
    dr->flags      = raflags;
    dr->pref       = pref;
    dr->expire     = nd6_rtexpire(lifetime);
    dr->installed  = 0;

    /* append, so that among equals the router heard first stays first */
    for(pp = &nd6_defrtrlist; *pp; pp = &(*pp)->next)
        ;
    *pp = dr;
    return dr;
}

/* ------------------------------------------------------------------
 * defrouter_del - remove a default router entry.
 * ------------------------------------------------------------------ */
static void
defrouter_del(struct nd_defrouter *dr)
{
    struct nd_defrouter **pp;

    /* remove route if installed */
    if(dr->installed) {
        struct in6_addr any;

        bzero(&any, sizeof(any));
        nd6_router_route(RTM_DELETE, &any, 0, &dr->rtaddr, dr->ifp);
        dr->installed = 0;
    }

    /* unlink from list */
    for(pp = &nd6_defrtrlist; *pp; pp = &(*pp)->next) {
        if(*pp == dr) {
            *pp = dr->next;
            break;
        }
    }
    FREE(dr, M_PCB);
}

/* ------------------------------------------------------------------
 * defrouter_select - install the best default router as the default
 * route: the most preferred one that has not expired (RFC 4191 section
 * 3.2), the one heard first among equals.
 * ------------------------------------------------------------------ */
void
defrouter_select(void)
{
    struct nd_defrouter *dr, *best = NULL;
    struct in6_addr any;

    for(dr = nd6_defrtrlist; dr; dr = dr->next) {
        if(nd6_rtexpired(dr->expire) || dr->rtlifetime == 0)
            continue;
        if(best == NULL || dr->pref > best->pref)
            best = dr;
    }

    bzero(&any, sizeof(any));
    /* remove any currently installed default routes */
    for(dr = nd6_defrtrlist; dr; dr = dr->next) {
        if(dr->installed && dr != best) {
            nd6_router_route(RTM_DELETE, &any, 0, &dr->rtaddr, dr->ifp);
            dr->installed = 0;
        }
    }

    /* install the selected router */
    if(best && !best->installed &&
            nd6_router_route(RTM_ADD, &any, 0, &best->rtaddr, best->ifp) == 0)
        best->installed = 1;
}

/* ==================================================================
 * More-specific routes (RFC 4191 Route Information Options)
 * ================================================================== */

/*
 * rtinfo_select - for prefix/plen, install the route through the most
 * preferred router that advertises it and has not expired, the one heard
 * first among equals, and remove the route through any other. Between
 * prefixes of different lengths the routing table's longest match decides
 * (section 3.2).
 */
static void
rtinfo_select(struct in6_addr *prefix, int plen)
{
    struct nd_route_info *ri, *best = NULL;

    for(ri = nd6_rtinfolist; ri; ri = ri->next) {
        if(ri->plen != plen || !IN6_ARE_ADDR_EQUAL(&ri->prefix, prefix))
            continue;
        if(nd6_rtexpired(ri->expire))
            continue;
        if(best == NULL || ri->pref > best->pref)
            best = ri;
    }
    for(ri = nd6_rtinfolist; ri; ri = ri->next) {
        if(ri->plen != plen || !IN6_ARE_ADDR_EQUAL(&ri->prefix, prefix))
            continue;
        if(ri->installed && ri != best) {
            nd6_router_route(RTM_DELETE, &ri->prefix, ri->plen, &ri->rtaddr, ri->ifp);
            ri->installed = 0;
        }
    }
    if(best && !best->installed &&
            nd6_router_route(RTM_ADD, &best->prefix, best->plen,
                             &best->rtaddr, best->ifp) == 0)
        best->installed = 1;
}

static void
rtinfo_del(struct nd_route_info *ri)
{
    struct nd_route_info **pp;

    if(ri->installed) {
        nd6_router_route(RTM_DELETE, &ri->prefix, ri->plen, &ri->rtaddr, ri->ifp);
        ri->installed = 0;
    }
    for(pp = &nd6_rtinfolist; *pp; pp = &(*pp)->next) {
        if(*pp == ri) {
            *pp = ri->next;
            break;
        }
    }
    FREE(ri, M_PCB);
}

/*
 * rtinfo_update - a router on ifp advertised prefix/plen with this
 * preference and lifetime (RFC 4191 section 3.1): add or refresh the
 * (prefix, router) entry, or remove it for lifetime 0; then reselect.
 */
static void
rtinfo_update(struct in6_addr *prefix, int plen, struct in6_addr *router,
              struct ifnet *ifp, int pref, u_int32_t lifetime)
{
    struct nd_route_info *ri, **pp;
    int count = 0;

    for(ri = nd6_rtinfolist; ri; ri = ri->next) {
        if(ri->ifp == ifp && ri->plen == plen &&
                IN6_ARE_ADDR_EQUAL(&ri->prefix, prefix) &&
                IN6_ARE_ADDR_EQUAL(&ri->rtaddr, router))
            break;
        count++;
    }
    if(ri == NULL) {
        if(lifetime == 0 || count >= ND6_MAX_RTINFO)
            return;
        MALLOC(ri, struct nd_route_info *, sizeof(*ri), M_PCB, M_NOWAIT);
        if(ri == NULL)
            return;
        bzero(ri, sizeof(*ri));
        ri->prefix = *prefix;
        ri->plen   = plen;
        ri->rtaddr = *router;
        ri->ifp    = ifp;
        for(pp = &nd6_rtinfolist; *pp; pp = &(*pp)->next)
            ;
        *pp = ri;
    } else if(lifetime == 0) {
        rtinfo_del(ri);
        rtinfo_select(prefix, plen);
        return;
    }
    ri->pref   = pref;
    ri->expire = nd6_rtexpire(lifetime);
    rtinfo_select(prefix, plen);
}

/*
 * nd6_rtinfo_input - a Route Information Option, other than ::/0, of an RA
 * from router on ifp (RFC 4191 section 2.3). A malformed option, or one
 * with the reserved preference, is ignored.
 */
static void
nd6_rtinfo_input(struct nd_opt_route_info *rio, int optlen,
                 struct in6_addr *router, struct ifnet *ifp)
{
    struct in6_addr prefix;
    int plen = rio->nd_opt_rti_prefixlen, len = rio->nd_opt_rti_len;
    int i, bits, n;
    u_int32_t lifetime;

    /* the option length must fit the prefix length (section 2.3) */
    if(plen > 128 || len < 1 || len > 3 ||
            (plen > 64 && len != 3) || (plen > 0 && len < 2))
        return;
    if((rio->nd_opt_rti_flags & ND_RA_FLAG_RTPREF_MASK) == ND_RA_FLAG_RTPREF_RSV)
        return;

    /* the prefix field holds (len - 1) * 8 bytes; bits past plen are ignored */
    bzero(&prefix, sizeof(prefix));
    n = (len - 1) * 8;
    if(n > optlen - 8)
        n = optlen - 8;
    bcopy((u_int8_t *)(rio + 1), &prefix, n);
    for(i = 0, bits = plen; i < 16; i++, bits -= 8) {
        if(bits >= 8)
            continue;
        prefix.s6_addr[i] &= (bits > 0) ? (0xff << (8 - bits)) & 0xff : 0;
    }
    /*
     * A route through a router to a link-local or multicast prefix would
     * take traffic that never leaves the link, or a group's scope, off it.
     */
    if((plen >= 10 && IN6_IS_ADDR_LINKLOCAL(&prefix)) ||
            (plen >= 8 && IN6_IS_ADDR_MULTICAST(&prefix)))
        return;

    bcopy(&rio->nd_opt_rti_lifetime, &lifetime, sizeof(lifetime));
    rtinfo_update(&prefix, plen, router, ifp,
                  nd6_rtpref(rio->nd_opt_rti_flags), ntohl(lifetime));
}

/* ------------------------------------------------------------------
 * nd6_defrouter_timer - expire default routers and learned routes
 * (called from nd6_timer).
 * ------------------------------------------------------------------ */
void
nd6_defrouter_timer(void)
{
    struct nd_defrouter *dr, *next;
    struct nd_route_info *ri;
    int changed = 0;

    for(dr = nd6_defrtrlist; dr; dr = next) {
        next = dr->next;
        if(nd6_rtexpired(dr->expire)) {
            defrouter_del(dr);
            changed = 1;
        }
    }
    if(changed)
        defrouter_select();

    /* rtinfo_select() may install another entry: start over after each */
    for(ri = nd6_rtinfolist; ri; ) {
        if(nd6_rtexpired(ri->expire)) {
            struct in6_addr prefix = ri->prefix;
            int plen = ri->plen;

            rtinfo_del(ri);
            rtinfo_select(&prefix, plen);
            ri = nd6_rtinfolist;
        } else
            ri = ri->next;
    }
}

/*
 * nd6_purge_routers - the interface is being deconfigured or going away:
 * forget the default routers heard on it and the routes they gave, with
 * their routing table entries, as FreeBSD's nd6_purge() does. An entry left
 * behind would keep a pointer to the interface, and, marked installed after
 * rt_purgeif() took its route, would never be put back when the router
 * advertises again. Another interface's routers take over meanwhile.
 */
void
nd6_purge_routers(struct ifnet *ifp)
{
    struct nd_defrouter *dr, *next;
    struct nd_route_info *ri;

    for(dr = nd6_defrtrlist; dr; dr = next) {
        next = dr->next;
        if(dr->ifp == ifp)
            defrouter_del(dr);
    }
    defrouter_select();

    for(ri = nd6_rtinfolist; ri; ) {
        if(ri->ifp == ifp) {
            struct in6_addr prefix = ri->prefix;
            int plen = ri->plen;

            rtinfo_del(ri);
            rtinfo_select(&prefix, plen);
            ri = nd6_rtinfolist;
        } else
            ri = ri->next;
    }
}

/* ==================================================================
 * Prefix List Management
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_prefix_lookup - find a prefix entry.
 * ------------------------------------------------------------------ */
struct nd_prefix *
nd6_prefix_lookup(struct sockaddr_in6 *prefix, int plen)
{
    struct nd_prefix *pr;

    for(pr = nd6_prefixlist; pr; pr = pr->next) {
        if(pr->ndpr_plen == plen &&
                IN6_ARE_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
                                   &prefix->sin6_addr))
            return pr;
    }
    return NULL;
}

/* ------------------------------------------------------------------
 * nd6_prefix_add - add or update a prefix list entry.
 * ------------------------------------------------------------------ */
static struct nd_prefix *
nd6_prefix_add(struct ifnet *ifp, struct nd_opt_prefix_info *pi)
{
    struct nd_prefix *pr;
    struct sockaddr_in6 prefix;
    struct timeval _tv;
    int plen = pi->nd_opt_pi_prefix_len;

    bzero(&prefix, sizeof(prefix));
    prefix.sin6_family = AF_INET6;
    prefix.sin6_len    = sizeof(prefix);
    prefix.sin6_addr   = pi->nd_opt_pi_prefix;

    GetSysTime(&_tv);

    pr = nd6_prefix_lookup(&prefix, plen);
    if(pr) {
        /* update lifetimes */
        pr->ndpr_vltime = ntohl(pi->nd_opt_pi_valid_time);
        pr->ndpr_pltime = ntohl(pi->nd_opt_pi_preferred_time);
        if(pr->ndpr_vltime == 0xffffffff)
            pr->ndpr_expire = 0;	/* infinite */
        else
            pr->ndpr_expire = _tv.tv_sec + pr->ndpr_vltime;
        if(pr->ndpr_pltime == 0xffffffff)
            pr->ndpr_preferred = 0;
        else
            pr->ndpr_preferred = _tv.tv_sec + pr->ndpr_pltime;
        pr->ndpr_flags = pi->nd_opt_pi_flags_reserved;
        return pr;
    }

    /* count existing */
    {
        int count = 0;
        for(pr = nd6_prefixlist; pr; pr = pr->next)
            count++;
        if(count >= ND6_MAX_PREFIXES)
            return NULL;
    }

    MALLOC(pr, struct nd_prefix *, sizeof(*pr), M_PCB, M_NOWAIT);
    if(pr == NULL)
        return NULL;
    bzero(pr, sizeof(*pr));

    pr->ndpr_ifp    = ifp;
    pr->ndpr_prefix = prefix;
    pr->ndpr_plen   = plen;
    pr->ndpr_vltime = ntohl(pi->nd_opt_pi_valid_time);
    pr->ndpr_pltime = ntohl(pi->nd_opt_pi_preferred_time);
    pr->ndpr_flags  = pi->nd_opt_pi_flags_reserved;
    pr->ndpr_refcount = 0;
    pr->ndpr_onlink = 0;
    pr->ndpr_autoconf = 0;

    if(pr->ndpr_vltime == 0xffffffff)
        pr->ndpr_expire = 0;
    else
        pr->ndpr_expire = _tv.tv_sec + pr->ndpr_vltime;
    if(pr->ndpr_pltime == 0xffffffff)
        pr->ndpr_preferred = 0;
    else
        pr->ndpr_preferred = _tv.tv_sec + pr->ndpr_pltime;

    /* insert at head */
    pr->next = nd6_prefixlist;
    nd6_prefixlist = pr;

    return pr;
}

/* ------------------------------------------------------------------
 * nd6_prefix_onlink - install an on-link route for a prefix.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_onlink(struct nd_prefix *pr)
{
    struct sockaddr_in6 prefix, mask;
    struct ifaddr *ifa;
    int i, plen;

    if(pr->ndpr_onlink)
        return;

    /*
     * An interface route needs the interface's own address as its
     * gateway. Without one the prefix cannot be made on-link yet -
     * handing rtrequest() a null gateway takes the whole stack down
     * inside ifa_ifwithroute().
     */
    for(ifa = pr->ndpr_ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
        if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6)
            break;
    if(ifa == NULL)
        return;

    bzero(&prefix, sizeof(prefix));
    prefix.sin6_family = AF_INET6;
    prefix.sin6_len    = sizeof(prefix);
    prefix.sin6_addr   = pr->ndpr_prefix.sin6_addr;

    /* build netmask */
    bzero(&mask, sizeof(mask));
    mask.sin6_family = AF_INET6;
    mask.sin6_len    = sizeof(mask);
    plen = pr->ndpr_plen;
    for(i = 0; i < 16 && plen >= 8; i++, plen -= 8)
        mask.sin6_addr.s6_addr[i] = 0xff;
    if(plen > 0)
        mask.sin6_addr.s6_addr[i] = (0xff << (8 - plen)) & 0xff;

    /* mask the prefix */
    for(i = 0; i < 16; i++)
        prefix.sin6_addr.s6_addr[i] &= mask.sin6_addr.s6_addr[i];

    if(rtrequest(RTM_ADD, (struct sockaddr *)&prefix,
                 ifa->ifa_addr, (struct sockaddr *)&mask,
                 RTF_UP | RTF_CLONING, NULL) == 0) {
        pr->ndpr_onlink = 1;
    }
}

/* ------------------------------------------------------------------
 * nd6_prefix_offlink - remove on-link route for a prefix.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_offlink(struct nd_prefix *pr)
{
    struct sockaddr_in6 prefix, mask;
    int i, plen;

    if(!pr->ndpr_onlink)
        return;

    bzero(&prefix, sizeof(prefix));
    prefix.sin6_family = AF_INET6;
    prefix.sin6_len    = sizeof(prefix);
    prefix.sin6_addr   = pr->ndpr_prefix.sin6_addr;

    bzero(&mask, sizeof(mask));
    mask.sin6_family = AF_INET6;
    mask.sin6_len    = sizeof(mask);
    plen = pr->ndpr_plen;
    for(i = 0; i < 16 && plen >= 8; i++, plen -= 8)
        mask.sin6_addr.s6_addr[i] = 0xff;
    if(plen > 0)
        mask.sin6_addr.s6_addr[i] = (0xff << (8 - plen)) & 0xff;

    for(i = 0; i < 16; i++)
        prefix.sin6_addr.s6_addr[i] &= mask.sin6_addr.s6_addr[i];

    rtrequest(RTM_DELETE, (struct sockaddr *)&prefix,
              NULL, (struct sockaddr *)&mask, RTF_UP | RTF_CLONING, NULL);
    pr->ndpr_onlink = 0;
}

/* ------------------------------------------------------------------
 * nd6_prefix_timer - expire prefixes (called from nd6_timer).
 * ------------------------------------------------------------------ */
void
nd6_prefix_timer(void)
{
    struct nd_prefix *pr, **pp;
    struct timeval _tv;

    GetSysTime(&_tv);

    pp = &nd6_prefixlist;
    while((pr = *pp) != NULL) {
        if(pr->ndpr_expire != 0 && pr->ndpr_expire < _tv.tv_sec) {
            /* prefix expired */
            nd6_prefix_offlink(pr);
            *pp = pr->next;
            FREE(pr, M_PCB);
            continue;
        }
        pp = &pr->next;
    }
}

/* ==================================================================
 * SLAAC - Stateless Address Autoconfiguration (RFC 4862)
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_prefix_slaac - create a SLAAC address from prefix + EUI-64.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_slaac(struct ifnet *ifp, struct nd_prefix *pr)
{
    struct in6_ifaddr *ia;
    struct in6_aliasreq ifra;
    struct sockaddr_in6 addr;

    if(pr->ndpr_plen != 64)
        return;			/* SLAAC requires /64 */
    if(pr->ndpr_autoconf)
        return;			/* already configured */

    /* get link-local to derive EUI-64 interface identifier */
    ia = in6_ifaof_ifpforlinklocal(ifp);
    if(ia == NULL)
        return;

    /* build global address: prefix + IID from link-local */
    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_len    = sizeof(addr);
    bcopy(&pr->ndpr_prefix.sin6_addr, &addr.sin6_addr, 8);
    bcopy(&ia->ia_addr.sin6_addr.s6_addr[8],
          &addr.sin6_addr.s6_addr[8], 8);

    /* check we don't already have this address */
    {
        struct in6_ifaddr *existing;
        for(existing = in6_ifaddr; existing; existing = existing->ia_next) {
            if(existing->ia6_ifp == ifp &&
                    IN6_ARE_ADDR_EQUAL(&existing->ia_addr.sin6_addr,
                                       &addr.sin6_addr))
                return;	/* already have it */
        }
    }

    /* add the address */
    bzero(&ifra, sizeof(ifra));
    bcopy(ifp->if_name, ifra.ifra_name, sizeof(ifra.ifra_name));
    ifra.ifra_addr = addr;
    ifra.ifra_flags = IN6_IFF_AUTOCONF;
    ifra.ifra_lifetime.ia6t_vltime = pr->ndpr_vltime;
    ifra.ifra_lifetime.ia6t_pltime = pr->ndpr_pltime;

    /* set /64 prefix mask */
    ifra.ifra_prefixmask.sin6_family = AF_INET6;
    ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
    {
        int j;
        for(j = 0; j < 8; j++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[j] = 0xff;
    }

    /* in6_ifinit() joins the address's solicited-node group */
    in6_control(NULL, SIOCAIFADDR_IN6, (caddr_t)&ifra, ifp);

    pr->ndpr_autoconf = 1;
    pr->ndpr_refcount++;
}

/* ==================================================================
 * Router Solicitation Output
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_rs_output - send a Router Solicitation on an interface.
 *
 * Called after DAD completes to discover routers on the link.
 * ------------------------------------------------------------------ */
void
nd6_rs_output(struct ifnet *ifp)
{
    struct mbuf *m;
    struct ip6_hdr *ip6;
    struct nd_router_solicit *rs;
    struct ip6_moptions im6o;
    caddr_t mac;
    int hlen = sizeof(struct ip6_hdr);
    int icmp6len = sizeof(struct nd_router_solicit);
    int optlen = 0;
    int pktlen;
    struct in6_ifaddr *ia;

    D(bug("[AROSTCP:ND6] %s: sending RS on %s%d\n",
          __func__, ifp->if_name, ifp->if_unit));

    /* include SLLA if we have a valid source address */
    ia = in6_ifaof_ifpforlinklocal(ifp);
    mac = nd6_ifptomac(ifp);
    if(ia && mac && ifp->if_addrlen == 6)
        optlen = 8;

    pktlen = hlen + icmp6len + optlen;

    m = m_gethdr(M_DONTWAIT, MT_DATA);
    if(m == NULL)
        return;

    m->m_pkthdr.len = m->m_len = pktlen;
    m->m_pkthdr.rcvif = NULL;

    ip6 = mtod(m, struct ip6_hdr *);
    rs  = (struct nd_router_solicit *)(ip6 + 1);

    /* IPv6 header */
    bzero(ip6, sizeof(*ip6));
    ip6->ip6_vfc  = IPV6_VERSION;
    ip6->ip6_nxt  = IPPROTO_ICMPV6;
    ip6->ip6_hlim = 255;
    ip6->ip6_plen = htons(icmp6len + optlen);

    /* destination: all-routers multicast ff02::2 */
    bzero(&ip6->ip6_dst, sizeof(ip6->ip6_dst));
    ip6->ip6_dst.s6_addr[0]  = 0xff;
    ip6->ip6_dst.s6_addr[1]  = 0x02;
    ip6->ip6_dst.s6_addr[15] = 0x02;

    /* source: link-local or :: if no address yet */
    if(ia)
        ip6->ip6_src = ia->ia_addr.sin6_addr;
    else
        bzero(&ip6->ip6_src, sizeof(ip6->ip6_src));

    /* Router Solicitation */
    bzero(rs, sizeof(*rs));
    rs->nd_rs_type = ND_ROUTER_SOLICIT;

    /* Source Link-Layer Address option */
    if(optlen) {
        struct nd_opt_hdr *nd_opt = (struct nd_opt_hdr *)(rs + 1);
        nd_opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
        nd_opt->nd_opt_len  = 1;
        bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
    }

    /* checksum */
    rs->nd_rs_cksum = 0;
    rs->nd_rs_cksum = in6_cksum(m, IPPROTO_ICMPV6,
                                sizeof(struct ip6_hdr), icmp6len + optlen);

    /* force the solicitation out of the requesting interface */
    bzero(&im6o, sizeof(im6o));
    im6o.im6o_multicast_ifp  = ifp;
    im6o.im6o_multicast_hlim = 255;

    nd6stat.nd6s_snd_rs++;
    ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0, &im6o,
               (struct ifnet **)NULL, (struct inpcb *)NULL);
}

/* ==================================================================
 * Router Solicitation / Advertisement Input
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_rs_input - process an inbound Router Solicitation.
 *
 * As a host, we ignore RS messages (only routers respond to RS).
 * ------------------------------------------------------------------ */
void
nd6_rs_input(struct mbuf *m, int off, int icmp6len)
{
    nd6stat.nd6s_rcv_rs++;
    m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_ra_input - process an inbound Router Advertisement.
 *
 * Implements RFC 4861 §6.3.4:
 *   - Update hop limit, reachable time, retrans timer
 *   - Manage default router list
 *   - Process prefix information options (on-link + SLAAC)
 *   - Process MTU option
 * ------------------------------------------------------------------ */
void
nd6_ra_input(struct mbuf *m, int off, int icmp6len)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct nd_router_advert *nd_ra;
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    struct nd_ifinfo *ndi;
    struct in6_addr src6;
    u_int8_t *opts, *end;
    u_int32_t rtlifetime;
    int rtpref;

    nd6stat.nd6s_rcv_ra++;

    /* copy source address out of packed header */
    memcpy(&src6, &ip6->ip6_src, sizeof(src6));

    /* validate hop limit (RFC 4861 §6.1.2) */
    if(ip6->ip6_hlim != 255) {
        nd6stat.nd6s_rcv_badra++;
        goto bad;
    }

    /* source must be link-local */
    if(!IN6_IS_ADDR_LINKLOCAL(&src6)) {
        nd6stat.nd6s_rcv_badra++;
        goto bad;
    }

    if(m->m_len < off + (int)sizeof(*nd_ra)) {
        if((m = m_pullup(m, off + sizeof(*nd_ra))) == NULL) {
            nd6stat.nd6s_rcv_badra++;
            return;
        }
        ip6 = mtod(m, struct ip6_hdr *);
    }

    nd_ra = (struct nd_router_advert *)(mtod(m, u_int8_t *) + off);

    /* get per-interface ND info */
    ndi = nd6_ifinfo(ifp);

    /* update hop limit (RFC 4861 §6.3.4 step 1) */
    if(nd_ra->nd_ra_curhoplimit) {
        if(ndi)
            ndi->chlim = nd_ra->nd_ra_curhoplimit;
        ip6_defhlim = nd_ra->nd_ra_curhoplimit;
    }

    /* update reachable time (RFC 4861 §6.3.4 step 2) */
    if(ntohl(nd_ra->nd_ra_reachable) != 0 && ndi) {
        ndi->basereachable = ntohl(nd_ra->nd_ra_reachable);
        ndi->reachable = ndi->basereachable;
    }

    /* update retrans timer (RFC 4861 §6.3.4 step 3) */
    if(ntohl(nd_ra->nd_ra_retransmit) != 0 && ndi) {
        ndi->retrans = ntohl(nd_ra->nd_ra_retransmit);
    }

    /*
     * The router as a default router. A router lifetime of 0 means it is not
     * one, and then its preference is ignored; the reserved preference counts
     * as medium (RFC 4191 section 2.2). A ::/0 Route Information Option among
     * the options overrides both (section 3.1), so the list is updated once
     * they have been read.
     */
    rtlifetime = ntohs(nd_ra->nd_ra_router_lifetime);
    rtpref = (rtlifetime == 0) ? 0 : nd6_rtpref(nd_ra->nd_ra_flags_reserved);

    /* update neighbor cache for the router */
    nd6_cache_lladdr(ifp, &src6, NULL, 0,
                     ND_ROUTER_ADVERT, 1);

    /* process options */
    opts = (u_int8_t *)(nd_ra + 1);
    end  = mtod(m, u_int8_t *) + m->m_len;

    while(opts + 2 <= end) {
        u_int8_t otype = opts[0];
        u_int8_t olen  = opts[1];
        int optlen;

        if(olen == 0) break;
        optlen = olen * 8;
        if(opts + optlen > end) break;

        switch(otype) {
        case ND_OPT_SOURCE_LINKADDR:
            /* update neighbor cache with router's link-layer addr */
            if(optlen >= 8) {
                char *lladdr = (char *)(opts + 2);
                nd6_cache_lladdr(ifp, &src6,
                                 lladdr, ifp->if_addrlen,
                                 ND_ROUTER_ADVERT, 1);
            }
            break;

        case ND_OPT_PREFIX_INFORMATION:
            if(optlen >= (int)sizeof(struct nd_opt_prefix_info)) {
                struct nd_opt_prefix_info *pi =
                    (struct nd_opt_prefix_info *)opts;
                struct nd_prefix *pr;

                /* skip link-local prefixes */
                if(IN6_IS_ADDR_LINKLOCAL(&pi->nd_opt_pi_prefix))
                    break;

                /* add/update prefix entry */
                pr = nd6_prefix_add(ifp, pi);
                if(pr == NULL)
                    break;

                /* on-link determination */
                if(pi->nd_opt_pi_flags_reserved &
                        ND_OPT_PI_FLAG_ONLINK)
                    nd6_prefix_onlink(pr);

                /* SLAAC */
                if((pi->nd_opt_pi_flags_reserved &
                        ND_OPT_PI_FLAG_AUTO) &&
                        pi->nd_opt_pi_prefix_len == 64)
                    nd6_prefix_slaac(ifp, pr);

                /* RFC 4941: also create temporary address for privacy */
                if(ip6_use_tempaddr &&
                        (pi->nd_opt_pi_flags_reserved &
                        ND_OPT_PI_FLAG_AUTO) &&
                        pi->nd_opt_pi_prefix_len == 64) {
                    if(in6_tmpifadd(ifp, pr) == EEXIST) {
                        /* collision, retry once */
                        (void)in6_tmpifadd(ifp, pr);
                    }
                }
            }
            break;

        case ND_OPT_MTU:
            if(optlen >= (int)sizeof(struct nd_opt_mtu) && ndi) {
                struct nd_opt_mtu *mtu_opt =
                    (struct nd_opt_mtu *)opts;
                u_int32_t mtu = ntohl(mtu_opt->nd_opt_mtu_mtu);
                if(mtu >= IPV6_MMTU && mtu <= ifp->if_mtu) {
                    ndi->linkmtu = mtu;
                }
            }
            break;

        case ND_OPT_ROUTE_INFO:
            if(optlen >= (int)sizeof(struct nd_opt_route_info)) {
                struct nd_opt_route_info *rio = (struct nd_opt_route_info *)opts;

                if(rio->nd_opt_rti_prefixlen != 0)
                    nd6_rtinfo_input(rio, optlen, &src6, ifp);
                else if(rio->nd_opt_rti_len <= 3 &&
                        (rio->nd_opt_rti_flags & ND_RA_FLAG_RTPREF_MASK) !=
                        ND_RA_FLAG_RTPREF_RSV) {
                    u_int32_t lt;

                    /* ::/0 is the router's default route */
                    bcopy(&rio->nd_opt_rti_lifetime, &lt, sizeof(lt));
                    rtlifetime = ntohl(lt);
                    rtpref = nd6_rtpref(rio->nd_opt_rti_flags);
                }
            }
            break;

        default:
            /* ignore unknown options */
            break;
        }

        opts += optlen;
    }

    {
        struct nd_defrouter *dr = defrouter_lookup(&src6, ifp);

        if(rtlifetime == 0) {
            if(dr) {
                defrouter_del(dr);
                defrouter_select();
            }
        } else if(defrouter_insert(&src6, ifp, rtlifetime,
                                   nd_ra->nd_ra_flags_reserved, rtpref) != NULL)
            defrouter_select();
    }

    m_freem(m);
    return;

bad:
    m_freem(m);
}

/* ==================================================================
 * Redirect Message Input (RFC 4861 §8.3)
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_redirect_input - process an inbound Redirect message.
 *
 * Updates the routing table to use the better next-hop.
 * ------------------------------------------------------------------ */
void
nd6_redirect_input(struct mbuf *m, int off, int icmp6len)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct nd_redirect *rd;
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    struct in6_addr src6, tgt, reddst;

    nd6stat.nd6s_rcv_redirect++;

    /* copy source address out of packed header */
    memcpy(&src6, &ip6->ip6_src, sizeof(src6));

    /* validate hop limit */
    if(ip6->ip6_hlim != 255) {
        nd6stat.nd6s_rcv_badredirect++;
        goto bad;
    }

    /* source must be link-local */
    if(!IN6_IS_ADDR_LINKLOCAL(&src6)) {
        nd6stat.nd6s_rcv_badredirect++;
        goto bad;
    }

    if(m->m_len < off + (int)sizeof(*rd)) {
        if((m = m_pullup(m, off + sizeof(*rd))) == NULL) {
            nd6stat.nd6s_rcv_badredirect++;
            return;
        }
        ip6 = mtod(m, struct ip6_hdr *);
    }

    rd = (struct nd_redirect *)(mtod(m, u_int8_t *) + off);
    tgt    = rd->nd_rd_target;
    reddst = rd->nd_rd_dst;

    /* validate: source must be a known default router */
    if(defrouter_lookup(&src6, ifp) == NULL) {
        nd6stat.nd6s_rcv_badredirect++;
        goto bad;
    }

    /* destination must not be multicast */
    if(IN6_IS_ADDR_MULTICAST(&reddst)) {
        nd6stat.nd6s_rcv_badredirect++;
        goto bad;
    }

    /* if target != destination, target must be link-local (router) */
    if(!IN6_ARE_ADDR_EQUAL(&tgt, &reddst) &&
            !IN6_IS_ADDR_LINKLOCAL(&tgt)) {
        nd6stat.nd6s_rcv_badredirect++;
        goto bad;
    }

    /* update routing table */
    {
        struct sockaddr_in6 dst_sa, gw_sa;
        struct rtentry *rt;

        bzero(&dst_sa, sizeof(dst_sa));
        dst_sa.sin6_family = AF_INET6;
        dst_sa.sin6_len    = sizeof(dst_sa);
        dst_sa.sin6_addr   = reddst;

        bzero(&gw_sa, sizeof(gw_sa));
        gw_sa.sin6_family = AF_INET6;
        gw_sa.sin6_len    = sizeof(gw_sa);
        gw_sa.sin6_addr   = tgt;

        /* try to update existing route, or add new one */
        rt = nd6_lookup(&reddst, 0, ifp);
        if(rt) {
            /* change gateway */
            rtrequest(RTM_DELETE, (struct sockaddr *)&dst_sa,
                      NULL, NULL, RTF_HOST, NULL);
            rtfree(rt);         /* release the ref held by nd6_lookup */
        }
        rtrequest(RTM_ADD, (struct sockaddr *)&dst_sa,
                  (struct sockaddr *)&gw_sa, NULL,
                  RTF_HOST | RTF_GATEWAY | RTF_DYNAMIC, NULL);
    }

    /* update neighbor cache for the new target */
    {
        union nd_opts ndopts;
        u_int8_t *optstart = (u_int8_t *)(rd + 1);
        int optbytes = icmp6len - (int)sizeof(*rd);

        if(optbytes > 0) {
            nd6_option_init(optstart, optbytes, &ndopts);
            if(nd6_options(&ndopts) >= 0 &&
                    ndopts.nd_opts_tgt_lladdr) {
                char *lladdr = (char *)
                               (ndopts.nd_opts_tgt_lladdr + 1);
                nd6_cache_lladdr(ifp, &tgt, lladdr,
                                 ifp->if_addrlen, ND_REDIRECT, 0);
            }
        }
    }

    m_freem(m);
    return;

bad:
    m_freem(m);
}

#endif /* INET6 */
