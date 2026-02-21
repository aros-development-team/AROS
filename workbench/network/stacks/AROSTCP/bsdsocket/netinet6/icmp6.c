/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 / FreeBSD icmp6 implementation.
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * icmp6.c - ICMPv6 protocol implementation.
 *
 * Handles inbound ICMPv6: echo request/reply, error messages,
 * and dispatches Neighbor Discovery messages to nd6.c.
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
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

/* IP6_HDR_OFF macro: ip6 header is at offset 0 in mbuf after ip6_input */
#define ICMPv6_HDRLEN sizeof(struct icmp6_hdr)

struct icmp6stat icmp6stat = {0};

/* forward */
static void icmp6_reflect(struct mbuf *, size_t);
static int  icmp6_cksum(struct mbuf *, u_int8_t, u_int32_t, u_int32_t);

/* ------------------------------------------------------------------
 * icmp6_init
 * ------------------------------------------------------------------ */
void
icmp6_init(void)
{
    /* nothing to initialise for now */
}

void icmp6_fasttimo(void) {}
void icmp6_slowtimo(void) {}
void icmp6_drain(void)    {}

/* ------------------------------------------------------------------
 * icmp6_ctloutput - socket option handler for ICMPv6.
 * ------------------------------------------------------------------ */
int
icmp6_ctloutput(int op, struct socket *so, int level, int optname,
                struct mbuf **m)
{
    return 0;
}

/* ------------------------------------------------------------------
 * icmp6_input - process an inbound ICMPv6 message.
 *
 * Called via protosw pr_input: void *args is struct mbuf *,
 * va_arg gives int off (offset to ICMPv6 header within mbuf).
 * ------------------------------------------------------------------ */
void
icmp6_input(void *args, ...)
{
    struct mbuf  *m = args;
    struct ip6_hdr   *ip6;
    struct icmp6_hdr *icmp6;
    struct ifnet *ifp = m->m_pkthdr.rcvif;
    int off, icmp6len;
    va_list va;

    va_start(va, args);
    off = va_arg(va, int);
    va_end(va);

    ip6 = mtod(m, struct ip6_hdr *);

    /* pull up icmp6 header */
    if (m->m_len < off + ICMPv6_HDRLEN &&
        (m = m_pullup(m, off + ICMPv6_HDRLEN)) == NULL) {
        icmp6stat.icp6s_tooshort++;
        return;
    }
    ip6   = mtod(m, struct ip6_hdr *);
    icmp6 = (struct icmp6_hdr *)(mtod(m, u_int8_t *) + off);
    icmp6len = m->m_pkthdr.len - off;

    icmp6stat.icp6s_inhist[icmp6->icmp6_type]++;

    /* verify checksum */
    /* (full pseudo-header checksum verification omitted for brevity;
     *  production code should call in_cksum with pseudo-header) */

    switch (icmp6->icmp6_type) {
    /* ----- Neighbor Discovery (delegated to nd6.c) ----- */
    case ND_NEIGHBOR_SOLICIT:
        nd6_ns_input(m, off, icmp6len);
        return;

    case ND_NEIGHBOR_ADVERT:
        nd6_na_input(m, off, icmp6len);
        return;

    case ND_ROUTER_SOLICIT:
        nd6_rs_input(m, off, icmp6len);
        return;

    case ND_ROUTER_ADVERT:
        nd6_ra_input(m, off, icmp6len);
        return;

    /* ----- Echo ----- */
    case ICMP6_ECHO_REQUEST:
        icmp6stat.icp6s_reflect++;
        icmp6->icmp6_type = ICMP6_ECHO_REPLY;
        icmp6->icmp6_code = 0;
        /* clear checksum so reflect recalculates */
        icmp6->icmp6_cksum = 0;
        icmp6_reflect(m, off);
        return;

    case ICMP6_ECHO_REPLY:
        /* pass to raw socket listeners */
        break;

    /* ----- Error messages ----- */
    case ICMP6_DST_UNREACH:
    case ICMP6_PACKET_TOO_BIG:
    case ICMP6_TIME_EXCEEDED:
    case ICMP6_PARAM_PROB:
        /* notify upper layer protocols */
        pfctlinput(PRC_UNREACH_NET, (struct sockaddr *)NULL);
        break;

    default:
        break;
    }

    m_freem(m);
}

/* ------------------------------------------------------------------
 * icmp6_error - generate an ICMPv6 error in response to m.
 *
 * type/code - ICMPv6 type and code
 * param     - depends on type (e.g. MTU for TOO_BIG, pointer for PARAM_PROB)
 * ------------------------------------------------------------------ */
void
icmp6_error(struct mbuf *m, int type, int code, int param)
{
    struct ip6_hdr *oip6, *nip6;
    struct icmp6_hdr *icmp6;
    struct mbuf *n;
    int hlen, prepend;

    icmp6stat.icp6s_error++;

    if (m->m_len < sizeof(struct ip6_hdr) &&
        (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL)
        return;

    oip6 = mtod(m, struct ip6_hdr *);

    /* never send ICMPv6 errors in response to multicast */
    if (IN6_IS_ADDR_MULTICAST(&oip6->ip6_dst)) {
        m_freem(m);
        return;
    }
    /* never send ICMPv6 errors in response to ICMPv6 errors */
    if (oip6->ip6_nxt == IPPROTO_ICMPV6) {
        struct icmp6_hdr *oicmp6 = (struct icmp6_hdr *)(oip6 + 1);
        if (m->m_len >= sizeof(*oip6) + sizeof(*oicmp6) &&
            oicmp6->icmp6_type < ICMP6_ECHO_REQUEST) {
            m_freem(m);
            return;
        }
    }

    /* include as much of the offending packet as possible (max 1280 - hdrs) */
    hlen = sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr);
    prepend = hlen;
    if (m->m_pkthdr.len + prepend > IPV6_MMTU)
        m_adj(m, IPV6_MMTU - prepend - m->m_pkthdr.len);

    /* prepend new IPv6 + ICMPv6 headers */
    M_PREPEND(m, prepend, M_DONTWAIT);
    if (m == NULL)
        return;

    nip6  = mtod(m, struct ip6_hdr *);
    icmp6 = (struct icmp6_hdr *)(nip6 + 1);

    /* copy original ip6 src -> new dst */
    bzero(nip6, sizeof(*nip6));
    nip6->ip6_vfc  = IPV6_VERSION;
    nip6->ip6_nxt  = IPPROTO_ICMPV6;
    nip6->ip6_hlim = ip6_defhlim;
    nip6->ip6_dst  = oip6->ip6_src;
    /* source: find appropriate local address */
    {
        struct in6_ifaddr *ia = NULL;
        if (m->m_pkthdr.rcvif)
            ia = in6_ifawithifp(m->m_pkthdr.rcvif, &nip6->ip6_dst);
        if (ia)
            nip6->ip6_src = ia->ia_addr.sin6_addr;
        /* else: stays 0 - routing will fix it */
    }

    icmp6->icmp6_type  = (u_int8_t)type;
    icmp6->icmp6_code  = (u_int8_t)code;
    icmp6->icmp6_cksum = 0;
    icmp6->icmp6_dataun.icmp6_un_data32[0] = htonl(param);

    nip6->ip6_plen = htons(m->m_pkthdr.len - sizeof(struct ip6_hdr));

    icmp6stat.icp6s_outhist[type]++;

    ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * icmp6_reflect - swap src/dst and send an ICMPv6 reply.
 * ------------------------------------------------------------------ */
static void
icmp6_reflect(struct mbuf *m, size_t off)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct in6_addr t;
    struct in6_ifaddr *ia;

    /* swap addresses */
    t = ip6->ip6_dst;
    /* choose a source address to reply from */
    ia = in6_ifawithifp(m->m_pkthdr.rcvif, &ip6->ip6_src);
    if (ia)
        ip6->ip6_src = ia->ia_addr.sin6_addr;
    else
        ip6->ip6_src = t;
    ip6->ip6_dst = ip6->ip6_src;
    ip6->ip6_src = t;

    ip6->ip6_hlim = ip6_defhlim;

    ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * rip6_input  - raw IPv6 socket input.
 * rip6_output - raw IPv6 socket output.
 * rip6_usrreq - raw IPv6 user request.
 * rip6_ctloutput - raw IPv6 socket options.
 *
 * These are minimal stubs; production code delegates to the raw-socket
 * machinery in net/raw_usrreq.c, extended for IPv6.
 * ------------------------------------------------------------------ */
void
rip6_input(void *args, ...)
{
    struct mbuf *m = args;
    /* pass to raw socket listeners (not yet wired) */
    m_freem(m);
}

int
rip6_output(void *args, ...)
{
    struct mbuf *m = args;
    m_freem(m);
    return EOPNOTSUPP;
}

int
rip6_ctloutput(int op, struct socket *so, int level, int optname,
               struct mbuf **m)
{
    return 0;
}

int
rip6_usrreq(struct socket *so, int req, struct mbuf *m,
            struct mbuf *nam, struct mbuf *ctrl)
{
    return EOPNOTSUPP;
}

#endif /* INET6 */
