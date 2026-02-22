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
 * ip6_output.c - IPv6 output processing.
 *
 * Handles outbound IPv6 packets: route lookup, hop-limit insertion,
 * extension header insertion, fragmentation if required, and final
 * hand-off to the network interface.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/synch.h>
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

/* ------------------------------------------------------------------
 * ip6_output - send an IPv6 datagram.
 *
 * Called via protosw pr_output: void *args is struct mbuf *m,
 * followed by va args: struct mbuf *opt, struct route *ro,
 * int flags, struct ip6_moptions *im6o, struct ifnet **ifpp,
 * struct inpcb *inp.
 *
 * Returns 0 on success or an errno.
 * ------------------------------------------------------------------ */
int
ip6_output(void *args, ...)
{
    struct mbuf *m = args;
    struct mbuf *opt;
    int flags;
    struct ip6_moptions *im6o;
    struct ifnet **ifpp;
    struct inpcb *inp;
    va_list va;

    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct ifnet   *ifp = NULL;
    struct route_in6 iproute;
    struct route    *ro;
    struct sockaddr_in6 *dst;
    struct rtentry *rt = NULL;
    int error = 0;
    int mtu;
    spl_t s;

    va_start(va, args);
    opt   = va_arg(va, struct mbuf *);
    ro    = va_arg(va, struct route *);
    flags = va_arg(va, int);
    im6o  = va_arg(va, struct ip6_moptions *);
    ifpp  = va_arg(va, struct ifnet **);
    inp   = va_arg(va, struct inpcb *);
    va_end(va);

    if (ro == NULL) {
        bzero(&iproute, sizeof(iproute));
        ro = (struct route *)&iproute;
    }

    /* Route lookup */
    dst = (struct sockaddr_in6 *)&ro->ro_dst;

    /* For multicast destinations, use im6o interface or pick first available */
    if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
        if (im6o && im6o->im6o_multicast_ifp) {
            ifp = im6o->im6o_multicast_ifp;
        } else {
            /* pick first non-loopback, up interface */
            extern struct ifnet *ifnet;
            struct ifnet *mifp;
            for (mifp = ifnet; mifp; mifp = mifp->if_next) {
                if ((mifp->if_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP) {
                    ifp = mifp;
                    break;
                }
            }
        }
        if (ifp == NULL) {
            ip6stat.ip6s_noroute++;
            error = EHOSTUNREACH;
            goto bad;
        }
        /* Build a gateway sockaddr pointing at the multicast destination
         * for link-local multicast (scope FF02::) - no routing needed */
        dst->sin6_family = AF_INET6;
        dst->sin6_len    = sizeof(*dst);
        dst->sin6_addr   = ip6->ip6_dst;
        /* Set hop limit for multicast */
        if (ip6->ip6_hlim == 0)
            ip6->ip6_hlim = im6o ? im6o->im6o_multicast_hlim
                                 : IP6_DEFAULT_MULTICAST_HLIM;
        goto multicast_output;
    }

    if (ro->ro_rt == NULL ||
        !IN6_ARE_ADDR_EQUAL(&dst->sin6_addr, &ip6->ip6_dst)) {
        /* flush stale route */
        if (ro->ro_rt) {
            RTFREE(ro->ro_rt);
            ro->ro_rt = NULL;
        }
        dst->sin6_family = AF_INET6;
        dst->sin6_len    = sizeof(*dst);
        dst->sin6_addr   = ip6->ip6_dst;
        rtalloc(ro);
    }
    if (ro->ro_rt == NULL) {
        ip6stat.ip6s_noroute++;
        error = EHOSTUNREACH;
        goto bad;
    }
    rt  = ro->ro_rt;
    ifp = rt->rt_ifp;

    /*
     * If the destination is a local IPv6 address configured on any interface,
     * redirect via the loopback interface so the packet loops back correctly
     * without needing NDP resolution on the NIC.
     */
#if INET6
    if (!(ifp->if_flags & IFF_LOOPBACK)) {
        struct in6_ifaddr *ia6;
        extern struct ifnet loif;
        for (ia6 = in6_ifaddr; ia6; ia6 = ia6->ia_next) {
            if (IN6_ARE_ADDR_EQUAL(&ia6->ia_addr.sin6_addr, &ip6->ip6_dst)) {
                ifp = &loif;
                break;
            }
        }
    }
#endif

multicast_output:
    if (ifpp)
        *ifpp = ifp;

    /* Set hop limit if not already set by transport layer */
    if (ip6->ip6_hlim == 0)
        ip6->ip6_hlim = ip6_defhlim;

    mtu = ifp->if_mtu;

    /* packet is within MTU: send directly */
    if (m->m_pkthdr.len <= mtu) {
        /* set packet length in ip6 header */
        ip6->ip6_plen = htons((u_int16_t)(m->m_pkthdr.len
                                          - sizeof(struct ip6_hdr)));

        /* add link-layer header via ifp */
        s = splimp();
        error = (*ifp->if_output)(ifp, m,
            (rt && (rt->rt_flags & RTF_GATEWAY)) ? rt->rt_gateway
                                         : (struct sockaddr *)dst,
            rt);
        splx(s);

        if (error)
            ip6stat.ip6s_odropped++;
        else
            ip6stat.ip6s_localout++;

        if (ro == (struct route *)&iproute && ro->ro_rt) {
            RTFREE(ro->ro_rt);
            ro->ro_rt = NULL;
        }
        return error;
    }

    /* ------------------------------------------------------------------
     * Fragmentation required.
     * Each fragment carries a Fragment extension header (8 bytes).
     * ------------------------------------------------------------------ */
    {
        int hlen    = sizeof(struct ip6_hdr);
        int frag_pl = (mtu - hlen - sizeof(struct ip6_frag)) & ~7;
        struct mbuf *m0;
        u_int32_t id;
        int off2;
        u_int16_t offlg;
        u_int8_t  nxt8;

        if (frag_pl <= 0) {
            ip6stat.ip6s_cantfrag++;
            error = EMSGSIZE;
            goto bad;
        }

        /* generate fragment identification */
        static u_int32_t ip6_id = 0;
        id = htonl(++ip6_id);
        nxt8 = ip6->ip6_nxt;

        /* rewrite next-header to IPPROTO_FRAGMENT */
        ip6->ip6_nxt = IPPROTO_FRAGMENT;

        off2 = 0;
        while (m->m_pkthdr.len > hlen) {
            int flen = MIN(frag_pl, m->m_pkthdr.len - hlen);
            int more = (m->m_pkthdr.len - hlen - flen) > 0;
            struct ip6_frag fh;

            /* allocate fragment mbuf */
            m0 = m_gethdr(M_WAIT, MT_DATA);
            if (m0 == NULL) { error = ENOBUFS; goto bad; }

            /* copy IPv6 header */
            M_COPY_PKTHDR(m0, m);
            m0->m_pkthdr.len = hlen + sizeof(fh) + flen;
            m0->m_len = hlen + sizeof(fh);
            bcopy(mtod(m, caddr_t), mtod(m0, caddr_t), hlen);

            /* fill fragment header */
            fh.ip6f_nxt      = nxt8;
            fh.ip6f_reserved = 0;
            offlg            = (u_int16_t)off2;
            if (more) offlg |= IP6F_MORE_FRAG;
            fh.ip6f_offlg    = htons(offlg);
            fh.ip6f_ident    = id;

            bcopy(&fh, mtod(m0, caddr_t) + hlen, sizeof(fh));

            /* copy payload */
            m_copydata(m, hlen + off2, flen, mtod(m0, caddr_t) + hlen + sizeof(fh));
            /* no separate data mbuf needed as M_EXT not used here */

            /* fix ip6_plen */
            ((struct ip6_hdr *)mtod(m0, caddr_t))->ip6_plen =
                htons(sizeof(fh) + flen);

            s = splimp();
            error = (*ifp->if_output)(ifp, m0,
                (rt->rt_flags & RTF_GATEWAY) ? rt->rt_gateway
                                             : (struct sockaddr *)dst,
                rt);
            splx(s);
            if (error) {
                ip6stat.ip6s_odropped++;
                goto bad;
            }
            ip6stat.ip6s_ofragments++;
            off2 += flen;
        }
        ip6stat.ip6s_fragmented++;
        m_freem(m);

        if (ro == (struct route *)&iproute && ro->ro_rt) {
            RTFREE(ro->ro_rt);
            ro->ro_rt = NULL;
        }
        return 0;
    }

bad:
    m_freem(m);
    if (ro == (struct route *)&iproute && ro->ro_rt) {
        RTFREE(ro->ro_rt);
        ro->ro_rt = NULL;
    }
    return error;
}

#endif /* INET6 */
