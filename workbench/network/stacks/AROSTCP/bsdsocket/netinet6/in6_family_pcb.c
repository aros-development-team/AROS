/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the PCB layer (netinet/in_pcb.c: inpcb_xxx()) asks of IPv6: the
 * IPv6 family's implementations of the "PCB layer" operations of struct
 * inetfamily (netinet/inetfamily.h). Everything here is what IPv6 alone
 * does with a local or a foreign address - its socket address, zones,
 * which addresses are this machine's and usable, the source address for a
 * destination, how it presents an IPv4 address. Who may share a port, the
 * hash, look-up and notification are not here: they are the same for
 * every family.
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
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/inetfamily.h>
#include <net/netcore_map.h>        /* always the last include */


/* ------------------------------------------------------------------ *
 * Zones (RFC 4007, Readme section 6.6).
 *
 * A link-scope address names a link only together with a zone, which is
 * an interface index: sin6_scope_id in the API, never embedded in the
 * address. A socket's zone is inp_boundif, the interface SO_BINDTODEVICE
 * also sets (as Linux keeps it in sk_bound_dev_if): bind() and connect()
 * set it from a link-scope address, and getsockname()/getpeername()
 * report it.
 * ------------------------------------------------------------------ */
#define IN6_IS_ADDR_ZONED(a) \
    (IN6_IS_ADDR_LINKLOCAL(a) || IN6_IS_ADDR_MC_LINKLOCAL(a) || \
     IN6_IS_ADDR_MC_NODELOCAL(a))

static struct ifnet *
in6_ifbyindex(u_int32_t index)
{
    struct ifnet *ifp;

    for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
        if((u_int32_t)ifp->if_index == index)
            return ifp;
    return NULL;
}

/*
 * Our address addr, on ifp if that is not NULL, usable as a local address
 * (not tentative or duplicated). NULL if there is none.
 */
struct in6_ifaddr *
in6_ourifa(const struct in6_addr *addr, struct ifnet *ifp)
{
    struct in6_ifaddr *ia;

    for(ia = in6_ifaddr; ia; ia = ia->ia_next)
        if((ifp == NULL || ia->ia6_ifp == ifp) &&
                IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, addr) &&
                !(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED)))
            return ia;
    return NULL;
}

/* The zone a received packet's link-scope address belongs to (0 if none). */
u_int32_t
in6_rcvzone(struct mbuf *m, const struct in6_addr *addr)
{
    if(!IN6_IS_ADDR_ZONED(addr) || m->m_pkthdr.rcvif == NULL)
        return 0;
    return m->m_pkthdr.rcvif->if_index;
}

/*
 * inf_pcbinit: an AF_INET6 socket is restricted to IPv6 until IPV6_V6ONLY
 * is cleared.
 */
void
in6_pcbinit(struct inpcb *inp)
{
    inp->in6p_v6only = 1;
}

/*
 * inf_pcbanyfamily: without IPV6_V6ONLY the socket listens for the other
 * family too (while it has no address: the PCB layer tests that).
 */
int
in6_pcbanyfamily(struct inpcb *inp)
{
    return (!inp->in6p_v6only);
}

/*
 * inf_sockaddr_parse. The zone is the interface sin6_scope_id names, and
 * only a link-scope address has one: on any other the field is ignored.
 */
int
in6_sockaddr_parse(struct mbuf *nam, const void **addr, u_short *port,
                   struct ifnet **zone)
{
    struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);

    if(nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;
    if(sin6->sin6_family != AF_INET6)
        return EAFNOSUPPORT;
    *zone = NULL;
    if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr) && sin6->sin6_scope_id != 0 &&
            (*zone = in6_ifbyindex(sin6->sin6_scope_id)) == NULL)
        return ENXIO;
    *addr = &sin6->sin6_addr;
    *port = sin6->sin6_port;
    return 0;
}

/*
 * inf_mksockaddr: the zone is named only with an address that needs one.
 * Made for every datagram that arrives, so each member is set once (the
 * structure has no others, and no padding) and the address is copied as
 * words: sa and addr are in mbufs, PCBs or variables, all even-aligned.
 */
void
in6_mksockaddr(struct sockaddr *sa, const void *addr, u_int port,
               const struct ifnet *zone)
{
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

    sin6->sin6_len      = sizeof(*sin6);
    sin6->sin6_family   = AF_INET6;
    sin6->sin6_port     = port;
    sin6->sin6_flowinfo = 0;
    inpcb_addrcopy(&sin6->sin6_addr, addr, sizeof(struct in6_addr));
    sin6->sin6_scope_id = (zone != NULL && IN6_IS_ADDR_ZONED(&sin6->sin6_addr))
                          ? zone->if_index : 0;
}

/*
 * inf_addr_from: an IPv4 address is presented in IPv4-mapped form,
 * ::ffff:a.b.c.d (the IPv4 connection of an AF_INET6 socket).
 */
int
in6_addr_from(void *dst, const struct inetfamily *from, const void *addr)
{
    struct in6_addr *a6 = (struct in6_addr *)dst;

    if(from->inf_family != AF_INET)
        return EAFNOSUPPORT;
    bzero(a6, sizeof(*a6));
    a6->s6_addr[10] = 0xff;
    a6->s6_addr[11] = 0xff;
    bcopy(addr, &a6->s6_addr[12], sizeof(struct in_addr));
    return 0;
}

/*
 * inf_pcbzone: the interface the socket is bound to; for an accepted
 * connection, which is not bound, the link its link-local address is on.
 */
const struct ifnet *
in6_pcbzone(struct inpcb *inp)
{
    struct in6_ifaddr *ia;

    if(inp->inp_boundif)
        return inp->inp_boundif;
    if(IN6_IS_ADDR_LINKLOCAL(&inp->inp_laddr6) &&
            (ia = in6_ourifa(&inp->inp_laddr6, NULL)) != NULL)
        return ia->ia6_ifp;
    return NULL;
}

/* inf_ownaddr: ours, and neither tentative nor found to be a duplicate */
int
in6_ownaddr(const void *addr, struct ifnet *zone)
{
    const struct in6_addr *a6 = (const struct in6_addr *)addr;

    return (in6_ourifa(a6, IN6_IS_ADDR_ZONED(a6) ? zone : NULL) != NULL);
}

/*
 * inf_pcbbindaddr. The address must be one of ours (or a group, or
 * unspecified). A link-scope one is looked for in its zone: sin6_scope_id,
 * else the interface the socket is bound to, else any interface that has
 * it - and the socket is then bound to that interface.
 */
int
in6_pcbbindaddr(struct inpcb *inp, struct mbuf *nam, const void **addr,
                u_short *port, struct ifnet **zonep)
{
    struct sockaddr_in6 *sin6;
    struct ifnet *zone = NULL;

    if(nam == NULL)
        return 0;
    if(nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;
    sin6 = mtod(nam, struct sockaddr_in6 *);
    if(sin6->sin6_family != AF_INET6)
        return EAFNOSUPPORT;

    if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr)) {
        zone = inp->inp_boundif;
        if(sin6->sin6_scope_id != 0) {
            zone = in6_ifbyindex(sin6->sin6_scope_id);
            if(zone == NULL ||
                    (inp->inp_boundif && inp->inp_boundif != zone))
                return EADDRNOTAVAIL;
        }
    }
    if(!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
            !IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
        struct in6_ifaddr *ia = in6_ourifa(&sin6->sin6_addr, zone);

        if(ia == NULL)
            return EADDRNOTAVAIL;
        if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr))
            zone = ia->ia6_ifp;
    }
    *addr = &sin6->sin6_addr;
    *port = sin6->sin6_port;
    if(zone != NULL)
        *zonep = zone;
    return 0;
}

/*
 * inf_selectsrc: the source address for dst by RFC 6724, among the
 * addresses of the interface the packet leaves by (in6_ifawithifp()).
 * That interface is oifp; else, for a group, the one IPV6_MULTICAST_IF
 * chose (RFC 3493 section 5.2); else the one the routing table gives.
 * EADDRNOTAVAIL when there is none: a packet is never sent from ::, the
 * peer's ip6_input() would discard it.
 *
 * The route is looked up for this and dropped: struct inpcb's inp_route
 * cannot hold an IPv6 destination (see FAMILY-INTERFACE.md, difference 7).
 */
int
in6_selectsrc(struct inpcb *inp, const void *dst, struct ifnet *oifp, void *src)
{
    struct in6_addr *dst6 = (struct in6_addr *)dst;
    struct in6_ifaddr *ia = NULL;

    if(oifp == NULL && inp != NULL && IN6_IS_ADDR_MULTICAST(dst6) &&
            inp->in6p_moptions && inp->in6p_moptions->im6o_multicast_ifp)
        oifp = inp->in6p_moptions->im6o_multicast_ifp;
    if(oifp != NULL)
        ia = in6_ifawithifp(oifp, dst6);
    else {
        struct route_in6 ro;

        bzero((caddr_t)&ro, sizeof(ro));
        ro.ro_dst.sin6_family = AF_INET6;
        ro.ro_dst.sin6_len    = sizeof(ro.ro_dst);
        ro.ro_dst.sin6_addr   = *dst6;
        rtalloc((struct route *)&ro);
        if(ro.ro_rt != NULL) {
            ia = in6_ifawithifp(ro.ro_rt->rt_ifp, dst6);
            RTFREE(ro.ro_rt);
        }
    }
    if(ia == NULL)
        return EADDRNOTAVAIL;
    *(struct in6_addr *)src = ia->ia_addr.sin6_addr;
    return 0;
}

/*
 * inf_pcbladdr. A link-scope peer is on the link its zone names:
 * sin6_scope_id, else the socket's. With neither, the routing table picks
 * the link (the default zone, RFC 4007 section 6). A group with no zone
 * is reached by the IPV6_MULTICAST_IF interface, but the socket is not
 * bound to it.
 *
 * A PCB without a local address gets one here: without it the connection
 * would use the unspecified source (::), which the peer's ip6_input()
 * discards as an invalid scope - so the SYN (and every later segment)
 * would never be accepted.
 */
int
in6_pcbladdr(struct inpcb *inp, struct mbuf *nam, void *laddr,
             struct ifnet **zonep)
{
    struct sockaddr_in6 *sin6;
    struct ifnet *zone = NULL;
    int error;

    if(nam == NULL || nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;
    sin6 = mtod(nam, struct sockaddr_in6 *);
    if(sin6->sin6_family != AF_INET6)
        return EAFNOSUPPORT;
    if(IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
        return EADDRNOTAVAIL;

    if(IN6_IS_ADDR_ZONED(&sin6->sin6_addr)) {
        zone = inp->inp_boundif;
        if(sin6->sin6_scope_id != 0) {
            zone = in6_ifbyindex(sin6->sin6_scope_id);
            if(zone == NULL)
                return ENXIO;
            if(inp->inp_boundif && inp->inp_boundif != zone)
                return EINVAL;
        }
    }

    if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6))
        *(struct in6_addr *)laddr = inp->inp_laddr6;
    else if((error = in6_selectsrc(inp, &sin6->sin6_addr, zone, laddr)) != 0)
        return error;
    if(zone != NULL)
        *zonep = zone;
    return 0;
}

#endif /* INET6 */
