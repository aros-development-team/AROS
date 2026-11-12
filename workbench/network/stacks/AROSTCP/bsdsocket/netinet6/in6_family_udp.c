/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the one UDP (netinet/udp_usrreq.c) asks of IPv6: the IPv6 family's
 * implementations of the "UDP and raw sockets" operations of struct
 * inetfamily (netinet/inetfamily.h), and the two entry points by which
 * IPv6 hands UDP a datagram and what ICMPv6 said about one. Everything
 * here is what IPv6 alone does for a datagram socket: its socket address
 * and zones, IPV6_PKTINFO / IPV6_HOPLIMIT / IPV6_TCLASS received and
 * sent, IPV6_DONTFRAG and IPV6_USE_MIN_MTU on one datagram, the source
 * address of a datagram, the path MTU told to a socket, ICMPv6 port
 * unreachable.
 *
 * The send path is the IPv6 half of what was udp6_output() in
 * netinet6/udp6_usrreq.c (derived from netinet/udp_usrreq.c and KAME's
 * udp6_usrreq.c):
 *
 * Copyright (C) 1982, 1986, 1988, 1990, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 * (the 4.4BSD licence, as in netinet/udp_usrreq.c)
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
#include <sys/synch.h>
#include <string.h>             /* memcpy */
#include <stddef.h>             /* offsetof */
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>
#include <netinet6/in6_var.h>
#include <netinet/inetfamily.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

extern const struct inetfamily in6_family;
extern int in6_cksum(struct mbuf *, u_int8_t, u_int32_t, u_int32_t);
extern int ip6_output(void *, ...);     /* (m, opt, ro, flags, im6o, ifpp, inp) */
extern void icmp6_error(struct mbuf *, int, int, int);

int     in6_rcvfrom(struct mbuf *, const void *, u_int, const void *, struct sockaddr *);
struct mbuf **in6_savecontrol(struct inpcb *, const struct ifnet *, const void *,
                              int, int, struct mbuf **);
void    in6_unreach(struct mbuf *, int);
int     in6_dgoutput(struct mbuf *, struct inpcb *, const void *, struct ifnet *,
                     struct mbuf *, int, int, int);
void    in6_pmtunotify(struct inpcb *, struct sockaddr *, u_int32_t);

/* the addresses that name a link only together with a zone (RFC 4007) */
#define IN6_IS_ADDR_ZONED(a) \
    (IN6_IS_ADDR_LINKLOCAL(a) || IN6_IS_ADDR_MC_LINKLOCAL(a) || \
     IN6_IS_ADDR_MC_NODELOCAL(a))

/*
 * inf_rcvfrom: the sender as a sockaddr_in6, a link-scope address with
 * the zone of the interface the datagram came in by; a datagram for a
 * group goes to every socket that listens.
 */
int
in6_rcvfrom(struct mbuf *m, const void *src, u_int sport, const void *dst,
            struct sockaddr *sa)
{
    in6_mksockaddr(sa, src, sport, m->m_pkthdr.rcvif);
    return (IN6_IS_ADDR_MULTICAST((const struct in6_addr *)dst));
}

/*
 * inf_savecontrol: IPV6_PKTINFO, IPV6_HOPLIMIT, IPV6_TCLASS (RFC 3542
 * sections 6.1, 6.3 and 6.5), each as one control message.
 */
struct mbuf **
in6_savecontrol(struct inpcb *inp, const struct ifnet *rcvif, const void *dst,
                int tos, int hlim, struct mbuf **mp)
{
    if(inp->in6p_flags & IN6P_PKTINFO) {
        struct in6_pktinfo pi6;

        bzero(&pi6, sizeof(pi6));
        pi6.ipi6_addr    = *(const struct in6_addr *)dst;
        pi6.ipi6_ifindex = (rcvif != NULL) ? rcvif->if_index : 0;
        *mp = sbcreatecontrol((caddr_t)&pi6, sizeof(pi6), IPV6_PKTINFO, IPPROTO_IPV6);
        if(*mp != NULL)
            mp = &(*mp)->m_next;
    }
    if(inp->in6p_flags & IN6P_HOPLIMIT) {
        *mp = sbcreatecontrol((caddr_t)&hlim, sizeof(int), IPV6_HOPLIMIT, IPPROTO_IPV6);
        if(*mp != NULL)
            mp = &(*mp)->m_next;
    }
    if(inp->in6p_flags & IN6P_TCLASS) {
        *mp = sbcreatecontrol((caddr_t)&tos, sizeof(int), IPV6_TCLASS, IPPROTO_IPV6);
        if(*mp != NULL)
            mp = &(*mp)->m_next;
    }
    return (mp);
}

/*
 * inf_unreach: Destination Unreachable, port unreachable (RFC 4443
 * section 3.1); icmp6_error() takes the packet, and leaves out what must
 * not be answered.
 */
void
in6_unreach(struct mbuf *m, int off)
{
    icmp6_error(m, ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_NOPORT, 0);
}

/*
 * inf_pmtunotify: a socket that asked with IPV6_RECVPATHMTU is told the
 * new path MTU (RFC 3542 section 11.3), as KAME's in6_pcbnotify() does -
 * unless it is in another zone than the destination the error names.
 */
void
in6_pmtunotify(struct inpcb *inp, struct sockaddr *dst, u_int32_t mtu)
{
    const struct sockaddr_in6 *dst6 = (const struct sockaddr_in6 *)dst;

    if(!(inp->in6p_flags & IN6P_RECVPATHMTU))
        return;
    if(inp->inp_boundif != NULL && dst6->sin6_scope_id != 0 &&
            (u_int32_t)inp->inp_boundif->if_index != dst6->sin6_scope_id)
        return;
    ip6_notify_pmtu(inp, dst6, mtu);
}

/*
 * The IPv6 header at the front of an mbuf's data, which is an even
 * address. struct ip6_hdr is packed: through a plain pointer to it every
 * field wider than a byte, and each address, is moved a byte at a time (or
 * by a call) on machines that cannot access a word at an odd address (the
 * 68000).
 */
typedef struct ip6_hdr ip6_ahdr_t __attribute__((__aligned__(2)));

/*
 * inf_dgoutput.
 *
 * Ancillary data (RFC 3542): IPV6_PKTINFO names the interface the
 * datagram leaves by and its source address, IPV6_DONTFRAG and
 * IPV6_USE_MIN_MTU replace the socket's for this datagram (section 11),
 * IPV6_TCLASS its traffic class (section 6.5). Anything else is ignored.
 *
 * The interface: IPV6_PKTINFO's, else the zone of the destination a
 * datagram is sent with (zone: the sockaddr's scope id, for a link-scope
 * address). Link-local and multicast destinations do not say which link
 * they mean any other way. A socket bound to an interface
 * (SO_BINDTODEVICE, or a link-scope bind() or connect()) is in that zone:
 * a link-scope destination with no zone is on its link, and one with
 * another zone is an error, as connect() has it (and Linux's
 * udpv6_sendmsg()).
 */
int
in6_dgoutput(struct mbuf *m, struct inpcb *inp, const void *faddr,
             struct ifnet *zone, struct mbuf *control, int proto, int cksumoff,
             int flags)
{
    int len = m->m_pkthdr.len;          /* transport header and data */
    static const struct in6_addr unspecified;   /* :: */
    struct in6_addr laddr6, faddr6, osrc;
    ip6_ahdr_t *ip6;
    struct ifnet *oifp = NULL;
    unsigned int oindex = 0;
    struct ip6_moptions oim6o;
    /* RFC 3542 section 11: the socket's, unless this datagram says */
    int minmtu = IN6P_MINMTU_OPT(inp);
    int dontfrag = (inp->in6p_flags & IN6P_DONTFRAG) != 0;
    int tclass = inp->in6p_tclass;      /* IPV6_TCLASS, RFC 3542 section 6.5 */
    int error;

    /*
     * (The addresses are moved as words, in place: for each, a call to
     * copy 16 bytes costs more than the copy. They are in PCBs, mbufs
     * and variables, all even-aligned.)
     */
    inpcb_addrcopy(&osrc, &unspecified, sizeof(osrc));
    if(control) {
        struct cmsghdr *cm = mtod(control, struct cmsghdr *);
        int left = control->m_len;

        while(left >= (int)sizeof(*cm) && cm->cmsg_len >= sizeof(*cm) &&
                (int)cm->cmsg_len <= left) {
            if(cm->cmsg_level == IPPROTO_IPV6 && cm->cmsg_type == IPV6_PKTINFO &&
                    cm->cmsg_len >= CMSG_LEN(sizeof(struct in6_pktinfo))) {
                struct in6_pktinfo pi6;

                bcopy(CMSG_DATA(cm), (caddr_t)&pi6, sizeof(pi6));
                oindex = pi6.ipi6_ifindex;
                osrc   = pi6.ipi6_addr;
            } else if(cm->cmsg_level == IPPROTO_IPV6 &&
                      (cm->cmsg_type == IPV6_DONTFRAG ||
                       cm->cmsg_type == IPV6_USE_MIN_MTU) &&
                      cm->cmsg_len == CMSG_LEN(sizeof(int))) {
                int v;

                memcpy(&v, CMSG_DATA(cm), sizeof(v));
                if(cm->cmsg_type == IPV6_DONTFRAG)
                    dontfrag = v != 0;
                else if(v >= -1 && v <= 1)
                    minmtu = v;
            } else if(cm->cmsg_level == IPPROTO_IPV6 && cm->cmsg_type == IPV6_TCLASS &&
                      cm->cmsg_len == CMSG_LEN(sizeof(int))) {
                int v;

                memcpy(&v, CMSG_DATA(cm), sizeof(v));
                if(v >= -1 && v <= 255)
                    tclass = v < 0 ? 0 : v;
            }
            if(left < (int)_ALIGN(cm->cmsg_len))
                break;
            left -= _ALIGN(cm->cmsg_len);
            cm = (struct cmsghdr *)((caddr_t)cm + _ALIGN(cm->cmsg_len));
        }
        m_freem(control);
    }

    if(faddr == NULL)
        inpcb_addrcopy(&faddr6, &inp->inp_faddr6, sizeof(faddr6));
    else {
        inpcb_addrcopy(&faddr6, faddr, sizeof(faddr6));
        if(IN6_IS_ADDR_UNSPECIFIED(&faddr6)) {
            error = EADDRNOTAVAIL;
            goto release;
        }
        if(oindex == 0) {
            oifp = zone;
            if(inp->inp_boundif && IN6_IS_ADDR_ZONED(&faddr6)) {
                if(oifp == NULL)
                    oifp = inp->inp_boundif;
                else if(oifp != inp->inp_boundif) {
                    error = EINVAL;
                    goto release;
                }
            }
        }
    }
    if(oindex != 0)
        for(oifp = ifnet_list; oifp; oifp = oifp->if_next)
            if((unsigned int)oifp->if_index == oindex)
                break;

    /*
     * A group datagram with no interface named on it leaves by the one
     * IPV6_MULTICAST_IF set (RFC 3493 section 5.2), so the source comes
     * from there too.
     */
    if(oifp == NULL && IN6_IS_ADDR_MULTICAST(&faddr6) && inp->in6p_moptions)
        oifp = inp->in6p_moptions->im6o_multicast_ifp;

    /*
     * ---- Resolve source address ----
     * An IPV6_PKTINFO source must be an address of ours, usable (RFC 3542
     * section 6.1, as KAME's in6_selectsrc() checks); on a datagram sent
     * with a destination it wins over the address the socket is bound to.
     * A connected socket keeps the source of its connection. Else the
     * interface asked for supplies the address, else the routing table
     * (in6_selectsrc()) - and a datagram is never sent from ::, the
     * peer's ip6_input() discards it.
     */
    if(!IN6_IS_ADDR_UNSPECIFIED(&osrc) && in6_ourifa(&osrc, NULL) == NULL) {
        error = EADDRNOTAVAIL;
        goto release;
    }
    if(!IN6_IS_ADDR_UNSPECIFIED(&osrc) &&
            (faddr != NULL || IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)))
        inpcb_addrcopy(&laddr6, &osrc, sizeof(laddr6)); /* IPV6_PKTINFO named it */
    else if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6))
        inpcb_addrcopy(&laddr6, &inp->inp_laddr6, sizeof(laddr6));
    else if((error = in6_selectsrc(inp, &faddr6, oifp, &laddr6)) != 0)
        goto release;

    /* ---- the IPv6 header in front ---- */
    if(flags & INF_DG_ROOM) {
        m->m_data -= sizeof(struct ip6_hdr);
        m->m_len += sizeof(struct ip6_hdr);
        m->m_pkthdr.len += sizeof(struct ip6_hdr);
    } else {
        int need = sizeof(struct ip6_hdr) +
                   (cksumoff < 0 ? 0 : cksumoff + (int)sizeof(u_int16_t));

        M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
        if(m == NULL)
            return ENOBUFS;
        /*
         * M_PREPEND may have allocated a separate mbuf for the IPv6
         * header, leaving the transport header in the following mbuf.
         * Pull up as far as the checksum field, so the store below lands
         * on it.
         */
        if(m->m_len < need && (m = m_pullup(m, need)) == NULL)
            return ENOBUFS;
    }
    ip6 = mtod(m, ip6_ahdr_t *);
    /* version 6 and the traffic class; no flow label */
    ip6->ip6_flow = htonl(0x60000000 | ((u_int32_t)(tclass & 0xff) << 20));
    ip6->ip6_plen = htons((u_short)len);
    ip6->ip6_nxt  = proto;
    if(IN6_IS_ADDR_MULTICAST(&faddr6))
        ip6->ip6_hlim = inp->in6p_moptions
            ? inp->in6p_moptions->im6o_multicast_hlim
            : IP6_DEFAULT_MULTICAST_HLIM;
    else
        ip6->ip6_hlim = (inp->in6p_hops >= 0) ? (u_int8_t)inp->in6p_hops
                        : (u_int8_t)ip6_defhlim;
    inpcb_addrcopy(&ip6->ip6_src, &laddr6, sizeof(struct in6_addr));
    inpcb_addrcopy(&ip6->ip6_dst, &faddr6, sizeof(struct in6_addr));

    /* ---- the checksum, over the IPv6 pseudo-header ---- */
    if(cksumoff >= 0) {
        u_int16_t *sump = (u_int16_t *)(mtod(m, caddr_t) + sizeof(struct ip6_hdr) + cksumoff);
        u_int16_t sum;

        *sump = 0;
        sum = in6_cksum(m, proto, sizeof(struct ip6_hdr), (u_int32_t)len);
        if(sum == 0 && (flags & INF_DG_NOTZERO))
            sum = 0xffff;   /* RFC 2460: UDP6 checksum must not be zero */
        *sump = sum;
    }

    flags = ip6_outflags(minmtu, dontfrag, &faddr6);
    if(oifp != NULL) {
        /* hand the interface on the way multicast options do */
        if(inp->in6p_moptions)
            oim6o = *inp->in6p_moptions;
        else {
            bzero(&oim6o, sizeof(oim6o));
            oim6o.im6o_multicast_hlim = IP6_DEFAULT_MULTICAST_HLIM;
            oim6o.im6o_multicast_loop = 1;
        }
        oim6o.im6o_multicast_ifp = oifp;
        return ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, flags,
                          &oim6o, (struct ifnet **)NULL, inp);
    }
    return ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, flags,
                      inp->in6p_moptions, (struct ifnet **)NULL, inp);

release:
    m_freem(m);
    return error;
}

#endif /* INET6 */
