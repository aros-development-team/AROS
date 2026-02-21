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
 * ip6_input.c - IPv6 input processing.
 *
 * Handles inbound IPv6 packets: basic header validation, hop-limit
 * decrement (when forwarding), extension-header parsing, reassembly
 * dispatch, and demultiplexing to the correct upper-layer protocol.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_var.h>
#include <netinet/in_pcb.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <kern/uipc_domain_protos.h>

/* ------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------ */
struct ip6stat ip6stat = {0};

extern struct domain inet6domain;
extern struct protosw inet6sw[];
u_char ip6_protox[IPPROTO_MAX] = {0};

/*
 * forward declarations (defined in icmp6.c).
 */
void icmp6_error(struct mbuf *, int, int, int);
void ip6_input(struct mbuf *m);

/* ------------------------------------------------------------------
 * ip6_init - called once during domain initialisation.
 * ------------------------------------------------------------------ */
void
ip6_init(void)
{
    struct protosw *pr;
    int i;

    pr = pffindproto(AF_INET6, IPPROTO_RAW, SOCK_RAW);
    if (pr == NULL)
        panic("ip6_init: no raw IPv6");

    /* build protocol dispatch table */
    for (i = 0; i < IPPROTO_MAX; i++)
        ip6_protox[i] = (u_char)(pr - inet6sw);

    for (pr = inet6domain.dom_protosw;
         pr < inet6domain.dom_protoswNPROTOSW;
         pr++) {
        if (pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
            ip6_protox[pr->pr_protocol] = (u_char)(pr - inet6sw);
    }

    nd6_init();

    ip6intrq.ifq_maxlen = IFQ_MAXLEN;
}

/* ------------------------------------------------------------------
 * ip6intr - software interrupt: drain ip6intrq.
 * ------------------------------------------------------------------ */
void
ip6intr(void)
{
    struct mbuf *m;
    spl_t s;

    for (;;) {
        s = splimp();
        IF_DEQUEUE(&ip6intrq, m);
        splx(s);
        if (m == NULL)
            break;
        ip6_input(m);
    }
}

/* ------------------------------------------------------------------
 * ip6_input - process a single inbound IPv6 packet.
 *
 * m->m_pkthdr.rcvif must be set to the receiving interface.
 * ------------------------------------------------------------------ */
void
ip6_input(struct mbuf *m)
{
    struct ip6_hdr *ip6;
    struct ifnet   *ifp = m->m_pkthdr.rcvif;
    struct in6_ifaddr *ia = NULL;
    struct protosw *pr;
    u_int8_t nxt;
    int off, plen;

    ip6stat.ip6s_total++;

    /* pull up to minimum header size */
    if (m->m_len < sizeof(struct ip6_hdr) &&
        (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
        ip6stat.ip6s_toosmall++;
        return;
    }
    ip6 = mtod(m, struct ip6_hdr *);

    /* version check */
    if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
        ip6stat.ip6s_badvers++;
        goto bad;
    }

    plen = ntohs(ip6->ip6_plen);
    if (m->m_pkthdr.len < sizeof(struct ip6_hdr) + plen) {
        ip6stat.ip6s_tooshort++;
        goto bad;
    }
    /* trim any trailing junk */
    if (m->m_pkthdr.len > sizeof(struct ip6_hdr) + plen)
        m_adj(m, sizeof(struct ip6_hdr) + plen - m->m_pkthdr.len);

    /* discard unspecified source except when link-local */
    if (IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src) &&
        !IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
        ip6stat.ip6s_badscope++;
        goto bad;
    }

    /* check if destination is one of our addresses */
    for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
        if (IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &ia->ia_addr.sin6_addr))
            goto ours;
        /* also accept solicited-node multicast for our addr */
        if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
            /* ff02::1:ffXX:XXXX */
            if (ip6->ip6_dst.s6_addr[0] == 0xff &&
                ip6->ip6_dst.s6_addr[1] == 0x02 &&
                ip6->ip6_dst.s6_addr[11] == 0x01 &&
                ip6->ip6_dst.s6_addr[12] == 0xff &&
                ip6->ip6_dst.s6_addr[13] == ia->ia_addr.sin6_addr.s6_addr[13] &&
                ip6->ip6_dst.s6_addr[14] == ia->ia_addr.sin6_addr.s6_addr[14] &&
                ip6->ip6_dst.s6_addr[15] == ia->ia_addr.sin6_addr.s6_addr[15])
                goto ours;
        }
    }

    /* accept all-nodes multicast ff02::1 */
    if (ip6->ip6_dst.s6_addr[0] == 0xff &&
        ip6->ip6_dst.s6_addr[1] == 0x02 &&
        ip6->ip6_dst.s6_addr[15] == 0x01)
        goto ours;

    /* forwarding */
    if (ip6_forwarding) {
        if (ip6->ip6_hlim <= 1) {
            /* hop limit exceeded */
            icmp6_error(m, ICMP6_TIME_EXCEEDED,
                        ICMP6_TIME_EXCEED_TRANSIT, 0);
            return;
        }
        ip6->ip6_hlim--;
        /* route and forward (not yet implemented) */
        ip6stat.ip6s_cantforward++;
    } else {
        ip6stat.ip6s_cantforward++;
    }
    m_freem(m);
    return;

ours:
    ip6stat.ip6s_delivered++;

    /* walk extension headers and dispatch */
    off = sizeof(struct ip6_hdr);
    nxt = ip6->ip6_nxt;

    /* simple extension header skip loop */
    while (1) {
        struct ip6_ext *ext;
        int extlen;

        switch (nxt) {
        case IPPROTO_HOPOPTS:
        case IPPROTO_ROUTING:
        case IPPROTO_DSTOPTS:
            if (m->m_len < off + sizeof(struct ip6_ext) &&
                (m = m_pullup(m, off + sizeof(struct ip6_ext))) == NULL) {
                ip6stat.ip6s_tooshort++;
                return;
            }
            ext = (struct ip6_ext *)(mtod(m, u_int8_t *) + off);
            extlen = (ext->ip6e_len + 1) * 8;
            nxt = ext->ip6e_nxt;
            off += extlen;
            break;

        case IPPROTO_FRAGMENT:
            /* fragment reassembly not yet implemented; drop */
            ip6stat.ip6s_fragments++;
            m_freem(m);
            return;

        case IPPROTO_NONE:
            m_freem(m);
            return;

        default:
            goto deliver;
        }
    }

deliver:
    /* dispatch to upper-layer protocol */
    pr = &inet6sw[ip6_protox[nxt]];
    if (pr->pr_input)
        (*pr->pr_input)(m, off, nxt);
    else
        m_freem(m);
    return;

bad:
    m_freem(m);
}

#endif /* INET6 */
