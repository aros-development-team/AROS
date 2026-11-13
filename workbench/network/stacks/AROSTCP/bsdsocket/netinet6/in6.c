/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 implementation:
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * in6.c - IPv6 interface address management.
 *
 * Manages IPv6 addresses assigned to network interfaces, including
 * address addition/removal, ioctl handling, and link-local address
 * auto-generation.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>
#include <stdio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_sana.h>
#include <kern/amiga_dhcp.h>
#include <net/netmodule.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6.h>
#include <net/rtsock_protos.h>

#include <devices/sana2.h>
#include <protos/kern/kern_random_protos.h>
#include <protos/net/route_protos.h>
#include <net/netcore_map.h>        /* always the last include */

struct in6_ifaddr *in6_ifaddr = NULL;
struct ifqueue    ip6intrq    = {0};

int ip6_defhlim    = 64;
int ip6_forwarding = 0;
int ip6_v6only     = 1;   /* sockets default to IPv6-only */
int ip6_use_tempaddr = 1; /* enable privacy extensions by default */


/* ------------------------------------------------------------------
 * Utility: compare two IPv6 addresses.
 * ------------------------------------------------------------------ */
static int
in6_addrscope(struct in6_addr *addr)
{
    if(IN6_IS_ADDR_MULTICAST(addr)) {
        return addr->s6_addr[1] & 0x0f;
    }
    if(IN6_IS_ADDR_LINKLOCAL(addr))
        return 2;
    if(IN6_IS_ADDR_SITELOCAL(addr))
        return 5;
    return 14;  /* global */
}

/* ------------------------------------------------------------------
 * in6_control - ioctl handler for IPv6 interface addresses.
 * ------------------------------------------------------------------ */
static int in6_control_locked(struct socket *, int, caddr_t, struct ifnet *);

/* The address lists are walked by ip6_input, so they change at splimp */
int
in6_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    spl_t s = splimp();
    int error = in6_control_locked(so, cmd, data, ifp);

    splx(s);
    return (error);
}

static int
in6_control_locked(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    struct in6_ifreq  *ifr  = (struct in6_ifreq *)data;
    struct in6_aliasreq *ifra = (struct in6_aliasreq *)data;
    struct in6_ifaddr *ia   = NULL, *oia;
    int error = 0, isnew = 0;

    /* locate existing IPv6 address for this interface */
    if(ifp) {
        for(ia = in6_ifaddr; ia; ia = ia->ia_next)
            if(ia->ia6_ifp == ifp)
                break;
    }

    switch(cmd) {
    case SIOCAIFADDR_IN6:
    case SIOCDIFADDR_IN6:
        /* find the exact address being requested */
        if(ifra->ifra_addr.sin6_family == AF_INET6) {
            for(oia = ia; ia; ia = ia->ia_next) {
                if(ia->ia6_ifp == ifp &&
                        IN6_ARE_ADDR_EQUAL(
                            &ia->ia_addr.sin6_addr,
                            &ifra->ifra_addr.sin6_addr))
                    break;
            }
        }
        if(cmd == SIOCDIFADDR_IN6 && ia == NULL)
            return EADDRNOTAVAIL;
    /* FALLTHROUGH */
    case SIOCSIFADDR:
        if(ifp == NULL)
            panic("in6_control");
        if(ia == NULL) {
            struct ifaddr *ifa;
            struct mbuf   *m;

            /* allocate new address record */
            m = m_getclr(M_WAIT, MT_IFADDR);
            if(m == NULL)
                return ENOBUFS;
            isnew = 1;
            if((oia = in6_ifaddr) != NULL) {
                for(; oia->ia_next; oia = oia->ia_next)
                    ;
                oia->ia_next = mtod(m, struct in6_ifaddr *);
            } else {
                in6_ifaddr = mtod(m, struct in6_ifaddr *);
            }
            ia = mtod(m, struct in6_ifaddr *);
            /* link into ifp address list */
            if((ifa = ifp->if_addrlist) != NULL) {
                for(; ifa->ifa_next; ifa = ifa->ifa_next)
                    ;
                ifa->ifa_next = (struct ifaddr *)ia;
            } else {
                ifp->if_addrlist = (struct ifaddr *)ia;
            }
            ia->ia_ifa.ifa_addr =
                (struct sockaddr *)&ia->ia_addr;
            ia->ia_ifa.ifa_dstaddr =
                (struct sockaddr *)&ia->ia_dstaddr;
            ia->ia_ifa.ifa_netmask =
                (struct sockaddr *)&ia->ia_prefixmask;
            ia->ia_ifa.ifa_rtrequest = nd6_rtrequest;
            ia->ia6_ifp = ifp;
        }
        break;

    case SIOCGIFADDR_IN6:
        if(ia == NULL)
            return EADDRNOTAVAIL;
        /* if a specific address was supplied, find it */
        if(ifr->ifr_ifru.ifru_addr.sin6_family == AF_INET6 &&
                !IN6_IS_ADDR_UNSPECIFIED(&ifr->ifr_ifru.ifru_addr.sin6_addr)) {
            for(; ia; ia = ia->ia_next) {
                if(ia->ia6_ifp == ifp &&
                        IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr,
                                           &ifr->ifr_ifru.ifru_addr.sin6_addr))
                    break;
            }
            if(ia == NULL)
                return EADDRNOTAVAIL;
        }
        break;

    case SIOCGIFDSTADDR_IN6:
    case SIOCGIFNETMASK_IN6:
        if(ia == NULL)
            return EADDRNOTAVAIL;
        break;

    case SIOCGIFAFLAG_IN6:
        /* the flags of the address given, on this interface (KAME) */
        for(; ia; ia = ia->ia_next)
            if(ia->ia6_ifp == ifp &&
                    IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr,
                                       &ifr->ifr_ifru.ifru_addr.sin6_addr))
                break;
        if(ia == NULL)
            return EADDRNOTAVAIL;
        ifr->ifr_ifru.ifru_flags6 = ia->ia6_ifaflags;
        return 0;

    default:
        if(ifp == NULL || ifp->if_ioctl == NULL)
            return EOPNOTSUPP;
        return (*ifp->if_ioctl)(ifp, cmd, data);
    }

    switch(cmd) {
    case SIOCGIFADDR_IN6:
        ifr->ifr_ifru.ifru_addr = ia->ia_addr;
        break;

    case SIOCGIFDSTADDR_IN6:
        ifr->ifr_ifru.ifru_dstaddr = ia->ia_dstaddr;
        break;

    case SIOCGIFNETMASK_IN6:
        ifr->ifr_ifru.ifru_addr = ia->ia_prefixmask;
        break;

    case SIOCSIFADDR:
        /* set IPv6 address (from in6_aliasreq) */
        return in6_ifinit(ifp, ia, &ifra->ifra_addr, 1);

    case SIOCAIFADDR_IN6:
        /* add/change alias */
        if(ifra->ifra_prefixmask.sin6_len) {
            int i, b, plen = 0;
            u_int8_t *mbytes = ifra->ifra_prefixmask.sin6_addr.s6_addr;
            ia->ia_prefixmask = ifra->ifra_prefixmask;
            /* compute ia6_plen from the mask */
            for(i = 0; i < 16; i++) {
                for(b = 7; b >= 0; b--) {
                    if(mbytes[i] & (1 << b))
                        plen++;
                    else
                        goto done_plen;
                }
            }
done_plen:
            ia->ia6_plen = plen;
        }
        /*
         * On a point-to-point interface (e.g. a 6in4 tunnel) honour the
         * supplied destination (peer) address so in6_ifinit() installs the
         * host route to the peer.  Without a peer, in6_ifinit() adds no route
         * for a POINTOPOINT interface at all.
         */
        if((ifp->if_flags & IFF_POINTOPOINT) &&
                ifra->ifra_dstaddr.sin6_family == AF_INET6) {
            ia->ia_dstaddr = ifra->ifra_dstaddr;
        }
        if(ifra->ifra_addr.sin6_family == AF_INET6) {
            error = in6_ifinit(ifp, ia, &ifra->ifra_addr, 0);
            if(error)
                return error;
        }
        /*
         * The request's IN6_IFF_* flags are the address's, in ia6_ifaflags;
         * ia6_flags is the generic ifaddr's, where in6_ifinit() has just set
         * IFA_ROUTE for in6_ifscrub() to find. The states the stack keeps -
         * tentative, duplicated, deprecated - are not the caller's to set.
         */
        ia->ia6_ifaflags = (ia->ia6_ifaflags & IN6_IFF_STACKSTATE) |
                           (ifra->ifra_flags & ~IN6_IFF_STACKSTATE);
        ia->ia6_lifetime_vltime = ifra->ifra_lifetime.ia6t_vltime;
        ia->ia6_lifetime_pltime = ifra->ifra_lifetime.ia6t_pltime;
        /*
         * A new unicast address is tentative until duplicate address
         * detection finds nobody else using it (RFC 4862 section 5.4), as the
         * link-local is (in6_if_up()). Interfaces that cannot multicast
         * cannot ask, and the loopback has nobody to ask.
         */
        if(isnew && !IN6_IS_ADDR_MULTICAST(&ia->ia_addr.sin6_addr) &&
                (ifp->if_flags & (IFF_MULTICAST | IFF_LOOPBACK)) == IFF_MULTICAST)
            nd6_dad_start(ia);
        rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
        return 0;

    case SIOCDIFADDR_IN6:
        rt_newaddrmsg(RTM_DELADDR, (struct ifaddr *)ia, 0, NULL);
        in6_purgeaddr((struct ifaddr *)ia);
        break;

    default:
        if(ifp == NULL || ifp->if_ioctl == NULL)
            return EOPNOTSUPP;
        return (*ifp->if_ioctl)(ifp, cmd, data);
    }
    return 0;
}

/* ------------------------------------------------------------------
 * in6_ifscrub - remove route for an IPv6 interface address.
 * ------------------------------------------------------------------ */
void
in6_ifscrub(struct ifnet *ifp, struct in6_ifaddr *ia)
{
    if((ia->ia6_flags & IFA_ROUTE) == 0)
        return;
    if(ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))
        rtinit(&ia->ia_ifa, RTM_DELETE, RTF_HOST);
    else
        rtinit(&ia->ia_ifa, RTM_DELETE, 0);
    ia->ia6_flags &= ~IFA_ROUTE;
}

/*
 * in6_solicited_join - the address joins its solicited-node group,
 * ff02::1:ffXX:XXXX from its last 24 bits (RFC 4291 section 2.7.1), which
 * Neighbor Solicitations for it are sent to (RFC 4861 section 7.2.1). The
 * membership belongs to the address: in6_purgeaddr() leaves it. Several
 * addresses with the same last 24 bits share one group by reference count.
 * An address given anew, on a record that already had one, moves its
 * membership. Interfaces that cannot multicast do neighbour discovery
 * without it.
 */
static void
in6_solicited_join(struct ifnet *ifp, struct in6_ifaddr *ia)
{
    struct in6_ifaddr_priv *iap = IA6_PRIV(ia);
    struct in6_addr maddr;

    if((ifp->if_flags & IFF_MULTICAST) == 0)
        return;
    bzero(&maddr, sizeof(maddr));
    maddr.s6_addr[0]  = 0xff;
    maddr.s6_addr[1]  = 0x02;
    maddr.s6_addr[11] = 0x01;
    maddr.s6_addr[12] = 0xff;
    maddr.s6_addr[13] = ia->ia_addr.sin6_addr.s6_addr[13];
    maddr.s6_addr[14] = ia->ia_addr.sin6_addr.s6_addr[14];
    maddr.s6_addr[15] = ia->ia_addr.sin6_addr.s6_addr[15];

    if(iap->ia6p_solicited != NULL) {
        if(iap->ia6p_solicited->in6m_ifp == ifp &&
                IN6_ARE_ADDR_EQUAL(&iap->ia6p_solicited->in6m_addr, &maddr))
            return;
        in6_delmulti(iap->ia6p_solicited);
        iap->ia6p_solicited = NULL;
    }
    iap->ia6p_solicited = in6_addmulti(&maddr, ifp);
}

/*
 * in6_join_allnodes - the interface joins the all-nodes group, ff02::1
 * (RFC 4291 section 2.8), once: Router Advertisements and MLD Queries are
 * sent to it. The membership belongs to the interface and in6_ifdetach()
 * releases it. No report is ever sent for this group (mld6.c). The IPv6
 * counterpart of in_join_allhosts().
 */
static void
in6_join_allnodes(struct ifnet *ifp)
{
    struct in6_multi *in6m;
    struct in6_addr allnodes;

    if((ifp->if_flags & IFF_MULTICAST) == 0)
        return;
    bzero(&allnodes, sizeof(allnodes));
    allnodes.s6_addr[0]  = 0xff;
    allnodes.s6_addr[1]  = 0x02;
    allnodes.s6_addr[15] = 0x01;
    IN6_LOOKUP_MULTI(allnodes, ifp, in6m);
    if(in6m == NULL)
        (void)in6_addmulti(&allnodes, ifp);
}

/* ------------------------------------------------------------------
 * in6_ifinit - initialise an IPv6 interface address and add its route.
 * ------------------------------------------------------------------ */
int
in6_ifinit(struct ifnet *ifp, struct in6_ifaddr *ia,
           struct sockaddr_in6 *sin6, int scrub)
{
    struct sockaddr_in6 oldaddr;
    int flags = RTF_UP;
    int error;
    spl_t s;

    s = splimp();
    oldaddr = ia->ia_addr;
    ia->ia_addr = *sin6;

    if(ifp->if_ioctl &&
            (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)ia))) {
        splx(s);
        ia->ia_addr = oldaddr;
        return error;
    }
    splx(s);

    if(scrub) {
        ia->ia_ifa.ifa_addr = (struct sockaddr *)&oldaddr;
        in6_ifscrub(ifp, ia);
        ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
    }

    /* the address is ours from here on: neighbours must be able to ask for it */
    in6_solicited_join(ifp, ia);

    /* build prefix mask from prefix length */
    if(ia->ia6_plen > 0) {
        int i, plen = ia->ia6_plen;
        u_int8_t *mask = ia->ia_prefixmask.sin6_addr.s6_addr;
        bzero(mask, 16);
        for(i = 0; i < 16 && plen >= 8; i++, plen -= 8)
            mask[i] = 0xff;
        if(plen > 0)
            mask[i] = (0xff << (8 - plen)) & 0xff;
        ia->ia_prefixmask.sin6_family = AF_INET6;
        ia->ia_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
    }

    if(ifp->if_flags & IFF_LOOPBACK) {
        ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
        flags |= RTF_HOST;
    } else if(ifp->if_flags & IFF_POINTOPOINT) {
        if(ia->ia_dstaddr.sin6_family != AF_INET6)
            return 0;
        flags |= RTF_HOST;
    } else {
        /*
         * On broadcast/multicast interfaces (e.g. Ethernet), set
         * RTF_CLONING so that rtalloc1() can create per-host cloned
         * routes for each neighbor.  Without this, all neighbors on
         * the prefix share one route entry and stomp on each other's
         * cached link-layer address in rt_gateway / rt_llinfo.
         */
        flags |= RTF_CLONING;
    }

    error = rtinit(&ia->ia_ifa, RTM_ADD, flags);
    if(error == 0)
        ia->ia6_flags |= IFA_ROUTE;
    else if(error == EEXIST)
        error = 0;  /* prefix route already present (e.g. a reload re-applying
                     * an interface's existing config) - that is not an error */

    return error;
}

/* ------------------------------------------------------------------
 * in6_purgeaddr - remove an IPv6 address from an interface.
 * ------------------------------------------------------------------ */
void
in6_purgeaddr(struct ifaddr *ifa)
{
    struct in6_ifaddr *ia  = (struct in6_ifaddr *)ifa;
    struct in6_ifaddr *oia;
    struct ifnet *ifp = ia->ia6_ifp;
    struct ifaddr *ifa2;

    /* an address deleted during duplicate address detection must not be
     * probed for, or written to, by the DAD timer once it is freed */
    nd6_dad_stop(ia);

    in6_ifscrub(ifp, ia);

    /*
     * Routes hold no reference on their rt_ifa: the host routes cloned from
     * the prefix route in6_ifscrub() just removed, and any other route that
     * resolved through this address, would be left pointing at it once it is
     * freed below, and deleting them later (rt_purgeif() on a reload, for
     * one) would call ifa_rtrequest through freed memory. Delete them now,
     * as in_control() does for an IPv4 address.
     */
    rt_purgeifa((struct ifaddr *)ia);

    /* the address no longer answers neighbour solicitations */
    if(IA6_PRIV(ia)->ia6p_solicited != NULL) {
        in6_delmulti(IA6_PRIV(ia)->ia6p_solicited);
        IA6_PRIV(ia)->ia6p_solicited = NULL;
    }

    /* unlink from ifp address list */
    if((ifa2 = ifp->if_addrlist) == (struct ifaddr *)ia) {
        ifp->if_addrlist = ifa2->ifa_next;
    } else {
        while(ifa2->ifa_next && ifa2->ifa_next != (struct ifaddr *)ia)
            ifa2 = ifa2->ifa_next;
        if(ifa2->ifa_next)
            ifa2->ifa_next = ((struct ifaddr *)ia)->ifa_next;
    }

    /* unlink from global list */
    oia = ia;
    if(oia == in6_ifaddr) {
        in6_ifaddr = ia->ia_next;
    } else {
        for(ia = in6_ifaddr; ia && ia->ia_next != oia; ia = ia->ia_next)
            ;
        if(ia)
            ia->ia_next = oia->ia_next;
    }

    (void)m_free(dtom(oia));
}

/* ------------------------------------------------------------------
 * in6_localaddr - return 1 if addr is a local IPv6 address.
 * ------------------------------------------------------------------ */
int
in6_localaddr(struct in6_addr *in6)
{
    struct in6_ifaddr *ia;

    if(IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_ADDR_LINKLOCAL(in6))
        return 1;

    /* on a link we have an address on, as in_localaddr() for IPv4 and KAME */
    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        int i;

        for(i = 0; i < 16; i++)
            if((in6->s6_addr[i] ^ ia->ia_addr.sin6_addr.s6_addr[i]) &
                    ia->ia_prefixmask.sin6_addr.s6_addr[i])
                break;
        if(i == 16)
            return 1;
    }
    return 0;
}

/*
 * in6_matchlen - the number of leading bits a and b have in common.
 */
static int
in6_matchlen(const struct in6_addr *a, const struct in6_addr *b)
{
    int i, n = 0;

    for(i = 0; i < 16; i++) {
        u_int8_t x = a->s6_addr[i] ^ b->s6_addr[i];

        if(x == 0) {
            n += 8;
            continue;
        }
        while(!(x & 0x80)) {
            n++;
            x <<= 1;
        }
        break;
    }
    return n;
}

/* the length of ia's prefix, from its prefix mask */
static int
in6_ia_plen(const struct in6_ifaddr *ia)
{
    int i, n = 0;

    for(i = 0; i < 16; i++) {
        u_int8_t x = ia->ia_prefixmask.sin6_addr.s6_addr[i];

        while(x & 0x80) {
            n++;
            x <<= 1;
        }
        if(ia->ia_prefixmask.sin6_addr.s6_addr[i] != 0xff)
            break;
    }
    return n;
}

/*
 * in6_addrlabel - the label of an address in the default policy table of
 * RFC 6724 section 2.1 (the longest prefix that matches).
 */
static int
in6_addrlabel(const struct in6_addr *a)
{
    static const struct {
        u_int8_t prefix[16];
        int len, label;
    } policy[] = {
        { { 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 },           128,  0 },  /* ::1 */
        { { 0,0,0,0, 0,0,0,0, 0,0,0xff,0xff, 0,0,0,0 },      96,  4 },  /* ::ffff:0:0/96 */
        { { 0 },                                              96,  3 },  /* ::/96 */
        { { 0x20,0x01,0,0 },                                  32,  5 },  /* 2001::/32 (Teredo) */
        { { 0x20,0x02 },                                      16,  2 },  /* 2002::/16 (6to4) */
        { { 0x3f,0xfe },                                      16, 12 },  /* 3ffe::/16 (6bone) */
        { { 0xfe,0xc0 },                                      10, 11 },  /* fec0::/10 (site-local) */
        { { 0xfc },                                            7, 13 },  /* fc00::/7 (ULA) */
    };
    int i, best = -1, label = 1;                /* ::/0 */

    for(i = 0; i < (int)(sizeof(policy) / sizeof(policy[0])); i++)
        if(policy[i].len > best &&
                in6_matchlen(a, (const struct in6_addr *)policy[i].prefix) >= policy[i].len) {
            best = policy[i].len;
            label = policy[i].label;
        }
    return label;
}

/*
 * in6_srcbetter - whether a is a better source address than b for dst, of
 * scope dstscope, by the rules of RFC 6724 section 5 that apply to addresses
 * of one interface:
 *  1. the destination itself;
 *  2. appropriate scope: of two scopes, the smaller one unless it is smaller
 *     than the destination's (so a link-scope destination gets the
 *     link-local, and a global one a global address);
 *  3. a preferred address over a deprecated one;
 *  6. one whose label is the destination's (so a ULA destination gets the
 *     ULA, and a global one a global address rather than the ULA);
 *  7. a temporary address over a public one (RFC 4941);
 *  8. the longer prefix in common with the destination, counted no further
 *     than the source's own prefix.
 * Otherwise the earlier address stays.
 */
static int
in6_srcbetter(struct in6_ifaddr *a, struct in6_ifaddr *b,
              const struct in6_addr *dst, int dstscope)
{
    int sa = in6_addrscope(&a->ia_addr.sin6_addr);
    int sb = in6_addrscope(&b->ia_addr.sin6_addr);
    int da = (a->ia6_ifaflags & IN6_IFF_DEPRECATED) != 0;
    int db = (b->ia6_ifaflags & IN6_IFF_DEPRECATED) != 0;
    int ta = (a->ia6_ifaflags & IN6_IFF_TEMPORARY) != 0;
    int tb = (b->ia6_ifaflags & IN6_IFF_TEMPORARY) != 0;
    int ea = IN6_ARE_ADDR_EQUAL(&a->ia_addr.sin6_addr, dst);
    int eb = IN6_ARE_ADDR_EQUAL(&b->ia_addr.sin6_addr, dst);
    int la, lb, ma, mb, pa, pb, ldst;

    if(ea != eb)
        return ea;
    if(sa != sb) {
        if(sa < sb)
            return sa >= dstscope;
        return sb < dstscope;
    }
    if(da != db)
        return db;
    ldst = in6_addrlabel(dst);
    la = in6_addrlabel(&a->ia_addr.sin6_addr) == ldst;
    lb = in6_addrlabel(&b->ia_addr.sin6_addr) == ldst;
    if(la != lb)
        return la;
    if(ta != tb)
        return ta;
    ma = in6_matchlen(&a->ia_addr.sin6_addr, dst);
    mb = in6_matchlen(&b->ia_addr.sin6_addr, dst);
    pa = in6_ia_plen(a);
    pb = in6_ia_plen(b);
    if(ma > pa)
        ma = pa;
    if(mb > pb)
        mb = pb;
    return ma > mb;
}

/* ------------------------------------------------------------------
 * in6_ifawithifp - find best IPv6 source address for given ifp/dest.
 * ------------------------------------------------------------------ */
struct in6_ifaddr *
in6_ifawithifp(struct ifnet *ifp, struct in6_addr *dst)
{
    struct in6_ifaddr *ia, *besta = NULL;
    int dstscope = in6_addrscope(dst);

    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(ia->ia6_ifp != ifp)
            continue;
        if(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED))
            continue;
        if(besta == NULL || in6_srcbetter(ia, besta, dst, dstscope))
            besta = ia;
    }
    return besta;
}

/* ------------------------------------------------------------------
 * in6_ifaof_ifpforlinklocal - find the link-local address of an ifp.
 * ------------------------------------------------------------------ */
struct in6_ifaddr *
in6_ifaof_ifpforlinklocal(struct ifnet *ifp)
{
    struct in6_ifaddr *ia;

    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(ia->ia6_ifp == ifp &&
                IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr) &&
                !(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED)))
            return ia;
    }
    return NULL;
}

/* ------------------------------------------------------------------
 * in6_if_up - called when interface is brought up; configure link-local.
 * ------------------------------------------------------------------ */
/*
 * in6_ifaddr records are allocated from mbufs (m_getclr, MT_IFADDR) so they
 * can later be released with m_free(dtom(ia)) (see in6.c:351).  That requires
 * the whole structure to fit within a single mbuf's data area; assert it at
 * compile time so growing the struct (or shrinking MSIZE) breaks the build
 * loudly instead of silently corrupting memory.
 */
typedef char in6_ifaddr_fits_mbuf[(sizeof(struct in6_ifaddr_priv) <= MLEN) ? 1 : -1];

void
in6_if_up(struct ifnet *ifp)
{
    struct in6_ifaddr *ia;
    struct sockaddr_in6 sin6;
    struct mbuf *m;

    D(bug("[AROSTCP:IN6] %s: ifp=%s%d flags=0x%x addrlen=%d\n",
          __func__, ifp->if_name, ifp->if_unit, ifp->if_flags, ifp->if_addrlen));

    /* skip loopback - it already has ::1 */
    if(ifp->if_flags & IFF_LOOPBACK) {
        D(bug("[AROSTCP:IN6] %s: skip loopback\n", __func__));
        return;
    }

    /* check if link-local already configured */
    if(in6_ifaof_ifpforlinklocal(ifp)) {
        D(bug("[AROSTCP:IN6] %s: link-local already configured\n", __func__));
        return;
    }

    /* only for interfaces with a link-layer address */
    if(ifp->if_addrlen == 0) {
        D(bug("[AROSTCP:IN6] %s: no link-layer address (addrlen=0)\n", __func__));
        return;
    }

    /* allocate address record */
    m = m_getclr(M_WAIT, MT_IFADDR);
    if(m == NULL) {
        D(bug("[AROSTCP:IN6] %s: m_getclr failed\n", __func__));
        return;
    }

    ia = mtod(m, struct in6_ifaddr *);

    /* build EUI-64 link-local from MAC address */
    bzero(&sin6, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_len    = sizeof(sin6);
    sin6.sin6_addr.s6_addr[0] = 0xfe;
    sin6.sin6_addr.s6_addr[1] = 0x80;
    /* EUI-64: copy MAC, flip U/L bit; find AF_LINK entry in addrlist */
    if(ifp->if_addrlen == 6) {
        struct ifaddr *ifa2;
        struct sockaddr_dl *sdl = NULL;
        for(ifa2 = ifp->if_addrlist; ifa2; ifa2 = ifa2->ifa_next) {
            if(ifa2->ifa_addr &&
                    ifa2->ifa_addr->sa_family == AF_LINK) {
                sdl = (struct sockaddr_dl *)ifa2->ifa_addr;
                break;
            }
        }
        if(sdl == NULL || sdl->sdl_alen < 6) {
            D(bug("[AROSTCP:IN6] %s: no AF_LINK sdl (sdl=%p alen=%d)\n",
                  __func__, sdl, sdl ? sdl->sdl_alen : -1));
            m_free(m);
            return;
        }
        u_int8_t *mac = (u_int8_t *)LLADDR(sdl);
        D(bug("[AROSTCP:IN6] %s: MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
              __func__, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
        sin6.sin6_addr.s6_addr[8]  = mac[0] ^ 0x02;
        sin6.sin6_addr.s6_addr[9]  = mac[1];
        sin6.sin6_addr.s6_addr[10] = mac[2];
        sin6.sin6_addr.s6_addr[11] = 0xff;
        sin6.sin6_addr.s6_addr[12] = 0xfe;
        sin6.sin6_addr.s6_addr[13] = mac[3];
        sin6.sin6_addr.s6_addr[14] = mac[4];
        sin6.sin6_addr.s6_addr[15] = mac[5];
    } else {
        /* can't form EUI-64 */
        D(bug("[AROSTCP:IN6] %s: addrlen=%d, can't form EUI-64\n",
              __func__, ifp->if_addrlen));
        m_free(m);
        return;
    }

    D(bug("[AROSTCP:IN6] %s: link-local = fe80::%02x%02x:%02xff:fe%02x:%02x%02x\n",
          __func__,
          sin6.sin6_addr.s6_addr[8], sin6.sin6_addr.s6_addr[9],
          sin6.sin6_addr.s6_addr[10],
          sin6.sin6_addr.s6_addr[13],
          sin6.sin6_addr.s6_addr[14], sin6.sin6_addr.s6_addr[15]));

    /*
     * Initialise the ifaddr fields BEFORE the entry is linked into the
     * global and per-interface lists, so any consumer walking those lists
     * never sees a half-initialised record (e.g. a NULL ifa_addr).
     */
    ia->ia_ifa.ifa_addr    = (struct sockaddr *)&ia->ia_addr;
    ia->ia_ifa.ifa_netmask = (struct sockaddr *)&ia->ia_prefixmask;
    ia->ia_ifa.ifa_rtrequest = nd6_rtrequest;
    ia->ia6_ifp = ifp;
    ia->ia6_plen = 64;
    ia->ia6_ifaflags = IN6_IFF_AUTOCONF;

    /* link into global and ifp lists */
    if(in6_ifaddr) {
        struct in6_ifaddr *tail;
        for(tail = in6_ifaddr; tail->ia_next; tail = tail->ia_next)
            ;
        tail->ia_next = ia;
    } else {
        in6_ifaddr = ia;
    }
    if(ifp->if_addrlist) {
        struct ifaddr *tail;
        for(tail = ifp->if_addrlist; tail->ifa_next; tail = tail->ifa_next)
            ;
        tail->ifa_next = (struct ifaddr *)ia;
    } else {
        ifp->if_addrlist = (struct ifaddr *)ia;
    }

    D(bug("[AROSTCP:IN6] %s: calling in6_ifinit\n", __func__));
    (void)in6_ifinit(ifp, ia, &sin6, 0);

    /*
     * The NIC must accept the all-nodes group for Router Advertisements
     * (RFC 4291 section 2.8). The link-local's solicited-node group, for
     * neighbour discovery and DAD, was joined by in6_ifinit() above.
     */
    in6_join_allnodes(ifp);

    /*
     * Initialize ND state for this interface and start DAD.
     * After DAD completes, nd6_dad_timer() will send the first RS.
     */
    D(bug("[AROSTCP:IN6] %s: calling nd6_ifattach\n", __func__));
    nd6_ifattach(ifp);
    D(bug("[AROSTCP:IN6] %s: calling nd6_dad_start\n", __func__));
    nd6_dad_start(ia);
    D(bug("[AROSTCP:IN6] %s: done for %s%d\n", __func__, ifp->if_name, ifp->if_unit));
}

/* ------------------------------------------------------------------
 * in6_ifconf - enumerate all IPv6 interface addresses (SIOCGLIFADDR).
 *
 * Works like ifconf()/SIOCGIFCONF but fills the buffer with
 * in6_ifreq entries, one per IPv6 address on all interfaces.
 * ifc->ifc_len is set to the number of bytes written on return.
 * ------------------------------------------------------------------ */
int
in6_ifconf(int cmd, caddr_t data)
{
    struct in6_ifconf *ifc = (struct in6_ifconf *)data;
    struct in6_ifreq  *ifrp;
    struct in6_ifaddr *ia;
    int space, error = 0;

    ifrp  = ifc->ifc6_req;
    space = ifc->ifc6_len;

    for(ia = in6_ifaddr; ia && space >= (int)sizeof(*ifrp); ia = ia->ia_next) {
        struct in6_ifreq ifr6;
        struct ifnet *ifp = ia->ia6_ifp;
        if(ifp == NULL)
            continue;

        bzero(&ifr6, sizeof(ifr6));
        if_fullname(ifp, ifr6.ifr_name, sizeof(ifr6.ifr_name));
        ifr6.ifr_ifru.ifru_addr = ia->ia_addr;

        bcopy(&ifr6, ifrp, sizeof(ifr6));
        ifrp++;
        space -= sizeof(ifr6);
    }

    ifc->ifc6_len -= space;
    return error;
}

/*
 * in6_map_mcast_macaddr - compute Ethernet multicast MAC from IPv6 multicast addr.
 * RFC 2464 §7: map ff02::X to 33:33:XX:XX:XX:XX (last 4 bytes of IPv6 addr).
 */
static void
in6_map_mcast_macaddr(struct in6_addr *addr, u_int8_t *mac)
{
    mac[0] = 0x33;
    mac[1] = 0x33;
    mac[2] = addr->s6_addr[12];
    mac[3] = addr->s6_addr[13];
    mac[4] = addr->s6_addr[14];
    mac[5] = addr->s6_addr[15];
}

/*
 * in6_mcast_device - tell the interface's device to accept (add) or stop
 * accepting (!add) the link address of an IPv6 group. The device request
 * is the core's (sana_mcast(), shared with IPv4); only the mapping is ours.
 */
static int
in6_mcast_device(struct ifnet *ifp, struct in6_addr *maddr, int add)
{
    u_int8_t mac[6];

    in6_map_mcast_macaddr(maddr, mac);
    return sana_mcast(ifp, mac, add);
}

/*
 * in6_addmulti - add (or refcount) a multicast group membership on an interface.
 * Also registers the corresponding Ethernet multicast address with the SANA-II
 * device so the NIC hardware filter accepts matching frames.
 */
struct in6_multi *in6_multihead = NULL;

struct in6_multi *
in6_addmulti(struct in6_addr *maddr, struct ifnet *ifp)
{
    struct in6_multi *in6m;

    D(bug("[AROSTCP:IN6] %s: ifp=%s%d maddr=%02x%02x:%02x%02x:...:%02x%02x\n",
          __func__, ifp->if_name, ifp->if_unit,
          maddr->s6_addr[0], maddr->s6_addr[1],
          maddr->s6_addr[2], maddr->s6_addr[3],
          maddr->s6_addr[14], maddr->s6_addr[15]));

    /* search for an existing entry */
    for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
        if(in6m->in6m_ifp == ifp &&
                IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, maddr)) {
            in6m->in6m_refcount++;
            D(bug("[AROSTCP:IN6] %s: existing entry, refcount=%d\n",
                  __func__, in6m->in6m_refcount));
            return in6m;
        }
    }

    /* register the group's link address with the device */
    if(in6_mcast_device(ifp, maddr, 1) != 0) {
        __log(LOG_ERR, "in6_addmulti: S2_ADDMULTICASTADDRESS failed for %s%d\n",
              ifp->if_name, ifp->if_unit);
        /* continue anyway — some drivers accept all multicast */
    }

    /* allocate a new entry */
    MALLOC(in6m, struct in6_multi *, sizeof(*in6m), M_IFADDR, M_WAITOK);
    if(in6m == NULL)
        return NULL;
    bzero(in6m, sizeof(*in6m));
    in6m->in6m_addr    = *maddr;
    in6m->in6m_ifp     = ifp;
    in6m->in6m_refcount = 1;
    in6m->in6m_state   = MLD6_IDLE_MEMBER;
    in6m->in6m_timer   = 0;
    in6m->in6m_next    = in6_multihead;
    in6_multihead      = in6m;

    /* Send initial MLD report for this group */
    mld6_start_listening(in6m);

    D(bug("[AROSTCP:IN6] %s: created new entry\n", __func__));
    return in6m;
}

/*
 * in6_delmulti - decrement refcount and free if zero.
 * Unregisters the Ethernet multicast address from the SANA-II device when
 * the last reference is released.
 */
void
in6_delmulti(struct in6_multi *in6m)
{
    struct in6_multi **p;

    if(--in6m->in6m_refcount > 0)
        return;

    /* a membership that outlived its interface has nobody to tell;
     * see in6_ifdetach() */
    if(in6m->in6m_ifp != NULL) {
        /* Send MLD Done before removing from list */
        mld6_stop_listening(in6m);

        /* unregister the group's link address from the device */
        in6_mcast_device(in6m->in6m_ifp, &in6m->in6m_addr, 0);
    }

    for(p = &in6_multihead; *p; p = &(*p)->in6m_next) {
        if(*p == in6m) {
            *p = in6m->in6m_next;
            break;
        }
    }
    FREE(in6m, M_IFADDR);
}

/*
 * in6_ifdetach - the interface is going away for good: nothing may keep an
 * IPv6 membership that names it. Another ifnet can be allocated at the same
 * address later, and would then look as if it had joined already, so the
 * device would never be told the group's address; and the MLD timer and
 * in6_delmulti() would go on using the freed interface.
 *
 * By now its addresses are gone (sana_if_deconfigure()), and their
 * solicited-node memberships with them. The interface's own membership of
 * the all-nodes group, from in6_join_allnodes(), is released here. What
 * sockets still hold cannot be freed under them: those entries are cut
 * loose from the interface and go when their sockets drop them. Nothing is
 * sent and the device is not asked to forget the addresses: it is offline
 * and about to be closed. The IPv6 counterpart of in_ifdetach().
 */
void
in6_ifdetach(struct ifnet *ifp)
{
    struct in6_multi *in6m, **p;
    struct in6_addr allnodes;
    spl_t s = splimp();

    bzero(&allnodes, sizeof(allnodes));
    allnodes.s6_addr[0]  = 0xff;
    allnodes.s6_addr[1]  = 0x02;
    allnodes.s6_addr[15] = 0x01;

    p = &in6_multihead;
    while((in6m = *p) != NULL) {
        if(in6m->in6m_ifp != ifp) {
            p = &in6m->in6m_next;
            continue;
        }
        in6m->in6m_timer = 0;
        in6m->in6m_state = MLD6_IDLE_MEMBER;
        in6m->in6m_ifp = NULL;
        if(IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &allnodes) &&
                --in6m->in6m_refcount == 0) {
            *p = in6m->in6m_next;
            FREE(in6m, M_IFADDR);
            continue;
        }
        p = &in6m->in6m_next;
    }
    splx(s);
}

/* ==================================================================
 * IPv6 Privacy Extensions (RFC 4941) - Temporary Address Management
 * ================================================================== */

/*
 * A random interface identifier for a temporary address (RFC 8981 section
 * 3.3.1), from the C runtime's arc4random_buf() through kern_random.c: a
 * xorshift generator seeded from the clock and the task address made them
 * guessable, and a temporary address is there so that it cannot be.
 */
static void
in6_generate_random_iid(struct in6_addr *addr)
{
    kern_random(&addr->s6_addr[8], 8);

    /* clear the universal/local bit (RFC 4941 §3.2.1) - set to "local" */
    addr->s6_addr[8] &= ~0x02;
}

/*
 * in6_tmpifadd - create a temporary address for the given prefix on ifp.
 * Called when an RA with the 'A' (autonomous) flag is processed.
 */
int
in6_tmpifadd(struct ifnet *ifp, struct nd_prefix *pr)
{
    struct in6_ifaddr *newia;
    struct in6_aliasreq ifra;
    struct in6_addr tmpaddr;
    struct sockaddr_in6 *sin6;
    struct timeval _tv;
    int error;

    if(!ip6_use_tempaddr)
        return 0;

    if(pr->ndpr_plen != 64)
        return 0;  /* SLAAC temporary addresses require /64 */

    /*
     * One temporary address per prefix at a time (RFC 4941 section 3.4): a
     * Router Advertisement repeating the prefix makes another only once the
     * current one is deprecated - its preferred lifetime has run out - or
     * was found to be a duplicate.
     */
    for(struct in6_ifaddr *ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(ia->ia6_ifp == ifp &&
                (ia->ia6_ifaflags & IN6_IFF_TEMPORARY) &&
                !(ia->ia6_ifaflags & (IN6_IFF_DEPRECATED | IN6_IFF_DUPLICATED)) &&
                bcmp(&ia->ia_addr.sin6_addr, &pr->ndpr_prefix.sin6_addr, 8) == 0)
            return 0;
    }

    /* build the temporary address: prefix + random IID */
    bzero(&tmpaddr, sizeof(tmpaddr));
    bcopy(&pr->ndpr_prefix.sin6_addr, &tmpaddr, 8);
    in6_generate_random_iid(&tmpaddr);

    /* check for conflicts with existing addresses */
    for(struct in6_ifaddr *ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, &tmpaddr))
            return EEXIST;  /* collision - caller should retry */
    }

    /* allocate and initialize */
    bzero(&ifra, sizeof(ifra));
    sin6 = &ifra.ifra_addr;
    sin6->sin6_len    = sizeof(*sin6);
    sin6->sin6_family = AF_INET6;
    sin6->sin6_addr   = tmpaddr;

    /* set prefix mask (/64) */
    ifra.ifra_prefixmask.sin6_len    = sizeof(struct sockaddr_in6);
    ifra.ifra_prefixmask.sin6_family = AF_INET6;
    {
        int i;
        for(i = 0; i < 8; i++)
            ifra.ifra_prefixmask.sin6_addr.s6_addr[i] = 0xff;
    }

    ifra.ifra_flags = IN6_IFF_AUTOCONF;

    /* set lifetimes: min of RA prefix lifetime and configured max */
    {
        u_int32_t preferred = pr->ndpr_pltime;
        u_int32_t valid = pr->ndpr_vltime;
        if(preferred > IP6_TEMP_PREFERRED_LIFETIME)
            preferred = IP6_TEMP_PREFERRED_LIFETIME;
        if(valid > IP6_TEMP_VALID_LIFETIME)
            valid = IP6_TEMP_VALID_LIFETIME;
        ifra.ifra_lifetime.ia6t_vltime = valid;
        ifra.ifra_lifetime.ia6t_pltime = preferred;
    }

    error = in6_control(NULL, SIOCAIFADDR_IN6, (caddr_t)&ifra, ifp);
    if(error)
        return error;

    /* find the newly created address and set temporary flags + lifetimes */
    GetUpTime(&_tv);
    for(newia = in6_ifaddr; newia; newia = newia->ia_next) {
        if(IN6_ARE_ADDR_EQUAL(&newia->ia_addr.sin6_addr, &tmpaddr) &&
           newia->ia6_ifp == ifp)
            break;
    }
    if(newia) {
        u_int32_t preferred = pr->ndpr_pltime;
        u_int32_t valid = pr->ndpr_vltime;
        if(preferred > IP6_TEMP_PREFERRED_LIFETIME)
            preferred = IP6_TEMP_PREFERRED_LIFETIME;
        if(valid > IP6_TEMP_VALID_LIFETIME)
            valid = IP6_TEMP_VALID_LIFETIME;

        newia->ia6_ifaflags |= IN6_IFF_TEMPORARY;
        newia->ia6_lifetime_preferred = (u_int32_t)_tv.tv_sec + preferred;
        newia->ia6_lifetime_expire = (u_int32_t)_tv.tv_sec + valid;

        D(bug("[AROSTCP:IN6] %s: created temporary addr on %s%d "
              "(preferred=%lus valid=%lus)\n",
              __func__, ifp->if_name, ifp->if_unit,
              (unsigned long)preferred, (unsigned long)valid));
    }

    /* its solicited-node group was joined by in6_ifinit() */
    return 0;
}

/*
 * in6_tmpaddrtimer - periodic maintenance of temporary addresses.
 * Called from icmp6_slowtimo() every 500ms.
 */
void
in6_tmpaddrtimer(void)
{
    struct in6_ifaddr *ia, *nia;
    struct timeval _tv;

    GetUpTime(&_tv);

    /* temporary addresses (RFC 4941) and those stateless autoconfiguration
     * made (RFC 4862 section 5.5.4): deprecated at the end of the preferred
     * lifetime, removed at the end of the valid one */
    for(ia = in6_ifaddr; ia; ia = nia) {
        nia = ia->ia_next;
        if(!(ia->ia6_ifaflags & (IN6_IFF_TEMPORARY | IN6_IFF_AUTOCONF)))
            continue;

        /* check valid lifetime expiry */
        if(ia->ia6_lifetime_expire &&
           (u_int32_t)_tv.tv_sec >= ia->ia6_lifetime_expire) {
            D(bug("[AROSTCP:IN6] %s: removing expired autoconfigured addr\n",
                  __func__));
            in6_purgeaddr((struct ifaddr *)ia);
            continue;
        }

        /* check preferred lifetime - deprecate if expired */
        if(ia->ia6_lifetime_preferred &&
           (u_int32_t)_tv.tv_sec >= ia->ia6_lifetime_preferred) {
            ia->ia6_ifaflags |= IN6_IFF_DEPRECATED;
        }
    }
}

#endif /* INET6 */


/*
 * IPv6's part in the life of an interface (net/netmodule.h). Registered
 * from ip6_init().
 */
static int
in6_ifevent(int event, struct ifnet *ifp)
{
    struct ifaddr *ifa, *nifa;
    int deleted = 0;

    switch(event) {
    case NFE_UP:
        /* the link-local address (EUI-64) is the stack's own to make */
        in6_if_up(ifp);
        break;

    case NFE_UNCONFIGURE_PRE:
        /* forget the routers heard on it, the routes they gave, the prefixes */
        nd6_purge_routers(ifp);
        nd6_purge_prefixes(ifp);
        break;

    case NFE_UNCONFIGURE:
        for(ifa = ifp->if_addrlist; ifa; ifa = nifa) {
            nifa = ifa->ifa_next;       /* in6_purgeaddr() unlinks+frees ifa */
            if(ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET6) {
                in6_purgeaddr(ifa);
                deleted++;
            }
        }
        break;

    case NFE_REMOVING:
#if DHCP6
        kill_dhclient6(ifp);
#endif
        break;

    case NFE_DETACHING:
        nd6_ifdetach(ifp);
        break;

    case NFE_DETACHED:
        /* (the sockets that name it are the transports' to see to:
         * their own handler, netinet/tcp_subr.c) */
        in6_ifdetach(ifp);
        break;
    }
    return (deleted);
}

/* IPv6's address requests: the list request names no interface, the others one. */
static int
in6_ifcontrol(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    if(cmd == SIOCGLIFADDR)
        return (in6_ifconf(cmd, data));
    return (in6_control(so, cmd, data, ifp));
}

/*
 * What the core asks about an IPv6 address (NFQ_xxx, net/netmodule.h).
 * Only one question is answered; -1 is "not answered", which is what the
 * core was told about every IPv6 address before.
 *
 * NFQ_GATEWAYLINK: a link-local gateway is on the link its zone names (RFC
 * 4007 section 6), and the route through it takes that interface and its
 * link-local address - not whichever interface's fe80::/64 the address
 * happens to match first, which sent the traffic of a route through
 * fe80::1%eth1 out by eth0, to another neighbour. The answer is the index
 * of that interface, 0 when the gateway names no link; `res' is then this
 * host's link-local address on it as the interface has it, or of length 0
 * when there is no such interface or it has no usable link-local address.
 */
static int
in6_addrquery(int what, const struct sockaddr *sa, struct sockaddr *res)
{
    const struct sockaddr_in6 *g6 = (const struct sockaddr_in6 *)sa;
    struct in6_ifaddr *ia;
    struct ifnet *ifp;

    if(what != NFQ_GATEWAYLINK)
        return (-1);
    if(g6->sin6_scope_id == 0 || !(IN6_IS_ADDR_LINKLOCAL(&g6->sin6_addr) ||
                                   IN6_IS_ADDR_MC_LINKLOCAL(&g6->sin6_addr)))
        return (0);
    bzero((caddr_t)res, sizeof(struct sockaddr_in6));
    for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
        if(ifp->if_index == g6->sin6_scope_id)
            break;
    if(ifp != NULL && (ia = in6_ifaof_ifpforlinklocal(ifp)) != NULL)
        *(struct sockaddr_in6 *)res = ia->ia_addr;
    return ((int)g6->sin6_scope_id);
}

const struct netfamily_ifops in6_ifops = {
    AF_INET6, 20, in6_ifevent, in6_ifcontrol, in6_addrquery
};

/*
 * IPV6 STARTS WHILE THE STACK RUNS (inet6_start(), netinet6/in6_proto.c):
 * the interfaces are there already, and nobody will raise for them the
 * events a starting stack raises. For each interface that is up IPv6 does
 * what it does when one comes up (NFE_UP): its link-local address, the
 * all-nodes and solicited-node groups, the interface's neighbour discovery
 * state, duplicate address detection - after which the routers are
 * solicited, and what they advertise (prefixes, addresses made from them,
 * routes, DNS servers) comes by itself. The loopback interface gets IPv6's
 * loopback address, ::1, which the stack's start-up gives it (loconfig(),
 * net/if_loop.c) and which went with IPv6 when it stopped.
 *
 * The addresses and the default route the configuration gives an
 * interface, and DHCPv6, come back with the next reload of the
 * configuration, which reads them again: the stack keeps no copy of an
 * interface's configuration that a family could apply by itself.
 *
 * When the stack itself starts, no interface is up yet when the families
 * are started, and this does nothing.
 */
void
in6_attach_running(void)
{
    struct ifnet *ifp;

    for(ifp = ifnet_list; ifp != NULL; ifp = ifp->if_next) {
        if(!(ifp->if_flags & IFF_UP))
            continue;
        if(ifp->if_flags & IFF_LOOPBACK) {
            static const struct in6_addr loopback6 = IN6ADDR_LOOPBACK_INIT;
            struct in6_aliasreq ifra6;

            bzero((caddr_t)&ifra6, sizeof(ifra6));
            ifra6.ifra_addr.sin6_len    = sizeof(struct sockaddr_in6);
            ifra6.ifra_addr.sin6_family = AF_INET6;
            ifra6.ifra_addr.sin6_addr   = loopback6;
            ifra6.ifra_prefixmask.sin6_len    = sizeof(struct sockaddr_in6);
            ifra6.ifra_prefixmask.sin6_family = AF_INET6;
            memset(&ifra6.ifra_prefixmask.sin6_addr, 0xff,
                   sizeof(ifra6.ifra_prefixmask.sin6_addr));    /* /128 */
            (void)in6_control((struct socket *)NULL, SIOCAIFADDR_IN6, (caddr_t)&ifra6, ifp);
        } else
            (void)in6_ifevent(NFE_UP, ifp);
    }
}

/*
 * IPV6 STOPS (inet6_stop()): what it has on the interfaces goes, on every
 * interface of the stack - the loopback among them - in the order an
 * interface loses its configuration and leaves (kern/netconfig_iface.c
 * sana_remove_unseen()), for this family alone: the DHCPv6 client is told
 * (NFE_REMOVING); the routers heard on the interface, the routes they
 * gave and the prefixes are forgotten, each with its entry of the routing
 * table (NFE_UNCONFIGURE_PRE); every IPv6 route through the interface is
 * deleted - the neighbour cache entries are freed with the routes they
 * hang on, and the packets that waited in them; then every IPv6 address
 * (NFE_UNCONFIGURE: each stops its duplicate address detection, takes the
 * routes that still used it, and leaves its solicited-node group, ::1 on
 * the loopback among them); then the interface's neighbour discovery
 * state (NFE_DETACHING).
 *
 * Then the group memberships that are left. Those of sockets were left
 * when the sockets were closed - none exists now - and those of the
 * addresses with the addresses; what remains is each interface's own
 * membership of the all-nodes group (in6_join_allnodes()), for which
 * nothing is ever sent, and any that a socket did not leave. The devices
 * stay open, so each is told to forget the group's link address.
 *
 * Returns the number of addresses removed.
 */
int
in6_detach_all(void)
{
    struct ifnet *ifp;
    int deleted = 0;
    spl_t s;

    for(ifp = ifnet_list; ifp != NULL; ifp = ifp->if_next) {
        (void)in6_ifevent(NFE_REMOVING, ifp);
        (void)in6_ifevent(NFE_UNCONFIGURE_PRE, ifp);
        rt_purgeif(ifp, AF_INET6);
        deleted += in6_ifevent(NFE_UNCONFIGURE, ifp);
        (void)in6_ifevent(NFE_DETACHING, ifp);
    }

    s = splimp();
    while(in6_multihead != NULL) {
        struct in6_multi *in6m = in6_multihead;

        in6_multihead = in6m->in6m_next;
        if(in6m->in6m_ifp != NULL)
            in6_mcast_device(in6m->in6m_ifp, &in6m->in6m_addr, 0);
        FREE(in6m, M_IFADDR);
    }
    splx(s);

    /* this file's variables, as the file has them */
    in6_ifaddr = NULL;
    ip6_defhlim = 64;
    ip6_forwarding = 0;
    ip6_v6only = 1;
    ip6_use_tempaddr = 1;
    return (deleted);
}
