/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the one UDP (netinet/udp_usrreq.c) asks of IPv4: the IPv4 family's
 * implementations of the "UDP and raw sockets" operations of struct
 * inetfamily (netinet/inetfamily.h), and the two entry points by which
 * IPv4 hands UDP a datagram and what ICMP said about one. Everything here
 * is what IPv4 alone does for a datagram socket: its socket address,
 * broadcast, IP_RECVDSTADDR / IP_RECVTOS / IP_RECVTTL / IP_RECVIF, IP_TOS on one
 * datagram, IP options, the limited broadcast of a socket bound to an
 * interface, ICMP port unreachable.
 *
 * The send path is the IPv4 half of what was udp_output() in
 * netinet/udp_usrreq.c:
 *
 * Copyright (C) 1982, 1986, 1988, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (the 4.4BSD licence, as in netinet/udp_usrreq.c)
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
#include <sys/synch.h>
#include <string.h>             /* memcpy */
#include <stddef.h>             /* offsetof */
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>
#include <net/if_dl.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>
#include <netinet/inetfamily.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/in_protos.h>
#include <netinet/in_cksum_protos.h>
#include <netinet/ip_input_protos.h>
#include <netinet/ip_output_protos.h>
#include <netinet/ip_icmp_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

extern const struct inetfamily in_family;
extern struct in_addr zeroin_addr;

int     in_rcvfrom(struct mbuf *, const void *, u_int, const void *, struct sockaddr *);
struct mbuf **in_savecontrol(struct inpcb *, const struct ifnet *, const void *,
                             int, int, struct mbuf **);
void    in_unreach(struct mbuf *, int);
int     in_dgoutput(struct mbuf *, struct inpcb *, const void *, struct ifnet *,
                    struct mbuf *, int, int, int);

/*
 * inf_rcvfrom: the sender as a sockaddr_in; a datagram for a group or for
 * a broadcast address of ours goes to every socket that listens.
 */
int
in_rcvfrom(struct mbuf *m, const void *src, u_int sport, const void *dst,
           struct sockaddr *sa)
{
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
    struct in_addr d = *(const struct in_addr *)dst;

    bzero((caddr_t)sin, sizeof(*sin));
    sin->sin_len = sizeof(*sin);
    sin->sin_family = AF_INET;
    sin->sin_port = sport;
    sin->sin_addr = *(const struct in_addr *)src;
    return (IN_MULTICAST(ntohl(d.s_addr)) || in_broadcast(d));
}

/*
 * inf_savecontrol: IP_RECVDSTADDR, IP_RECVTOS, IP_RECVTTL (the last two
 * one byte each, as FreeBSD), IP_RECVIF.
 */
struct mbuf **
in_savecontrol(struct inpcb *inp, const struct ifnet *rcvif, const void *dst,
               int tos, int hlim, struct mbuf **mp)
{
    if(inp->inp_flags & INP_RECVDSTADDR) {
        struct in_addr d = *(const struct in_addr *)dst;

        *mp = sbcreatecontrol((caddr_t)&d, sizeof(struct in_addr),
                              IP_RECVDSTADDR, IPPROTO_IP);
        if(*mp)
            mp = &(*mp)->m_next;
    }
    if(inp->inp_flags & INP_RECVTOS) {
        u_char b = tos;

        *mp = sbcreatecontrol((caddr_t)&b, sizeof(b), IP_RECVTOS, IPPROTO_IP);
        if(*mp)
            mp = &(*mp)->m_next;
    }
    if(inp->inp_flags & INP_RECVTTL) {
        u_char b = hlim;

        *mp = sbcreatecontrol((caddr_t)&b, sizeof(b), IP_RECVTTL, IPPROTO_IP);
        if(*mp)
            mp = &(*mp)->m_next;
    }
    if((inp->inp_flags & INP_RECVIF) && rcvif != NULL) {
        /*
         * The interface it came in by, as FreeBSD's ip_savecontrol() gives
         * it: the interface's link-level address (if_attach() put it on
         * if_addrlist) - index, name and hardware address - or a
         * sockaddr_dl with the index alone.
         */
        const struct ifaddr *ifa;
        struct sockaddr_dl sdl;
        const struct sockaddr_dl *sdp = NULL;

        for(ifa = rcvif->if_addrlist; ifa != NULL; ifa = ifa->ifa_next)
            if(ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_LINK) {
                sdp = (const struct sockaddr_dl *)ifa->ifa_addr;
                break;
            }
        if(sdp == NULL || CMSG_SPACE((u_int)sdp->sdl_len) > MLEN) {
            memset(&sdl, 0, sizeof(sdl));
            sdl.sdl_len = sizeof(sdl);
            sdl.sdl_family = AF_LINK;
            sdl.sdl_index = rcvif->if_index;
            sdp = &sdl;
        }
        *mp = sbcreatecontrol((caddr_t)sdp, sdp->sdl_len, IP_RECVIF, IPPROTO_IP);
        if(*mp)
            mp = &(*mp)->m_next;
    }
    return (mp);
}

/*
 * inf_unreach: ICMP port unreachable. The IP header is as ip_input() left
 * it - IPv4's checksum of a received packet does not write on it
 * (in_tpcksum()) - which is what icmp_error() wants.
 */
void
in_unreach(struct mbuf *m, int off)
{
    icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PORT, zeroin_addr);
}

/*
 * inf_dgoutput.
 *
 * Ancillary data: IP_TOS for this datagram (a byte, as FreeBSD, or an
 * int, as Linux), so an application can set the ECN field per packet
 * (RFC 9000 section 13.4). Anything else is ignored.
 *
 * A datagram sent with an address: "any" stands for this host's primary
 * address, and the limited broadcast address for the broadcast address of
 * the primary interface if it has one. The source of a socket that is
 * bound to no address is chosen by the route (in_selectsrc()). A socket
 * bound to an interface (SO_BINDTODEVICE) sends from that interface's own
 * address - none yet for a DHCP client - and its 255.255.255.255 stays as
 * it is.
 */
int
in_dgoutput(struct mbuf *m, struct inpcb *inp, const void *faddr,
            struct ifnet *zone, struct mbuf *control, int proto, int cksumoff,
            int flags)
{
    int len = m->m_pkthdr.len;          /* transport header and data */
    struct in_addr src, dst;
    struct ipovly *ipov;
    struct ip *ip;
    u_char tos = inp->inp_ip.ip_tos;
    int ipflags, error;

    if(control) {
        struct cmsghdr *cm = mtod(control, struct cmsghdr *);
        int left = control->m_len;

        while(left >= (int)sizeof(*cm) && cm->cmsg_len >= sizeof(*cm) &&
                (int)cm->cmsg_len <= left) {
            if(cm->cmsg_level == IPPROTO_IP && cm->cmsg_type == IP_TOS) {
                if(cm->cmsg_len == CMSG_LEN(sizeof(u_char)))
                    tos = *(u_char *)CMSG_DATA(cm);
                else if(cm->cmsg_len == CMSG_LEN(sizeof(int))) {
                    int v;

                    memcpy(&v, CMSG_DATA(cm), sizeof(v));
                    tos = v & 0xff;
                }
            }
            if(left < (int)_ALIGN(cm->cmsg_len))
                break;
            left -= _ALIGN(cm->cmsg_len);
            cm = (struct cmsghdr *)((caddr_t)cm + _ALIGN(cm->cmsg_len));
        }
        m_freem(control);
    }

    if(faddr == NULL) {
        src = inp->inp_laddr;
        dst = inp->inp_faddr;
    } else {
        int tobroadcast;

        dst = *(const struct in_addr *)faddr;
        tobroadcast = (dst.s_addr == (u_long)INADDR_BROADCAST);
        if(in_ifaddr) {
            if(dst.s_addr == INADDR_ANY)
                dst = IA_SIN(in_ifaddr)->sin_addr;
            else if(tobroadcast && (in_ifaddr->ia_ifp->if_flags & IFF_BROADCAST))
                dst = ((struct sockaddr_in *)&in_ifaddr->ia_broadaddr)->sin_addr;
        }
        src = inp->inp_laddr;
        if(src.s_addr == INADDR_ANY) {
            /* by the route, which the PCB keeps for the next datagram */
            if((error = in_selectsrc(inp, &dst, (struct ifnet *)NULL, &src)) != 0) {
                m_freem(m);
                return (error);
            }
            if(inp->inp_boundif)
                (void)in_selectsrc(inp, &dst, inp->inp_boundif, &src);
        }
        if(inp->inp_boundif && tobroadcast)
            dst.s_addr = INADDR_BROADCAST;
    }

    /*
     * The IP header goes in front, first as the pseudo-header the
     * checksum is taken over, then as the header itself.
     */
    if(flags & INF_DG_ROOM) {
        m->m_data -= sizeof(struct ip);
        m->m_len += sizeof(struct ip);
        m->m_pkthdr.len += sizeof(struct ip);
    } else {
        int need = sizeof(struct ip) + (cksumoff < 0 ? 0 : cksumoff + (int)sizeof(u_int16_t));

        M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
        if(m == NULL)
            return (ENOBUFS);
        /* no room in front: the header is in an mbuf of its own, and the
         * checksum field must be where the store below puts it */
        if(m->m_len < need && (m = m_pullup(m, need)) == NULL)
            return (ENOBUFS);
    }
    ipov = mtod(m, struct ipovly *);
    bzero(ipov->ih_x1, sizeof(ipov->ih_x1));
    ipov->ih_pr = proto;
    ipov->ih_len = htons((u_short)len);
    ipov->ih_src = src;
    ipov->ih_dst = dst;
    if(cksumoff >= 0) {
        u_int16_t *sump = (u_int16_t *)(mtod(m, caddr_t) + sizeof(struct ip) + cksumoff);
        u_int16_t sum;

        *sump = 0;
        sum = in_cksum(m, len + (int)sizeof(struct ip));
        if(sum == 0 && (flags & INF_DG_NOTZERO))
            sum = 0xffff;
        *sump = sum;
    }

    /* ip_output() wants the length in host byte order and fills in the rest */
    ip = mtod(m, struct ip *);
    ip->ip_len = len + sizeof(struct ip);
    ip->ip_ttl = inp->inp_ip.ip_ttl;	/* XXX */
    ip->ip_tos = tos;
    /* IP_DONTFRAG: DF, so ip_output() says EMSGSIZE above the path MTU
     * instead of fragmenting (RFC 9000 section 14, FreeBSD) */
    if(inp->inp_flags & INP_DONTFRAG)
        ip->ip_off = IP_DF;

    ipflags = inp->inp_socket->so_options & (SO_DONTROUTE | SO_BROADCAST);
    if(inp->inp_boundif) {
        /* SO_BINDTODEVICE: ip_output() takes the interface from the header */
        m->m_pkthdr.rcvif = inp->inp_boundif;
        ipflags |= IP_BOUNDIF;
    }
    return (ip_output(m, inp->inp_options, &inp->inp_route, ipflags,
                      inp->inp_moptions));
}
