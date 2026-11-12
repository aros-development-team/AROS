/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * IPv6's instance of the family interface (netinet/inetfamily.h): what the
 * transports and the PCB layer ask of IPv6. Everything in this file is
 * IPv6's own; nothing outside the IPv6 part calls it except through the
 * interface.
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

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/inetfamily.h>
#include <netinet/icmp6.h>

#include <netinet/tcp.h>
#include <protos/net/route_protos.h>
#include <stddef.h>             /* offsetof */
#include <stdarg.h>
#include <net/netcore_map.h>        /* always the last include */

extern int ip6_ctloutput(int, struct socket *, int, int, struct mbuf **);
extern int in6_cksum(struct mbuf *, u_int8_t, u_int32_t, u_int32_t);
extern int ip6_output(void *, ...);     /* (m, opt, ro, flags, im6o, ifpp, inp) */
extern int ip6_defhlim;                 /* the default hop limit */

/*
 * The MSS IPv6 assumes when nothing says otherwise: the minimum link MTU
 * less the IPv6 and TCP headers (1280 - 60); what a peer that sends no MSS
 * option takes (RFC 9293 section 3.7.1) and the most to use towards a
 * destination off the link with no path MTU known.
 */
int     tcp_v6mssdflt = 1220;

/* IPv6 stops (inet6_stop()): this file's variable as the file has it */
void
in6_family_defaults(void)
{
    tcp_v6mssdflt = 1220;
}

/*
 * The errno a transport reports for each PRC_xxx an ICMPv6 error is turned
 * into (as KAME's inet6ctlerrmap; the values IPv4's map has).
 */
static const u_char inet6ctlerrmap[PRC_NCMDS] = {
    0,              0,              0,              0,
    0,              EMSGSIZE,       EHOSTDOWN,      EHOSTUNREACH,
    EHOSTUNREACH,   EHOSTUNREACH,   ECONNREFUSED,   ECONNREFUSED,
    EMSGSIZE,       EHOSTUNREACH,   0,              0,
    0,              0,              0,              0,
    ENOPROTOOPT
};

/* Registered from ip6_init(). */
static int
in6_sa_multicast(struct mbuf *nam)
{
    struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);

    return (sin6->sin6_family == AF_INET6 && IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr));
}

static int
in6_peer_local(struct inpcb *inp)
{
    return (in6_localaddr(&inp->inp_faddr6));
}

static void
in6_pcbfree(struct inpcb *inp)
{
    ip6_freemoptions(inp->in6p_moptions);
}

/*
 * inf_pcbifgone: the interface IPV6_MULTICAST_IF chose is forgotten, and
 * the group memberships taken on it are left - as in_pcbmoptifgone(), the
 * IPv4 counterpart: IPV6_LEAVE_GROUP could not name the interface any
 * more. The last holder of a group cuts it loose from the offline
 * interface first, so that in6_delmulti() sends no MLD Done.
 */
static void
in6_pcbmoptifgone(struct inpcb *inp, const struct ifnet *ifp)
{
    struct ip6_moptions *im6o = inp->in6p_moptions;
    int i, j;

    if(im6o == NULL)
        return;
    if(im6o->im6o_multicast_ifp == ifp)
        im6o->im6o_multicast_ifp = NULL;
    for(i = j = 0; i < im6o->im6o_num_memberships; i++) {
        struct in6_multi *in6m = im6o->im6o_membership[i];

        if(in6m->in6m_ifp != ifp && in6m->in6m_ifp != NULL) {
            im6o->im6o_membership[j++] = in6m;
            continue;
        }
        if(in6m->in6m_refcount == 1) {
            in6m->in6m_timer = 0;
            in6m->in6m_ifp = NULL;
        }
        in6_delmulti(in6m);
    }
    im6o->im6o_num_memberships = j;
}

/* ---- per packet: receive information, checksum, output ---- */

/*
 * The IPv6 header at the front of an mbuf's data, which is an even
 * address. struct ip6_hdr is packed: through a plain pointer to it every
 * field wider than a byte, and each address, is moved a byte at a time (or
 * by a call) on machines that cannot access a word at an odd address (the
 * 68000).
 */
typedef struct ip6_hdr ip6_ahdr_t __attribute__((__aligned__(2)));

/*
 * A packet for a transport: the IPv6 header is at the front of the mbuf
 * as it arrived, the transport header at off, after any extension headers.
 */
static int
in6_rcvinfo(struct mbuf *m, int off, const void **src, const void **dst,
    int *tos, int *hlim, int *ulen)
{
    ip6_ahdr_t *ip6 = mtod(m, ip6_ahdr_t *);

    *src = &ip6->ip6_src;
    *dst = &ip6->ip6_dst;
    *tos = (ntohl(ip6->ip6_flow) >> 20) & 0xff;         /* the traffic class */
    *hlim = ip6->ip6_hlim;
    *ulen = (int)ntohs(ip6->ip6_plen) - (off - (int)sizeof(struct ip6_hdr));
    return (*ulen < 0 ? EINVAL : 0);
}

/* in6_cksum() takes the addresses of the pseudo-header from the IPv6 header */
static int
in6_tpcksum(struct mbuf *m, int proto, int off, int len, const void *src, const void *dst)
{
    return (in6_cksum(m, proto, off, len));
}

/*
 * Send a transport's segment: prepend and fill the IPv6 header, take the
 * checksum over the IPv6 pseudo-header and hand the packet to ip6_output().
 */
static int
in6_tpoutput(struct mbuf *m, struct inpcb *inp, const void *laddr, const void *faddr,
    int proto, int cksumoff, int tos, int hlim, int flags)
{
    int len = m->m_pkthdr.len;          /* transport header and data */
    ip6_ahdr_t *ip6;
    u_int16_t *sump;
    int tclass = tos;

    if(flags & INF_OUT_ROOM) {
        m->m_data -= sizeof(struct ip6_hdr);
        m->m_len += sizeof(struct ip6_hdr);
        m->m_pkthdr.len += sizeof(struct ip6_hdr);
    } else {
        M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
        if(m == NULL)
            return (ENOBUFS);
        /*
         * M_PREPEND may have allocated a fresh mbuf for the IPv6 header,
         * leaving the transport header in the following mbuf. Pull up as far
         * as the checksum field, so the store below lands on it.
         */
        if(m->m_len < sizeof(struct ip6_hdr) + cksumoff + sizeof(u_int16_t) &&
                (m = m_pullup(m, sizeof(struct ip6_hdr) + cksumoff + sizeof(u_int16_t))) == NULL)
            return (ENOBUFS);
    }
    /* the socket's IPV6_TCLASS; the ECN field is the transport's, as for IPv4 */
    if(inp != NULL && !(flags & INF_OUT_BARE))
        tclass |= inp->in6p_tclass & ~IPTOS_ECN_MASK;
    if(hlim == 0 && inp != NULL)
        hlim = inp->in6p_hops;          /* -1: the default */

    ip6 = mtod(m, ip6_ahdr_t *);
    /* version 6 and the traffic class; no flow label */
    ip6->ip6_flow = htonl(0x60000000 | ((u_int32_t)(tclass & 0xff) << 20));
    ip6->ip6_plen = htons((u_short)len);
    ip6->ip6_nxt  = proto;
    ip6->ip6_hlim = (hlim > 0) ? (u_int8_t)hlim : (u_int8_t)ip6_defhlim;
    /* (the transports' addresses are in PCBs or copies as even-aligned) */
    inpcb_addrcopy(&ip6->ip6_src, laddr, sizeof(struct in6_addr));
    inpcb_addrcopy(&ip6->ip6_dst, faddr, sizeof(struct in6_addr));

    sump = (u_int16_t *)(mtod(m, caddr_t) + sizeof(struct ip6_hdr) + cksumoff);
    *sump = 0;
    *sump = in6_cksum(m, proto, sizeof(struct ip6_hdr), (u_int32_t)len);

    return (ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
                       inp ? inp->in6p_moptions : (struct ip6_moptions *)NULL,
                       (struct ifnet **)NULL, inp));
}

static int
in6_addr_multicast(const void *addr)
{
    return (IN6_IS_ADDR_MULTICAST((const struct in6_addr *)addr));
}

/* there is no access control list for IPv6 peers yet (DEFECTS.md, defect 4) */
static int
in6_acceptok(const void *src, u_int lport)
{
    return (1);
}

/*
 * The MTU towards faddr: the path MTU of the route, else the MTU of the
 * link the peer is on (zone, if given; else the route's), else what gives
 * the default MSS.
 */
static u_int32_t
in6_mtu(const void *faddr, const struct ifnet *zone)
{
    struct sockaddr_in6 dst;
    struct rtentry *rt;
    u_int32_t mtu;

    bzero(&dst, sizeof(dst));
    dst.sin6_family = AF_INET6;
    dst.sin6_len    = sizeof(dst);
    dst.sin6_addr   = *(const struct in6_addr *)faddr;
    rt = rtalloc1((struct sockaddr *)&dst, 0);
    if(rt != NULL && rt->rt_rmx.rmx_mtu)
        mtu = rt->rt_rmx.rmx_mtu;
    else if(zone != NULL)
        mtu = zone->if_mtu;
    else if(rt != NULL)
        mtu = rt->rt_ifp->if_mtu;
    else
        mtu = tcp_v6mssdflt + sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
    if(rt != NULL)
        rtfree(rt);
    return (mtu);
}

/*
 * The route to the PCB's peer. inp_route has no room for an IPv6
 * destination, so the route is looked up each time and comes back with a
 * reference the caller drops with rtfree() (INF_RTREF). NULL if there is no
 * peer or no route.
 */
static struct rtentry *
in6_pcbroute(struct inpcb *inp)
{
    struct sockaddr_in6 dst;

    if(IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6))
        return (NULL);
    bzero(&dst, sizeof(dst));
    dst.sin6_family = AF_INET6;
    dst.sin6_len    = sizeof(dst);
    dst.sin6_addr   = inp->inp_faddr6;
    return (rtalloc1((struct sockaddr *)&dst, 1));
}

extern const struct inetfamily in6_family;

/*
 * inf_pcbinit: what a new IPv6 PCB starts with besides what
 * in_pcballoc() gave it - the hop limit is the system default (-1) until
 * IPV6_UNICAST_HOPS says otherwise.
 */
static void
in6_pcbdefaults(struct inpcb *inp)
{
    in6_pcbinit(inp);
    inp->in6p_hops = -1;
}

/* UDP and raw sockets (netinet6/in6_family_udp.c) */
int     in6_rcvfrom(struct mbuf *, const void *, u_int, const void *, struct sockaddr *);
struct mbuf **in6_savecontrol(struct inpcb *, const struct ifnet *, const void *,
                              int, int, struct mbuf **);
void    in6_unreach(struct mbuf *, int);
int     in6_dgoutput(struct mbuf *, struct inpcb *, const void *, struct ifnet *,
                     struct mbuf *, int, int, int);
void    in6_pmtunotify(struct inpcb *, struct sockaddr *, u_int32_t);

void in6_rawattach(struct inpcb *);
int  in6_rawfilter(struct inpcb *, struct mbuf *, int, int);
int  in6_rawctloutput(int, struct socket *, int, int, struct mbuf **);
int  in6_rawsendopts(struct inpcb *, struct mbuf *, void *, struct ifnet **, int *);
int  in6_rawoutput(struct mbuf *, struct inpcb *, const void *, const void *, int, int, struct ifnet *, const int *);
extern int in6_control(struct socket *, int, caddr_t, struct ifnet *);

const struct inetfamily in6_family = {
    .inf_size           = sizeof(struct inetfamily),
    .inf_family         = AF_INET6,
    .inf_optlevel       = IPPROTO_IPV6,

    .inf_ctloutput      = ip6_ctloutput,
    .inf_sa_multicast   = in6_sa_multicast,
    .inf_peer_local     = in6_peer_local,
    .inf_pcbfree        = in6_pcbfree,
    .inf_pcbifgone      = in6_pcbmoptifgone,

    /* per packet */
    .inf_addrlen        = sizeof(struct in6_addr),
    .inf_hdrlen         = sizeof(struct ip6_hdr),
    .inf_mssdflt        = &tcp_v6mssdflt,
    .inf_bhmtu          = IPV6_MMTU,
    .inf_flags          = INF_CKSUM_REQUIRED | INF_ZONES | INF_RTREF,
    .inf_rcvinfo        = in6_rcvinfo,
    .inf_cksum          = in6_tpcksum,
    .inf_output         = in6_tpoutput,
    .inf_addr_multicast = in6_addr_multicast,
    .inf_acceptok       = in6_acceptok,
    .inf_mtu            = in6_mtu,
    .inf_pcbroute       = in6_pcbroute,

    /* PCB layer (netinet6/in6_family_pcb.c) */
    .inf_salen          = sizeof(struct sockaddr_in6),
    .inf_ctlerrmap      = inet6ctlerrmap,
    .inf_pcbinit        = in6_pcbdefaults,
    .inf_pcbanyfamily   = in6_pcbanyfamily,
    .inf_sockaddr_parse = in6_sockaddr_parse,
    .inf_mksockaddr     = in6_mksockaddr,
    .inf_addr_from      = in6_addr_from,
    .inf_pcbzone        = in6_pcbzone,
    .inf_ownaddr        = in6_ownaddr,
    .inf_selectsrc      = in6_selectsrc,
    .inf_pcbbindaddr    = in6_pcbbindaddr,
    .inf_pcbladdr       = in6_pcbladdr,

    /* UDP and raw sockets (netinet6/in6_family_udp.c) */
    .inf_rcvopt_off     = offsetof(struct inpcb, in6p_flags),
    .inf_rcvopt_mask    = IN6P_PKTINFO | IN6P_HOPLIMIT | IN6P_TCLASS,
    .inf_rcvfrom        = in6_rcvfrom,
    .inf_savecontrol    = in6_savecontrol,
    .inf_unreach        = in6_unreach,
    .inf_dgoutput       = in6_dgoutput,
    .inf_pmtunotify     = in6_pmtunotify,
    .inf_control        = in6_control,

    /* raw sockets (netinet6/in6_family_raw.c) */
    .inf_rawprivlen     = sizeof(struct icmp6_filter),
    .inf_rawattach      = in6_rawattach,
    .inf_rawfilter      = in6_rawfilter,
    .inf_rawctloutput   = in6_rawctloutput,
    .inf_rawsendopts    = in6_rawsendopts,
    .inf_rawoutput      = in6_rawoutput,
};
