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
#include <net/if_loop_protos.h>
#include <protos/kern/kern_random_protos.h>
#include <kern/uipc_socket2_protos.h>

/*
 * ip6_mloopback - give a copy of a group datagram being sent on ifp to this
 * host's own members of the group there, through the loopback driver's
 * output routine with ifp as the receiving interface, as ip_mloopback()
 * does for IPv4. ip6_input() then treats it as having arrived on ifp.
 */
static void
ip6_mloopback(struct ifnet *ifp, struct mbuf *m, struct sockaddr_in6 *dst)
{
    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct mbuf *copym;

    /* the payload length is otherwise only filled in on the way out */
    ip6->ip6_plen = htons((u_int16_t)(m->m_pkthdr.len - sizeof(struct ip6_hdr)));
    copym = m_copy(m, 0, M_COPYALL);
    if(copym != NULL)
        (void)looutput(ifp, copym, (struct sockaddr *)dst, NULL);
}

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
/*
 * The interface a source address belongs to, or NULL (unspecified source, or
 * not one of ours).
 */
static struct ifnet *
ip6_srcifp(struct ip6_hdr *ip6)
{
    struct in6_addr src = ip6->ip6_src;     /* the header is packed */
    struct in6_ifaddr *ia6;

    if(IN6_IS_ADDR_UNSPECIFIED(&src))
        return NULL;
    for(ia6 = in6_ifaddr; ia6; ia6 = ia6->ia_next)
        if(IN6_ARE_ADDR_EQUAL(&ia6->ia_addr.sin6_addr, &src))
            return ia6->ia6_ifp;
    return NULL;
}

/*
 * ip6_outflags - the ip6_output() flags for one datagram from its
 * IPV6_USE_MIN_MTU (-1, 0, 1) and IPV6_DONTFRAG (RFC 3542 sections 11.1 and
 * 11.2): -1 sends multicast at the minimum MTU, unicast at the path MTU.
 */
int
ip6_outflags(int minmtu, int dontfrag, const struct in6_addr *dst)
{
    int flags = dontfrag ? IPV6_OUT_DONTFRAG : 0;

    if(minmtu == 1 || (minmtu == -1 && IN6_IS_ADDR_MULTICAST(dst)))
        flags |= IPV6_OUT_MINMTU;
    return flags;
}

/*
 * ip6_notify_pmtu - give a socket that asked with IPV6_RECVPATHMTU an
 * IPV6_PATHMTU message (RFC 3542 section 11.3): no data, the destination
 * and its path MTU as ancillary data, from that destination.
 */
void
ip6_notify_pmtu(const struct inpcb *inp, const struct sockaddr_in6 *dst,
                u_int32_t mtu)
{
    struct socket *so = inp->inp_socket;
    struct ip6_mtuinfo mi;
    struct mbuf *ctl;

    if(!(inp->in6p_flags & IN6P_RECVPATHMTU))
        return;
    bzero(&mi, sizeof(mi));
    mi.ip6m_addr = *dst;
    mi.ip6m_mtu = mtu;
    ctl = sbcreatecontrol((caddr_t)&mi, sizeof(mi), IPV6_PATHMTU, IPPROTO_IPV6);
    if(ctl == NULL)
        return;
    if(sbappendaddr(&so->so_rcv, (struct sockaddr *)&mi.ip6m_addr, NULL, ctl) == 0) {
        m_freem(ctl);
        return;
    }
    sorwakeup(so);
}

/*
 * ip6_pathmtu - the path MTU to a connected socket's peer, as ip6_output()
 * finds it: a link-local peer on the socket's zone, through its neighbour
 * entry; any other through the routing table. 0 if there is no way there.
 */
u_int32_t
ip6_pathmtu(const struct inpcb *inp)
{
    struct in6_addr dst = inp->inp_faddr6;
    struct rtentry *rt = NULL;
    struct ifnet *ifp = NULL;
    u_int32_t mtu;

    if(IN6_IS_ADDR_LINKLOCAL(&dst) || IN6_IS_ADDR_MULTICAST(&dst)) {
        ifp = inp->inp_boundif;
        if(ifp != NULL && IN6_IS_ADDR_LINKLOCAL(&dst))
            rt = nd6_lookup(&dst, 0, ifp);
    } else {
        struct route_in6 ro;

        bzero(&ro, sizeof(ro));
        ro.ro_dst.sin6_family = AF_INET6;
        ro.ro_dst.sin6_len = sizeof(ro.ro_dst);
        ro.ro_dst.sin6_addr = dst;
        rtalloc((struct route *)&ro);
        rt = ro.ro_rt;
        if(rt != NULL)
            ifp = rt->rt_ifp;
    }
    if(ifp == NULL) {
        if(rt != NULL)
            rtfree(rt);
        return 0;
    }
    mtu = ifp->if_mtu;
    if(rt != NULL) {
        if(rt->rt_rmx.rmx_mtu != 0 && rt->rt_rmx.rmx_mtu < mtu)
            mtu = rt->rt_rmx.rmx_mtu;
        rtfree(rt);
    }
    return mtu;
}

int
ip6_output(void *args, ...)
{
    struct mbuf *m = args;
    struct ip6_moptions *im6o;
    struct ifnet **ifpp;
    va_list va;

    struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
    struct ifnet   *ifp = NULL;
    /*
     * The interface a looped-back packet is received on: the one that owns
     * the link-scope address it is sent to (lo0 for everything else), so
     * the receiving side sees the zone the address belongs to, as KAME's
     * origifp does.
     */
    struct ifnet   *rcvifp = NULL;
    struct route_in6 iproute;
    struct route    *ro;
    struct sockaddr_in6 *dst;
    struct rtentry *rt = NULL;
    int error = 0;
    int mtu, flags;
    struct inpcb *inp;
    spl_t s;

    va_start(va, args);
    (void)va_arg(va, struct mbuf *);
    ro    = va_arg(va, struct route *);
    flags = va_arg(va, int);
    im6o  = va_arg(va, struct ip6_moptions *);
    ifpp  = va_arg(va, struct ifnet **);
    inp   = va_arg(va, struct inpcb *);
    va_end(va);

    if(ro == NULL) {
        bzero(&iproute, sizeof(iproute));
        ro = (struct route *)&iproute;
    }

    /* Route lookup */
    dst = (struct sockaddr_in6 *)&ro->ro_dst;

    /* For multicast destinations, use im6o interface or pick first available */
    if(IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
        if(im6o && im6o->im6o_multicast_ifp) {
            ifp = im6o->im6o_multicast_ifp;
        } else if((ifp = ip6_srcifp(ip6)) != NULL) {
            /* a sender that has picked its source address has picked the
             * link: a DHCPv6 client bound to an interface's link-local
             * address means that interface, not the first one that is up */
        } else {
            /* pick first non-loopback, up interface */
            extern struct ifnet *ifnet;
            struct ifnet *mifp;
            for(mifp = ifnet; mifp; mifp = mifp->if_next) {
                if((mifp->if_flags & (IFF_UP | IFF_LOOPBACK)) == IFF_UP) {
                    ifp = mifp;
                    break;
                }
            }
        }
        if(ifp == NULL) {
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
        if(ip6->ip6_hlim == 0)
            ip6->ip6_hlim = im6o ? im6o->im6o_multicast_hlim
                            : IP6_DEFAULT_MULTICAST_HLIM;
        /*
         * If this host is a member of the group on the outgoing interface
         * and the sender did not forbid it with IPV6_MULTICAST_LOOP (on by
         * default, RFC 3493 section 5.2), loop a copy back, as ip_output()
         * does for IPv4. The stack's own neighbour discovery and MLD
         * messages pass zeroed options, so they never come back to it.
         */
        {
            struct in6_addr grp = ip6->ip6_dst;     /* the header is packed */
            struct in6_multi *in6m;

            IN6_LOOKUP_MULTI(grp, ifp, in6m);
            if(in6m != NULL && (im6o == NULL || im6o->im6o_multicast_loop))
                ip6_mloopback(ifp, m, dst);
        }
        goto multicast_output;
    }

    /*
     * A link-local destination is a neighbour on one particular link, but
     * every interface has the same fe80::/64 prefix and the routing table
     * holds it once - so a lookup sends all link-local traffic to whichever
     * interface registered first (neighbour advertisements for a router on
     * one interface going out of another). The link is the one the caller
     * names, or the one the source address lives on. Our own addresses
     * still take the normal path below, which loops them back.
     */
    if(IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_dst)) {
        struct ifnet *lifp = (im6o && im6o->im6o_multicast_ifp) ?
                             im6o->im6o_multicast_ifp : ip6_srcifp(ip6);
        struct in6_ifaddr *ia6;

        for(ia6 = in6_ifaddr; lifp && ia6; ia6 = ia6->ia_next)
            if(IN6_ARE_ADDR_EQUAL(&ia6->ia_addr.sin6_addr, &ip6->ip6_dst))
                lifp = NULL;
        if(lifp != NULL && !(lifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))) {
            struct in6_addr nbr = ip6->ip6_dst;     /* the header is packed */
            struct rtentry *nrt;

            /* the neighbour's cache entry, on that link (nd6_lookup() moves
             * it there if it was cloned for another interface) */
            if((nrt = nd6_lookup(&nbr, 1, lifp)) != NULL) {
                if(ro->ro_rt)
                    RTFREE(ro->ro_rt);
                ro->ro_rt = nrt;
                dst->sin6_family = AF_INET6;
                dst->sin6_len    = sizeof(*dst);
                dst->sin6_addr   = nbr;
                rt  = nrt;
                ifp = lifp;
                goto multicast_output;
            }
        }
    }

    if(ro->ro_rt == NULL ||
            !IN6_ARE_ADDR_EQUAL(&dst->sin6_addr, &ip6->ip6_dst)) {
        /* flush stale route */
        if(ro->ro_rt) {
            RTFREE(ro->ro_rt);
            ro->ro_rt = NULL;
        }
        dst->sin6_family = AF_INET6;
        dst->sin6_len    = sizeof(*dst);
        dst->sin6_addr   = ip6->ip6_dst;
        rtalloc(ro);
    }
    if(ro->ro_rt == NULL) {
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
    if(!(ifp->if_flags & IFF_LOOPBACK)) {
        struct in6_ifaddr *ia6;
        extern struct ifnet loif;
        for(ia6 = in6_ifaddr; ia6; ia6 = ia6->ia_next) {
            if(IN6_ARE_ADDR_EQUAL(&ia6->ia_addr.sin6_addr, &ip6->ip6_dst)) {
                ifp = &loif;
                if(IN6_IS_ADDR_LINKLOCAL(&ia6->ia_addr.sin6_addr))
                    rcvifp = ia6->ia6_ifp;
                break;
            }
        }
    }
#endif

multicast_output:
    if(ifpp)
        *ifpp = ifp;

    /* Set hop limit if not already set by transport layer */
    if(ip6->ip6_hlim == 0)
        ip6->ip6_hlim = ip6_defhlim;

    mtu = ifp->if_mtu;
    /*
     * Honour a path MTU discovered via ICMPv6 Packet Too Big (RFC 8201),
     * recorded on the route by icmp6_update_pmtu().  Never exceed the link MTU.
     */
    if(ro->ro_rt != NULL && ro->ro_rt->rt_rmx.rmx_mtu != 0 &&
            ro->ro_rt->rt_rmx.rmx_mtu < (u_long)mtu)
        mtu = (int)ro->ro_rt->rt_rmx.rmx_mtu;
    if((flags & IPV6_OUT_MINMTU) && mtu > IPV6_MMTU)
        mtu = IPV6_MMTU;                    /* IPV6_USE_MIN_MTU */

    /*
     * IPV6_DONTFRAG (RFC 3542 section 11.2): the datagram leaves whole even
     * above the path MTU (an application probing for a larger one, RFC
     * 8899), but not above the link's: EMSGSIZE, and an IPV6_PATHMTU
     * message for a socket that asked. As KAME and FreeBSD.
     */
    if((flags & IPV6_OUT_DONTFRAG) && m->m_pkthdr.len > mtu) {
        if(m->m_pkthdr.len > (int)ifp->if_mtu) {
            ip6stat.ip6s_cantfrag++;
            if(inp != NULL) {
                struct sockaddr_in6 d6;

                bzero(&d6, sizeof(d6));
                d6.sin6_family = AF_INET6;
                d6.sin6_len = sizeof(d6);
                d6.sin6_addr = ip6->ip6_dst;
                if(IN6_IS_ADDR_LINKLOCAL(&d6.sin6_addr) ||
                        IN6_IS_ADDR_MC_LINKLOCAL(&d6.sin6_addr))
                    d6.sin6_scope_id = ifp->if_index;
                ip6_notify_pmtu(inp, &d6, (u_int32_t)mtu);
            }
            error = EMSGSIZE;
            goto bad;
        }
        mtu = m->m_pkthdr.len;
    }

    /* packet is within MTU: send directly */
    if(m->m_pkthdr.len <= mtu) {
        /* set packet length in ip6 header */
        ip6->ip6_plen = htons((u_int16_t)(m->m_pkthdr.len
                                          - sizeof(struct ip6_hdr)));

        /* add link-layer header via ifp */
        s = splimp();
        error = (*ifp->if_output)(rcvifp ? rcvifp : ifp, m,
                                  (rt && (rt->rt_flags & RTF_GATEWAY)) ? rt->rt_gateway
                                  : (struct sockaddr *)dst,
                                  rt);
        splx(s);

        if(error)
            ip6stat.ip6s_odropped++;
        else
            ip6stat.ip6s_localout++;

        if(ro == (struct route *)&iproute && ro->ro_rt) {
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
        int off2, total;
        u_int8_t  nxt8;

        if(frag_pl <= 0) {
            ip6stat.ip6s_cantfrag++;
            error = EMSGSIZE;
            goto bad;
        }

        /* a random fragment identification, not a counter: RFC 7739
         * section 5 (arc4random_buf(), through kern_random.c) */
        id = kern_random32();
        nxt8 = ip6->ip6_nxt;

        /* rewrite next-header to IPPROTO_FRAGMENT */
        ip6->ip6_nxt = IPPROTO_FRAGMENT;

        /*
         * The fragmentable part is everything after the IPv6 header; each
         * fragment carries the next frag_pl bytes of it (a multiple of 8)
         * until what is left fits (RFC 8200 section 4.5).
         */
        total = m->m_pkthdr.len - hlen;
        off2 = 0;
        while(off2 < total) {
            int flen = MIN(frag_pl, total - off2);
            int more = off2 + flen < total;
            struct ip6_frag fh;

            /*
             * Build the fragment as a chain: a small head mbuf carries the
             * (copied) IPv6 header plus the 8-byte fragment header, and the
             * payload is attached with m_copy(), which shares the original
             * clusters.  A jumbo fragment therefore never has to fit within a
             * single cluster (mbconf.mclbytes), which an inline copy assumed.
             */
            m0 = m_gethdr(M_WAIT, MT_DATA);
            if(m0 == NULL) {
                error = ENOBUFS;
                goto bad;
            }

            /* head mbuf: IPv6 header + fragment header (48 bytes, fits MHLEN) */
            M_COPY_PKTHDR(m0, m);
            m0->m_len = hlen + sizeof(fh);
            bcopy(mtod(m, caddr_t), mtod(m0, caddr_t), hlen);

            /* fill fragment header */
            /* the offset in 8-octet units sits above the M flag; off2 is a
             * multiple of 8, so in network order it is htons(off2), and
             * IP6F_MORE_FRAG is defined in network order already */
            fh.ip6f_nxt      = nxt8;
            fh.ip6f_reserved = 0;
            fh.ip6f_offlg    = htons((u_int16_t)off2);
            if(more)
                fh.ip6f_offlg |= IP6F_MORE_FRAG;
            fh.ip6f_ident    = id;

            bcopy(&fh, mtod(m0, caddr_t) + hlen, sizeof(fh));

            /* attach the payload as a shared chain rather than an inline copy */
            m0->m_next = m_copy(m, hlen + off2, flen);
            if(m0->m_next == NULL) {
                m_freem(m0);
                error = ENOBUFS;
                goto bad;
            }
            m0->m_pkthdr.len = hlen + sizeof(fh) + flen;

            /* fix ip6_plen */
            ((struct ip6_hdr *)mtod(m0, caddr_t))->ip6_plen =
                htons(sizeof(fh) + flen);

            s = splimp();
            /* a group datagram has no route (rt is NULL) */
            error = (*ifp->if_output)(rcvifp ? rcvifp : ifp, m0,
                                      (rt && (rt->rt_flags & RTF_GATEWAY)) ? rt->rt_gateway
                                      : (struct sockaddr *)dst,
                                      rt);
            splx(s);
            if(error) {
                ip6stat.ip6s_odropped++;
                goto bad;
            }
            ip6stat.ip6s_ofragments++;
            off2 += flen;
        }
        ip6stat.ip6s_fragmented++;
        m_freem(m);

        if(ro == (struct route *)&iproute && ro->ro_rt) {
            RTFREE(ro->ro_rt);
            ro->ro_rt = NULL;
        }
        return 0;
    }

bad:
    m_freem(m);
    if(ro == (struct route *)&iproute && ro->ro_rt) {
        RTFREE(ro->ro_rt);
        ro->ro_rt = NULL;
    }
    return error;
}

#endif /* INET6 */
