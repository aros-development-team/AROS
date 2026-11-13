/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * if_stf.c - 6in4 (RFC 4213 / SIT) IPv6-in-IPv4 tunnel pseudo-interface.
 *
 * A deviceless pseudo-interface, modelled on BSD gif/stf.  The stack sends
 * IPv6 datagrams to stf_output(), which prepends an IPv4 header (protocol 41)
 * with the configured tunnel endpoints and hands the packet to IPv4's output -
 * so the outer IPv4 packet is routed and ARP-resolved by the existing IPv4
 * stack.  Inbound IPv4 protocol-41 packets arrive via the normal IPv4 input
 * path and are dispatched to stf_input(), which strips the outer header and
 * enqueues the inner IPv6 datagram onto IPv6's input queue.
 *
 * The tunnel belongs to IPv6 and uses IPv4 as a carrier, without naming
 * anything of it or of the transports: it takes IP protocol 41 from
 * whichever IPv4 is part of the running stack (the dispatch table IPv4
 * published with the core, net/netmodule.h: protodispatch_of()) and sends
 * through the output of IPv4's protocol switch (pr_output of the family's
 * base protocol).  Without IPv4 there is no carrier, and a tunnel cannot
 * be made (stf_make() fails and says so).  What it receives goes to the
 * input queue IPv6 registered with the core.  The core does not name the
 * tunnel either: it is a registered kind of tunnel interface (struct
 * nettunnel), which the reader of the interface configuration asks for.
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

#include <sys/protosw.h>
#include <sys/domain.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/netisr.h>
#include <net/netmodule.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>			/* the outer header's layout */

#include <kern/uipc_domain_protos.h>	/* pffindproto() */

#include <net/sana2config.h>
#include <net/if_stf.h>
#include <protos/net/if_protos.h>	/* if_attach(), if_remove() */
#include <protos/net/route_protos.h>	/* rtfree() */

#include <assert.h>
#include <net/netcore_map.h>        /* always the last include */

struct stf_softc *stf_softc_list = NULL;

static int stf_output(struct ifnet *ifp, struct mbuf *m,
                      struct sockaddr *dst, struct rtentry *rt);
static int stf_ioctl(struct ifnet *ifp, int cmd, caddr_t data);

/*
 * The carrier.  stf_ip4out is IPv4's output, as its protocol switch has it:
 * (m, options, route, flags, multicast options), m starting with a struct ip
 * whose ip_len and ip_off are in host byte order.  stf_proto is what IPv4's
 * input finds under protocol 41 once the tunnel has taken the number: only
 * its input is used.  stf_ip6q and stf_ip6isr are IPv6's input queue and
 * the bit that schedules it.  All are set by stf_carrier(), once, so that
 * a packet costs no look-up.
 *
 * They stay good because neither family can stop under a tunnel.  IPv6's
 * queue is this module's own: the tunnel is part of IPv6, and IPv6's stop
 * takes the tunnels down first (stf_detach()).  IPv4 is another module,
 * which the tunnel uses without a socket and without IPv6 needing it: from
 * the moment the tunnel has IPv4's protocol number and output until the
 * last tunnel interface is gone it is counted as a user of IPv4's module
 * (netmodule_hold_family()), so a stop of IPv4 is refused with EBUSY and
 * the user is reported, instead of leaving stf_ip4out and IPv4's dispatch
 * table pointing into code that has left memory.
 */
static int (*stf_ip4out)(void *, ...);
static struct protosw stf_proto;
static struct ifqueue *stf_ip6q;
static int stf_ip6isr;

/*
 * Take IP protocol 41 from IPv4 and find its output, and IPv6's input
 * queue: 0, or an errno when IPv6 (EPFNOSUPPORT) or IPv4 (EAFNOSUPPORT) is
 * not part of the running stack, or IPv4 will not give the number (another
 * protocol has it).
 */
static int
stf_carrier(void)
{
    struct protodispatch *pd4;
    const struct netinput *ni6;
    struct protosw *ip4;
    int error;

    if(stf_ip4out != NULL)
        return 0;
    if((ni6 = netinput_find(AF_INET6)) == NULL)
        return EPFNOSUPPORT;
    /* IPv4 is asked through the core: its table of protocol numbers, as it
     * published it, and its base protocol - none of IPv4's code is named */
    pd4 = protodispatch_of(AF_INET);
    ip4 = pffindproto(PF_INET, 0, 0);		/* the family's base protocol */
    if(pd4 == NULL || ip4 == NULL || ip4->pr_output == NULL)
        return EAFNOSUPPORT;
    stf_proto.pr_domain = ip4->pr_domain;
    stf_proto.pr_protocol = IPPROTO_IPV6;
    stf_proto.pr_input = stf_input;
    if((error = protodispatch_set(pd4, IPPROTO_IPV6, &stf_proto)) != 0)
        return error;
    /* IPv4 has a user now, until stf_release() */
    if((error = netmodule_hold_family(AF_INET, 1)) != 0) {
        (void)protodispatch_clear(pd4, IPPROTO_IPV6, &stf_proto);
        return error;
    }
    stf_ip6q = ni6->ni_queue;
    stf_ip6isr = ni6->ni_isr;
    stf_ip4out = ip4->pr_output;
    return 0;
}

/*
 * The reverse of stf_carrier(), when no tunnel interface is left: IPv4
 * gets its protocol number back and loses its user, and nothing of IPv4's
 * is remembered.
 */
static void
stf_release(void)
{
    struct protodispatch *pd4;

    if(stf_ip4out == NULL)
        return;
    if((pd4 = protodispatch_of(AF_INET)) != NULL)
        (void)protodispatch_clear(pd4, IPPROTO_IPV6, &stf_proto);
    (void)netmodule_hold_family(AF_INET, -1);
    stf_ip4out = NULL;
    stf_ip6q = NULL;
    stf_ip6isr = 0;
    bzero((caddr_t)&stf_proto, sizeof(stf_proto));
}

/*
 * Create and attach a 6in4 tunnel pseudo-interface.  The nt_make of the
 * tunnel kind registered below: called by the reader of the interface
 * configuration (addifent()) when an "interfaces" line requests TUNNEL.
 * src/dst/ttl are the outer IPv4 endpoints, already parsed from the
 * TSRC/TDST/TTL keywords as addresses of the carrier family (AF_INET).
 */
static struct ifnet *
stf_make(struct ssconfig *ifc, const struct sockaddr *srcsa,
         const struct sockaddr *dstsa, int ttl)
{
    struct in_addr src = ((const struct sockaddr_in *)srcsa)->sin_addr;
    struct in_addr dst = ((const struct sockaddr_in *)dstsa)->sin_addr;
    struct stf_softc *sc;
    int error;

    /* no IPv4, no tunnel: there is nothing to carry it */
    if((error = stf_carrier()) != 0) {
        if(error == EAFNOSUPPORT || error == EPFNOSUPPORT)
            __log(LOG_ERR, "stf: 6in4 tunnel %s needs %s, which is not part "
                  "of the running stack\n", ifc->name,
                  error == EAFNOSUPPORT ? "IPv4" : "IPv6");
        else
            __log(LOG_ERR, "stf: 6in4 tunnel %s: IPv4 does not give out IP "
                  "protocol 41 (error %ld)\n", ifc->name, (long)error);
        return NULL;
    }

    sc = (struct stf_softc *)bsd_malloc(sizeof(*sc), M_IFNET, M_WAITOK);
    if(sc == NULL) {
        __log(LOG_ERR, "stf_make: out of memory\n");
        if(stf_softc_list == NULL)
            stf_release();      /* no tunnel: IPv4 is not held for one */
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
 * The 6in4 tunnel as a kind of tunnel interface (net/netmodule.h): the
 * core's configuration reader finds it by asking for a tunnel, and names
 * neither this file nor IPv6.  Registered by IPv6's module start
 * (inet6_start(), netinet6/in6_proto.c), of which the tunnel is a part.
 */
static void stf_seen(struct ifnet *ifp, ULONG gen);
static void stf_prune(ULONG gen);
static const struct nettunnel stf_tunnel = { "6in4", AF_INET, stf_make, stf_seen, stf_prune };

void
stf_attach(void)
{
    (void)nettunnel_register(&stf_tunnel);
}

/*
 * IPv6 stops (inet6_stop(), netinet6/in6_proto.c), and the tunnel with
 * it: called first, while both families still work. No tunnel can be made
 * any more; every tunnel interface is taken out of the running stack as an
 * interface that left the configuration is (if_remove(), net/if.c: its
 * addresses and routes of every family, each family's and the transports'
 * references to it) and freed with the IPv4 route it kept; then IPv4 gets
 * protocol 41 back and is no longer held (stf_release()).
 *
 * The configuration still has the tunnel's line: the next reload makes
 * the tunnel again if IPv6 runs then, and refuses the line, with a
 * message, if it does not.
 */
/* take one tunnel, already unlinked from stf_softc_list, out and free it */
static void
stf_remove(struct stf_softc *sc, const char *why)
{
    __log(LOG_NOTICE, "stf: 6in4 tunnel %s%ld removed (%s)\n",
          sc->sc_name, (long)sc->sc_if.if_unit, why);
    if_remove(&sc->sc_if);
    if(sc->sc_route.ro_rt != NULL)
        RTFREE(sc->sc_route.ro_rt);
    bsd_free((caddr_t)sc, M_IFNET);
}

void
stf_detach(void)
{
    struct stf_softc *sc;

    (void)nettunnel_unregister(&stf_tunnel);
    while((sc = stf_softc_list) != NULL) {
        stf_softc_list = sc->sc_next;
        stf_remove(sc, "IPv6 stops");
    }
    stf_release();
}

/*
 * A reload read the configuration again (nt_seen/nt_prune, net/netmodule.h):
 * a tunnel whose line is still there is marked with the reload's generation,
 * one whose line is gone is removed as stf_detach() removes them, and when
 * no tunnel is left IPv4 gets protocol 41 back and is no longer held - so
 * IPv4 can be stopped again.
 */
static void
stf_seen(struct ifnet *ifp, ULONG gen)
{
    struct stf_softc *sc;

    for(sc = stf_softc_list; sc != NULL; sc = sc->sc_next)
        if(&sc->sc_if == ifp) {
            sc->sc_seen = gen;
            return;
        }
}

static void
stf_prune(ULONG gen)
{
    struct stf_softc *sc, **prev = &stf_softc_list;

    while((sc = *prev) != NULL) {
        if(sc->sc_seen == gen) {
            prev = &sc->sc_next;
            continue;
        }
        *prev = sc->sc_next;
        stf_remove(sc, "no longer in the configuration");
    }
    if(stf_softc_list == NULL)
        stf_release();
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
 * IPv4's output.  Mirrors a raw socket's prepend-and-send pattern.
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
     * same tunnel, IPv4's output would call us re-entrantly.  Refuse. */
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
    ip->ip_len = m->m_pkthdr.len;		/* host order; IPv4's output byteswaps */
    ip->ip_src = sc->sc_src;
    ip->ip_dst = sc->sc_dst;
    ip->ip_ttl = sc->sc_ttl;
    /* ip_v / ip_hl / ip_id / ip_sum are filled by IPv4's output */

    sc->sc_if.if_opackets++;
    sc->sc_if.if_obytes += m->m_pkthdr.len;

    sc->sc_busy = 1;
    error = (*stf_ip4out)(m, (struct mbuf *)0, &sc->sc_route, 0,
                          (void *)0 /* no multicast options */);
    sc->sc_busy = 0;

    if(error)
        sc->sc_if.if_oerrors++;

    return error;
}

/*
 * Input: an inbound IPv4 protocol-41 packet.  The pr_input IPv4 was given
 * for IPPROTO_IPV6 (stf_carrier()), so it is called from ipintr as
 * pr_input(m, hlen) with the IPv4 header still present at mtod(m).  Strip
 * the outer header and enqueue the inner IPv6 datagram on IPv6's input
 * queue, as an interface that received it would.
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

    /* IPv6's input queue (stf_carrier(): there is one, or no tunnel) */
    s = splimp();
    if(IF_QFULL(stf_ip6q)) {
        IF_DROP(stf_ip6q);
        sc->sc_if.if_ierrors++;
        m_freem(m);
    } else {
        IF_ENQUEUE(stf_ip6q, m);
        schednetisr(stf_ip6isr);
    }
    splx(s);
}
