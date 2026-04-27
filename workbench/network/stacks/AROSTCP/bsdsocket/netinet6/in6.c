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
#include <net/sana2request.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6.h>
#include <net/rtsock_protos.h>

#include <devices/sana2.h>

struct in6_ifaddr *in6_ifaddr = NULL;
struct ifqueue    ip6intrq    = {0};

int ip6_defhlim    = 64;
int ip6_forwarding = 0;
int ip6_v6only     = 1;   /* sockets default to IPv6-only */
int ip6_use_tempaddr = 1; /* enable privacy extensions by default */

extern struct ifnet loif;

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
int
in6_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
    struct in6_ifreq  *ifr  = (struct in6_ifreq *)data;
    struct in6_aliasreq *ifra = (struct in6_aliasreq *)data;
    struct in6_ifaddr *ia   = NULL, *oia;
    struct ifaddr     *ifa;
    struct mbuf       *m;
    int error = 0;
    spl_t s;

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
            /* allocate new address record */
            m = m_getclr(M_WAIT, MT_IFADDR);
            if(m == NULL)
                return ENOBUFS;
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
        if(ifra->ifra_addr.sin6_family == AF_INET6) {
            error = in6_ifinit(ifp, ia, &ifra->ifra_addr, 0);
            if(error)
                return error;
        }
        ia->ia6_flags    = ifra->ifra_flags;
        ia->ia6_lifetime_vltime = ifra->ifra_lifetime.ia6t_vltime;
        ia->ia6_lifetime_pltime = ifra->ifra_lifetime.ia6t_pltime;
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

    if((error = rtinit(&ia->ia_ifa, RTM_ADD, flags)) == 0)
        ia->ia6_flags |= IFA_ROUTE;

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

    in6_ifscrub(ifp, ia);

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

    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(IN6_ARE_ADDR_EQUAL(in6, &ia->ia_addr.sin6_addr))
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------
 * in6_ifawithifp - find best IPv6 source address for given ifp/dest.
 * ------------------------------------------------------------------ */
struct in6_ifaddr *
in6_ifawithifp(struct ifnet *ifp, struct in6_addr *dst)
{
    struct in6_ifaddr *ia, *besta = NULL;
    int bestscope = -1;
    int dstscope  = in6_addrscope(dst);

    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if(ia->ia6_ifp != ifp)
            continue;
        if(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED))
            continue;
        /* skip deprecated addresses if we have a better candidate */
        if((ia->ia6_ifaflags & IN6_IFF_DEPRECATED) && besta != NULL)
            continue;
        int sc = in6_addrscope(&ia->ia_addr.sin6_addr);
        if(sc == dstscope) {
            /*
             * RFC 4941: prefer non-deprecated temporary addresses
             * for outgoing connections (privacy).
             */
            if(besta == NULL || bestscope != dstscope) {
                besta = ia;
                bestscope = sc;
            } else if((ia->ia6_ifaflags & IN6_IFF_TEMPORARY) &&
                      !(ia->ia6_ifaflags & IN6_IFF_DEPRECATED) &&
                      (!(besta->ia6_ifaflags & IN6_IFF_TEMPORARY) ||
                       (besta->ia6_ifaflags & IN6_IFF_DEPRECATED))) {
                besta = ia;
                bestscope = sc;
            }
            continue;
        }
        if(sc > bestscope) {
            bestscope = sc;
            besta = ia;
        }
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

    ia->ia_ifa.ifa_addr    = (struct sockaddr *)&ia->ia_addr;
    ia->ia_ifa.ifa_netmask = (struct sockaddr *)&ia->ia_prefixmask;
    ia->ia_ifa.ifa_rtrequest = nd6_rtrequest;
    ia->ia6_ifp = ifp;
    ia->ia6_plen = 64;
    ia->ia6_ifaflags = IN6_IFF_AUTOCONF;

    D(bug("[AROSTCP:IN6] %s: calling in6_ifinit\n", __func__));
    (void)in6_ifinit(ifp, ia, &sin6, 0);

    /*
     * Join required IPv6 multicast groups so the NIC accepts the
     * corresponding Ethernet multicast MACs (RFC 4291 §2.8):
     *  - ff02::1        (all-nodes) — needed to receive Router Advertisements
     *  - ff02::1:ffXX:XXXX (solicited-node) — needed for NDP / DAD
     */
    D(bug("[AROSTCP:IN6] %s: joining multicast groups\n", __func__));
    {
        struct in6_addr maddr;

        /* ff02::1 — all-nodes link-local multicast */
        bzero(&maddr, sizeof(maddr));
        maddr.s6_addr[0]  = 0xff;
        maddr.s6_addr[1]  = 0x02;
        maddr.s6_addr[15] = 0x01;
        (void)in6_addmulti(&maddr, ifp);

        /* ff02::1:ffXX:XXXX — solicited-node multicast for our link-local */
        bzero(&maddr, sizeof(maddr));
        maddr.s6_addr[0]  = 0xff;
        maddr.s6_addr[1]  = 0x02;
        maddr.s6_addr[11] = 0x01;
        maddr.s6_addr[12] = 0xff;
        maddr.s6_addr[13] = sin6.sin6_addr.s6_addr[13];
        maddr.s6_addr[14] = sin6.sin6_addr.s6_addr[14];
        maddr.s6_addr[15] = sin6.sin6_addr.s6_addr[15];
        (void)in6_addmulti(&maddr, ifp);
    }

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
        snprintf(ifr6.ifr_name, sizeof(ifr6.ifr_name),
                 "%s%u", ifp->if_name, ifp->if_unit);
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
 * sana_add_mcast6 - tell SANA-II device to accept Ethernet multicast for an
 * IPv6 multicast address via S2_ADDMULTICASTADDRESS.
 */
static int
sana_add_mcast6(struct ifnet *ifp, struct in6_addr *maddr)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;
    struct IOSana2Req *req;
    u_int8_t mac[6];
    int error = 0;

    if(ifp->if_addrlen != 6)
        return 0;  /* not Ethernet — nothing to do */

    in6_map_mcast_macaddr(maddr, mac);

    D(bug("[AROSTCP:IN6] %s: %s%d registering eth mcast %02x:%02x:%02x:%02x:%02x:%02x\n",
          __func__, ifp->if_name, ifp->if_unit,
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    req = CreateIOSana2Req(ssc);
    if(req == NULL) {
        D(bug("[AROSTCP:IN6] %s: CreateIOSana2Req failed\n", __func__));
        return ENOBUFS;
    }

    req->ios2_Req.io_Command = S2_ADDMULTICASTADDRESS;
    bcopy(mac, req->ios2_SrcAddr, 6);
    D(bug("[AROSTCP:IN6] %s: calling DoIO(S2_ADDMULTICASTADDRESS)\n", __func__));
    DoIO((struct IORequest *)req);

    if(req->ios2_Req.io_Error) {
        D(bug("[AROSTCP:IN6] %s: DoIO error=%d\n",
              __func__, req->ios2_Req.io_Error));
        error = EIO;
    } else {
        D(bug("[AROSTCP:IN6] %s: DoIO success\n", __func__));
    }

    DeleteIOSana2Req(req);
    return error;
}

/*
 * sana_del_mcast6 - tell SANA-II device to stop accepting an Ethernet multicast.
 */
static int
sana_del_mcast6(struct ifnet *ifp, struct in6_addr *maddr)
{
    struct sana_softc *ssc = (struct sana_softc *)ifp;
    struct IOSana2Req *req;
    u_int8_t mac[6];
    int error = 0;

    if(ifp->if_addrlen != 6)
        return 0;

    in6_map_mcast_macaddr(maddr, mac);

    req = CreateIOSana2Req(ssc);
    if(req == NULL)
        return ENOBUFS;

    req->ios2_Req.io_Command = S2_DELMULTICASTADDRESS;
    bcopy(mac, req->ios2_SrcAddr, 6);
    DoIO((struct IORequest *)req);

    if(req->ios2_Req.io_Error)
        error = EIO;

    DeleteIOSana2Req(req);
    return error;
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

    /* register Ethernet multicast address with the NIC */
    D(bug("[AROSTCP:IN6] %s: calling sana_add_mcast6\n", __func__));
    if(sana_add_mcast6(ifp, maddr) != 0) {
        __log(LOG_ERR, "in6_addmulti: S2_ADDMULTICASTADDRESS failed for %s%d\n",
              ifp->if_name, ifp->if_unit);
        /* continue anyway — some drivers accept all multicast */
    } else {
        D(bug("[AROSTCP:IN6] %s: sana_add_mcast6 succeeded\n", __func__));
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

    /* Send MLD Done before removing from list */
    mld6_stop_listening(in6m);

    /* unregister Ethernet multicast address from the NIC */
    sana_del_mcast6(in6m->in6m_ifp, &in6m->in6m_addr);

    for(p = &in6_multihead; *p; p = &(*p)->in6m_next) {
        if(*p == in6m) {
            *p = in6m->in6m_next;
            break;
        }
    }
    FREE(in6m, M_IFADDR);
}

/* ==================================================================
 * IPv6 Privacy Extensions (RFC 4941) - Temporary Address Management
 * ================================================================== */

/*
 * Generate a random interface identifier for temporary addresses (RFC 4941).
 * Uses AROS-available entropy sources.
 */
static void
in6_generate_random_iid(struct in6_addr *addr)
{
    u_int32_t *p = (u_int32_t *)&addr->s6_addr[8];
    static u_int32_t rstate1 = 0, rstate2 = 0;

    /* seed from available entropy if needed */
    if(rstate1 == 0) {
        struct timeval _tv;
        GetSysTime(&_tv);
        rstate1 = (u_int32_t)(IPTR)FindTask(NULL) ^ 0xdeadbeef;
        rstate2 = (u_int32_t)(_tv.tv_sec * 1000000 + _tv.tv_usec) ^ 0xcafebabe;
    }

    /* xorshift64 */
    rstate1 ^= rstate1 << 13;
    rstate1 ^= rstate1 >> 7;
    rstate1 ^= rstate1 << 17;
    rstate2 ^= rstate2 << 13;
    rstate2 ^= rstate2 >> 7;
    rstate2 ^= rstate2 << 17;

    p[0] = rstate1;
    p[1] = rstate2;

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
    struct in6_ifaddr *ia, *newia;
    struct in6_aliasreq ifra;
    struct in6_addr tmpaddr;
    struct sockaddr_in6 *sin6;
    struct timeval _tv;
    int error;

    if(!ip6_use_tempaddr)
        return 0;

    if(pr->ndpr_plen != 64)
        return 0;  /* SLAAC temporary addresses require /64 */

    /* build the temporary address: prefix + random IID */
    bzero(&tmpaddr, sizeof(tmpaddr));
    bcopy(&pr->ndpr_prefix.sin6_addr, &tmpaddr, 8);
    in6_generate_random_iid(&tmpaddr);

    /* check for conflicts with existing addresses */
    for(ia = in6_ifaddr; ia; ia = ia->ia_next) {
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
    GetSysTime(&_tv);
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

    /* join solicited-node multicast for the new address */
    {
        struct in6_addr maddr;
        bzero(&maddr, sizeof(maddr));
        maddr.s6_addr[0]  = 0xff;
        maddr.s6_addr[1]  = 0x02;
        maddr.s6_addr[11] = 0x01;
        maddr.s6_addr[12] = 0xff;
        maddr.s6_addr[13] = tmpaddr.s6_addr[13];
        maddr.s6_addr[14] = tmpaddr.s6_addr[14];
        maddr.s6_addr[15] = tmpaddr.s6_addr[15];
        (void)in6_addmulti(&maddr, ifp);
    }

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

    GetSysTime(&_tv);

    for(ia = in6_ifaddr; ia; ia = nia) {
        nia = ia->ia_next;
        if(!(ia->ia6_ifaflags & IN6_IFF_TEMPORARY))
            continue;

        /* check valid lifetime expiry */
        if(ia->ia6_lifetime_expire &&
           (u_int32_t)_tv.tv_sec >= ia->ia6_lifetime_expire) {
            D(bug("[AROSTCP:IN6] %s: removing expired temporary addr\n",
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
