/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * if_stf.c - 6in4 (RFC 4213 / SIT) IPv6-in-IPv4 tunnel pseudo-interface.
 *
 * A deviceless pseudo-interface, modelled on BSD gif/stf.  The stack sends
 * IPv6 datagrams to stf_output(), which prepends an IPv4 header (protocol 41)
 * with the configured tunnel endpoints and hands the packet to ip_output() -
 * so the outer IPv4 packet is routed and ARP-resolved by the existing IPv4
 * stack.  Inbound IPv4 protocol-41 packets arrive via the normal IPv4 input
 * path and are dispatched to stf_input() (registered in inetsw[]), which
 * strips the outer header and enqueues the inner IPv6 datagram onto ip6intrq.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <conf.h>

#include <exec/errors.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>

#include <sys/synch.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/netisr.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_output_protos.h>

#if INET6
#include <netinet6/in6_var.h>		/* ip6intrq */
#endif

#include <net/sana2config.h>
#include <net/if_stf.h>

#include <assert.h>

struct stf_softc *stf_softc_list = NULL;

static int stf_output(struct ifnet *ifp, struct mbuf *m,
                      struct sockaddr *dst, struct rtentry *rt);
static int stf_ioctl(struct ifnet *ifp, int cmd, caddr_t data);

/*
 * Create and attach a 6in4 tunnel pseudo-interface.  Called from addifent()
 * when an "interfaces" line requests TUNNEL.  src/dst/ttl are the outer IPv4
 * endpoints, already parsed from the TSRC/TDST/TTL keywords.
 */
struct ifnet *
stf_make(struct ssconfig *ifc, struct in_addr src, struct in_addr dst, UBYTE ttl)
{
    struct stf_softc *sc;

    sc = (struct stf_softc *)bsd_malloc(sizeof(*sc), M_IFNET, M_WAITOK);
    if(sc == NULL) {
        __log(LOG_ERR, "stf_make: out of memory\n");
        return NULL;
    }
    aligned_bzero_const(sc, sizeof(*sc));

    sc->sc_src = src;
    sc->sc_dst = dst;
    sc->sc_ttl = ttl ? ttl : 64;

    sc->sc_if.if_name = sc->sc_name;
    {
        size_t nlen = strnlen(ifc->name, IFNAMSIZ - 1);
        memcpy(sc->sc_name, ifc->name, nlen);
        sc->sc_name[nlen] = '\0';
    }
    sc->sc_if.if_unit    = ifc->unit;
    sc->sc_if.if_mtu     = 1480;	/* 1500 - 20 (outer IPv4 header) */
    sc->sc_if.if_flags   = IFF_UP | IFF_POINTOPOINT | IFF_DRV_RUNNING;
    sc->sc_if.if_type    = IFT_TUNNEL;
    sc->sc_if.if_addrlen = 0;		/* no link-layer address */
    sc->sc_if.if_output  = stf_output;
    sc->sc_if.if_ioctl   = stf_ioctl;
    sc->sc_if.if_query   = NULL;

    if_attach(&sc->sc_if);
    ifinit();

    sc->sc_next = stf_softc_list;
    stf_softc_list = sc;

    __log(LOG_INFO, "stf: 6in4 tunnel %s%ld attached (ttl %ld)\n",
          sc->sc_name, (long)sc->sc_if.if_unit, (long)sc->sc_ttl);

    return &sc->sc_if;
}

/*
 * Interface ioctl.  A tunnel has no hardware to configure - accept the
 * address-setting ioctls (they arrive via in_control/in6_control while the
 * IPv4/IPv6 address is being bound) and keep the interface up.
 */
static int
stf_ioctl(struct ifnet *ifp, int cmd, caddr_t data)
{
    int error = 0;
    spl_t s = splimp();

    switch(cmd) {
    case SIOCSIFADDR:
    case SIOCAIFADDR:
#if INET6
    case SIOCAIFADDR_IN6:
#endif
    case SIOCSIFDSTADDR:
        ifp->if_flags |= IFF_UP | IFF_DRV_RUNNING;
        break;

    case SIOCSIFFLAGS:
        break;

    default:
        error = EINVAL;
        break;
    }

    splx(s);
    return error;
}

/*
 * Output: encapsulate an IPv6 datagram in IPv4 (protocol 41) and hand it to
 * ip_output().  Mirrors rip_output()'s prepend-and-send pattern.
 * Called (at splimp) from ip6_output via ifp->if_output; m starts at the
 * IPv6 header and m_pkthdr.len is the full IPv6 length.
 */
static int
stf_output(struct ifnet *ifp, struct mbuf *m, struct sockaddr *dst,
           struct rtentry *rt)
{
    struct stf_softc *sc = (struct stf_softc *)ifp;
    struct ip *ip;
    int error;

    if(!(ifp->if_flags & IFF_UP)) {
        m_freem(m);
        return ENETDOWN;
    }

    /* Loop protection: if the outer IPv4 packet would route back into this
     * same tunnel, ip_output() would call us re-entrantly.  Refuse. */
    if(sc->sc_busy) {
        m_freem(m);
        sc->sc_if.if_oerrors++;
        return ELOOP;
    }

    /* Prepend the outer IPv4 header. */
    M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
    if(m == NULL) {
        sc->sc_if.if_oerrors++;
        return ENOBUFS;
    }

    ip = mtod(m, struct ip *);
    ip->ip_tos = 0;
    ip->ip_off = 0;
    ip->ip_p   = IPPROTO_IPV6;			/* 41 */
    ip->ip_len = m->m_pkthdr.len;		/* host order; ip_output byteswaps */
    ip->ip_src = sc->sc_src;
    ip->ip_dst = sc->sc_dst;
    ip->ip_ttl = sc->sc_ttl;
    /* ip_v / ip_hl / ip_id / ip_sum are filled by ip_output() */

    sc->sc_if.if_opackets++;
    sc->sc_if.if_obytes += m->m_pkthdr.len;

    sc->sc_busy = 1;
    error = ip_output(m, (struct mbuf *)0, &sc->sc_route, 0);
    sc->sc_busy = 0;

    if(error)
        sc->sc_if.if_oerrors++;

    return error;
}

/*
 * Input: an inbound IPv4 protocol-41 packet.  Registered as the inetsw[]
 * pr_input for IPPROTO_IPV6, so it is called from ipintr as pr_input(m, hlen)
 * with the IPv4 header still present at mtod(m).  Strip the outer header and
 * enqueue the inner IPv6 datagram exactly like sana_ip6_read().
 */
void
stf_input(void *arg, ...)
{
    struct mbuf *m = (struct mbuf *)arg;
    struct ip *ip;
    struct stf_softc *sc;
    int hlen;
    spl_t s;

    if(m == NULL)
        return;

    ip = mtod(m, struct ip *);
    hlen = ip->ip_hl << 2;

    /* Match the tunnel by its remote endpoint: outer src == our remote
     * (sc_dst).  We deliberately do NOT also require the outer dst to equal
     * our local endpoint - when the tunnel runs behind NAT the outer dst has
     * been rewritten (to the LAN address, or left as the public one depending
     * on the router), so matching on the remote source only keeps decap
     * working in both cases.  A single tunnel per remote makes this unambiguous.
     * Addresses are in network order in the header and in sc. */
    for(sc = stf_softc_list; sc != NULL; sc = sc->sc_next) {
        if(sc->sc_dst.s_addr == ip->ip_src.s_addr)
            break;
    }

    if(sc == NULL || !(sc->sc_if.if_flags & IFF_UP)) {
        /* Not for any configured tunnel - drop the proto-41 packet. */
        m_freem(m);
        return;
    }

    /* Strip the outer IPv4 header; m now starts at the inner IPv6 header. */
    m_adj(m, hlen);

    sc->sc_if.if_ipackets++;
    sc->sc_if.if_ibytes += m->m_pkthdr.len;
    m->m_pkthdr.rcvif = &sc->sc_if;

#if INET6
    s = splimp();
    if(IF_QFULL(&ip6intrq)) {
        IF_DROP(&ip6intrq);
        sc->sc_if.if_ierrors++;
        m_freem(m);
    } else {
        IF_ENQUEUE(&ip6intrq, m);
        schednetisr(NETISR_IPV6);
    }
    splx(s);
#else
    m_freem(m);
#endif
}
