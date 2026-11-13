/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the PCB layer (netinet/in_pcb.c: inpcb_xxx()) asks of IPv4: the
 * IPv4 family's implementations of the "PCB layer" operations of struct
 * inetfamily (netinet/inetfamily.h). Everything here is what IPv4 alone
 * does with a local or a foreign address - its socket address, which
 * addresses are this machine's, the source address for a destination.
 * Who may share a port, the hash, look-up and notification are not here:
 * they are the same for every family.
 *
 * The route and source selection (in_srcia(), in_pcbladdr()) are the code
 * that was in_pcbladdr() in netinet/in_pcb.c:
 *
 * Copyright (C) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (the 4.4BSD licence, as in netinet/in_pcb.c)
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>
#include <net/if_protos.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include "in_var.h"
#include <netinet/ip_var.h>
#include <netinet/inetfamily.h>
#include <net/netcore_map.h>        /* always the last include */

#define	satosin(sa)	((struct sockaddr_in *)(sa))
#define sintosa(sin)	((struct sockaddr *)(sin))
#define ifatoia(ifa)	((struct in_ifaddr *)(ifa))

/* A socket address of nothing but the address a, to look interfaces up by. */
static void
in_sinof(struct sockaddr_in *sin, struct in_addr a)
{
    bzero((caddr_t)sin, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(*sin);
    sin->sin_addr = a;
}

/* inf_pcbinit: a new IPv4 PCB needs nothing from the family */
void
in_pcbinit(struct inpcb *inp)
{
}

/* inf_pcbanyfamily: an IPv4 socket is an IPv4 socket */
int
in_pcbanyfamily(struct inpcb *inp)
{
    return (0);
}

/* inf_sockaddr_parse */
int
in_sockaddr_parse(struct mbuf *nam, const void **addr, u_short *port,
                  struct ifnet **zone)
{
    struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

    if(nam->m_len != sizeof(*sin))
        return (EINVAL);
    if(sin->sin_family != AF_INET)
        return (EAFNOSUPPORT);
    *addr = &sin->sin_addr;
    *port = sin->sin_port;
    *zone = NULL;               /* IPv4 addresses have no zones */
    return (0);
}

/* inf_mksockaddr */
void
in_mksockaddr(struct sockaddr *sa, const void *addr, u_int port,
              const struct ifnet *zone)
{
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;

    bzero((caddr_t)sin, sizeof(*sin));
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(*sin);
    sin->sin_port = port;
    sin->sin_addr = *(const struct in_addr *)addr;
}

/* inf_addr_from: IPv4 has no form for another family's addresses */
int
in_addr_from(void *dst, const struct inetfamily *from, const void *addr)
{
    return (EAFNOSUPPORT);
}

/* inf_pcbzone */
const struct ifnet *
in_pcbzone(struct inpcb *inp)
{
    return (NULL);
}

/*
 * inf_ownaddr: an address of one of our interfaces, or the broadcast
 * address of one (ifa_ifwithaddr() takes both, as bind() always has).
 */
int
in_ownaddr(const void *addr, struct ifnet *zone)
{
    struct sockaddr_in sin;

    in_sinof(&sin, *(const struct in_addr *)addr);
    return (ifa_ifwithaddr(sintosa(&sin)) != 0);
}

/*
 * inf_pcbbindaddr. Nothing can be bound while the machine has no IPv4
 * address at all - not even a port alone. A group, or "any", needs no
 * interface; any other address must be ours. (The family of the socket
 * address is not looked at: old programs fail to initialise it.)
 */
int
in_pcbbindaddr(struct inpcb *inp, struct mbuf *nam, const void **addr,
               u_short *port, struct ifnet **zone)
{
    struct sockaddr_in *sin;

    if(in_ifaddr == 0)
        return (EADDRNOTAVAIL);
    if(nam == NULL)
        return (0);
    sin = mtod(nam, struct sockaddr_in *);
    if(nam->m_len != sizeof(*sin))
        return (EINVAL);
    if(!IN_MULTICAST(ntohl(sin->sin_addr.s_addr)) &&
            sin->sin_addr.s_addr != INADDR_ANY &&
            !in_ownaddr(&sin->sin_addr, NULL))
        return (EADDRNOTAVAIL);
    *addr = &sin->sin_addr;
    *port = sin->sin_port;
    return (0);
}

/*
 * The interface address to send from to dst, for a PCB that has no local
 * address: by the PCB's cached route (looked up here if there is none, or
 * if it leads elsewhere), else by the interface that is on dst's network,
 * else the primary address. A group goes out by the interface chosen with
 * IP_MULTICAST_IF, and so takes its address.
 */
static int
in_srcia(struct inpcb *inp, struct in_addr dst, struct in_ifaddr **iap)
{
    struct in_ifaddr *ia = (struct in_ifaddr *)0;
    register struct route *ro;
    struct sockaddr_in *rodst_saddr;

    /*
     * If route is known or can be allocated now,
     * our src addr is taken from the i/f, else punt.
     */
    ro = &inp->inp_route;
    rodst_saddr = (struct sockaddr_in *)&ro->ro_dst;

    if(ro->ro_rt &&
            (rodst_saddr->sin_addr.s_addr != dst.s_addr ||
             inp->inp_socket->so_options & SO_DONTROUTE)) {
        RTFREE(ro->ro_rt);
        ro->ro_rt = (struct rtentry *)0;
    }
    if((inp->inp_socket->so_options & SO_DONTROUTE) == 0 &&  /*XXX*/
            (ro->ro_rt == (struct rtentry *)0 ||
             ro->ro_rt->rt_ifp == (struct ifnet *)0)) {
        /* No route yet, so try to acquire one */
        ro->ro_dst.sa_family = AF_INET;
        ro->ro_dst.sa_len = sizeof(struct sockaddr_in);
        rodst_saddr->sin_addr = dst;
        rtalloc(ro);
    }
    /*
     * If we found a route, use the address
     * corresponding to the outgoing interface
     * unless it is the loopback (in case a route
     * to our address on another net goes to loopback).
     */
    if(ro->ro_rt && !(ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK))
        ia = ifatoia(ro->ro_rt->rt_ifa);
    if(ia == 0) {
        struct sockaddr_in sin;

        in_sinof(&sin, dst);
        ia = ifatoia(ifa_ifwithdstaddr(sintosa(&sin)));
        if(ia == 0)
            ia = ifatoia(ifa_ifwithnet(sintosa(&sin)));
        if(ia == 0)
            ia = in_ifaddr;
        if(ia == 0)
            return (EADDRNOTAVAIL);
    }
#ifdef ENABLE_MULTICAST
    /*
     * If the destination address is multicast and an outgoing
     * interface has been set as a multicast option, use the
     * address of that interface as our source address.
     */
    if(IN_MULTICAST(ntohl(dst.s_addr)) &&
            inp->inp_moptions != NULL) {
        struct ip_moptions *imo;
        struct ifnet *ifp;

        imo = inp->inp_moptions;
        if(imo->imo_multicast_ifp != NULL) {
            ifp = imo->imo_multicast_ifp;
            for(ia = in_ifaddr; ia; ia = ia->ia_next)
                if(ia->ia_ifp == ifp)
                    break;
            if(ia == 0)
                return (EADDRNOTAVAIL);
        }
    }
#endif
    *iap = ia;
    return (0);
}

/*
 * inf_selectsrc. With the interface named (a socket bound to one,
 * SO_BINDTODEVICE) the source is that interface's own address - "any" if
 * it has none yet, which is how a DHCP client sends; otherwise in_srcia()
 * decides, which needs the PCB (its route, its multicast options).
 */
int
in_selectsrc(struct inpcb *inp, const void *dst, struct ifnet *oifp, void *src)
{
    struct in_ifaddr *ia;
    int error;

    if(oifp != NULL) {
        for(ia = in_ifaddr; ia; ia = ia->ia_next)
            if(ia->ia_ifp == oifp)
                break;
        ((struct in_addr *)src)->s_addr = ia ? IA_SIN(ia)->sin_addr.s_addr : INADDR_ANY;
        return (0);
    }
    if(inp == NULL)
        return (EADDRNOTAVAIL);
    if((error = in_srcia(inp, *(const struct in_addr *)dst, &ia)) != 0)
        return (error);
    *(struct in_addr *)src = IA_SIN(ia)->sin_addr;
    return (0);
}

/*
 * inf_pcbladdr.
 *
 *   Do some validity-checking on the remote
 *   address (in mbuf 'nam') and then determine local host address
 *   (i.e., which interface) to use to access that remote host.
 *
 * The local address is written at laddr: the PCB's own if it has one,
 * else the address of the interface chosen. The PCB is not changed, and
 * no PCB is looked up: the caller does that.
 */
int
in_pcbladdr(struct inpcb *inp, struct mbuf *nam, void *laddr,
            struct ifnet **zone)
{
    struct in_ifaddr *ia;
    register struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);
    int error;

    if(nam->m_len != sizeof(*sin))
        return (EINVAL);
    if(sin->sin_family != AF_INET)
        return (EAFNOSUPPORT);
    if(sin->sin_port == 0)
        return (EADDRNOTAVAIL);
    if(in_ifaddr) {
        /*
         * If the destination address is INADDR_ANY,
         * use the primary local address.
         * If the supplied address is INADDR_BROADCAST,
         * and the primary interface supports broadcast,
         * choose the broadcast address for that interface.
         */
        if(sin->sin_addr.s_addr == INADDR_ANY)
            sin->sin_addr = IA_SIN(in_ifaddr)->sin_addr;
        else if(sin->sin_addr.s_addr == (u_long)INADDR_BROADCAST &&
                (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
            sin->sin_addr = satosin(&in_ifaddr->ia_broadaddr)->sin_addr;
    }
    if(inp->inp_laddr.s_addr == INADDR_ANY) {
        if((error = in_srcia(inp, sin->sin_addr, &ia)) != 0)
            return (error);
        *(struct in_addr *)laddr = ia->ia_addr.sin_addr;
    } else
        *(struct in_addr *)laddr = inp->inp_laddr;
    return (0);
}
