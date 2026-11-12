/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * IPv4's instance of the family interface (netinet/inetfamily.h): what the
 * transports and the PCB layer ask of IPv4. Everything in this file is
 * IPv4's own; nothing outside the IPv4 part calls it except through the
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
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include "in_var.h"             /* the stack's own: struct ip_moptions */
#include <netinet/ip_var.h>
#include <netinet/inetfamily.h>
#include <netinet/in_protos.h>
#include <netinet/ip_output_protos.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/in_cksum_protos.h>
#include <protos/net/route_protos.h>
#include <kern/accesscontrol.h>
#include <stddef.h>             /* offsetof */
#include <stdarg.h>
#include <net/netcore_map.h>        /* always the last include */

extern u_char inetctlerrmap[];

/* the TTL a packet is sent with when its socket says nothing else */
int     ip_defttl = 60;

/* "any", for those of IPv4's functions that take an address by value */
struct in_addr zeroin_addr;

/*
 * The MSS IPv4 assumes when nothing says otherwise: Ethernet's (1500 - 40).
 * It can be set while the stack runs (net.inet.tcp.mssdflt).
 */
int     tcp_mssdflt = 1460;

/* IPv4 stops (inet_stop()): this file's variables as the file has them */
void
in_family_defaults(void)
{
    ip_defttl = 60;
    zeroin_addr.s_addr = 0;
    tcp_mssdflt = 1460;
}

/* Registered from ip_init(). */
static int
in_sa_multicast(struct mbuf *nam)
{
    struct sockaddr_in *sin = mtod(nam, struct sockaddr_in *);

    return (sin->sin_family == AF_INET && IN_MULTICAST(ntohl(sin->sin_addr.s_addr)));
}

static int
in_peer_local(struct inpcb *inp)
{
    return (in_localaddr(inp->inp_faddr));
}

static void
in_pcbfree(struct inpcb *inp)
{
#ifdef ENABLE_MULTICAST
    ip_freemoptions(inp->inp_moptions);
#endif
}

/* inf_pcbifgone: the interface IP_MULTICAST_IF chose is forgotten */
static void
in_pcbmoptifgone(struct inpcb *inp, const struct ifnet *ifp)
{
    if(inp->inp_moptions && inp->inp_moptions->imo_multicast_ifp == ifp)
        inp->inp_moptions->imo_multicast_ifp = NULL;
}

/* ---- per packet: receive information, checksum, output ---- */

/*
 * The IP header at the front of an mbuf's data, which is an even address.
 * struct ip is packed: through a plain pointer to it every 16- and 32-bit
 * field is read and written a byte at a time on machines that cannot
 * access a word at an odd address (the 68000).
 */
typedef struct ip ip_ahdr_t __attribute__((__aligned__(2)));

/*
 * A packet for a transport. ip_input() left the header at the front of the
 * mbuf, ip_len in host byte order and less the header.
 */
static int
in_rcvinfo(struct mbuf *m, int off, const void **src, const void **dst,
    int *tos, int *hlim, int *ulen)
{
    ip_ahdr_t *ip = mtod(m, ip_ahdr_t *);

    *src = &ip->ip_src;
    *dst = &ip->ip_dst;
    *tos = ip->ip_tos;
    *hlim = ip->ip_ttl;
    *ulen = ip->ip_len;
    return (0);
}

/*
 * The transport checksum with IPv4's pseudo-header. The data is summed
 * where it lies, from the transport header on, and the twelve bytes of
 * the pseudo-header are added to that sum: nothing is written into the
 * packet, so its IP header stays as ip_input() left it - which the ICMP
 * error for a datagram nobody listens for needs (in_unreach()), without a
 * copy of the header being kept for every datagram.
 */
static int
in_tpcksum(struct mbuf *m, int proto, int off, int len, const void *src, const void *dst)
{
    /* the addresses a 16-bit word at a time, as in_cksum() sums the data */
    typedef u_int16_t __attribute__((__may_alias__)) ck_word_t;
    const ck_word_t *s = (const ck_word_t *)src, *d = (const ck_word_t *)dst;
    u_int32_t sum;

    m->m_data += off;
    m->m_len -= off;
    sum = ~in_cksum(m, len) & 0xffff;   /* the sum itself, not its complement */
    m->m_data -= off;
    m->m_len += off;
    sum += s[0];
    sum += s[1];
    sum += d[0];
    sum += d[1];
    sum += htons((u_short)proto);
    sum += htons((u_short)len);
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return (~sum & 0xffff);
}

/*
 * Send a transport's segment: the checksum is taken as for a segment that
 * arrives - the segment summed where it lies, the pseudo-header added to
 * the sum (in_tpcksum()) - so nothing is built in the packet for it, and
 * the IP header is written once, in front.
 */
static int
in_tpoutput(struct mbuf *m, struct inpcb *inp, const void *laddr, const void *faddr,
    int proto, int cksumoff, int tos, int hlim, int flags)
{
    int len = m->m_pkthdr.len;          /* transport header and data */
    int bare = (inp == NULL || (flags & INF_OUT_BARE));
    ip_ahdr_t *ip;
    inp_pktword_t *w;
    u_int16_t *sump;

    if(flags & INF_OUT_ROOM) {
        m->m_data -= sizeof(struct ip);
        m->m_len += sizeof(struct ip);
        m->m_pkthdr.len += sizeof(struct ip);
    } else {
        M_PREPEND(m, sizeof(struct ip), M_DONTWAIT);
        if(m == NULL)
            return (ENOBUFS);
        /* no room in front: the header is in an mbuf of its own, and the
         * checksum field must be where the store below puts it */
        if(m->m_len < sizeof(struct ip) + cksumoff + sizeof(u_int16_t) &&
                (m = m_pullup(m, sizeof(struct ip) + cksumoff + sizeof(u_int16_t))) == NULL)
            return (ENOBUFS);
    }
    sump = (u_int16_t *)(mtod(m, caddr_t) + sizeof(struct ip) + cksumoff);
    *sump = 0;
    *sump = in_tpcksum(m, proto, sizeof(struct ip), len, laddr, faddr);

    /*
     * The header: all of it is written here or by ip_output(), which
     * wants the length in host byte order and fills in the version, the
     * header length, the identification and the header checksum. (The
     * first twelve bytes are cleared as three words; the room may hold
     * anything.)
     */
    ip = mtod(m, ip_ahdr_t *);
    w = (inp_pktword_t *)ip;
    w[0] = 0;
    w[1] = 0;
    w[2] = 0;
    ip->ip_p = proto;
    ip->ip_len = len + sizeof(struct ip);
    ip->ip_ttl = hlim ? hlim : bare ? ip_defttl : inp->inp_ip.ip_ttl;
    /* the ECN field is the transport's (RFC 3168 section 6.1), not IP_TOS's */
    ip->ip_tos = (bare ? 0 : (inp->inp_ip.ip_tos & ~IPTOS_ECN_MASK)) | tos;
    if(flags & INF_OUT_DF)
        ip->ip_off = IP_DF;
    ip->ip_src = *(const struct in_addr *)laddr;
    ip->ip_dst = *(const struct in_addr *)faddr;

    if(inp == NULL)
        return (ip_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
                          (struct ip_moptions *)NULL));
    if(bare)
        return (ip_output(m, (struct mbuf *)NULL, &inp->inp_route, 0,
                          (struct ip_moptions *)NULL));
    return (ip_output(m, inp->inp_options, &inp->inp_route,
                      inp->inp_socket->so_options & SO_DONTROUTE,
                      (struct ip_moptions *)NULL));
}

static int
in_addr_multicast(const void *addr)
{
    return (IN_MULTICAST(ntohl(((const struct in_addr *)addr)->s_addr)));
}

/*
 * The AmiTCP/IP access control list (kern/accesscontrol.c). TCP gives the
 * port as it is in the header (network order); the list, and its
 * "privileged port" test, are in host order.
 */
static int
in_acceptok(const void *src, u_int lport)
{
    return (controlaccess(*(const struct in_addr *)src, ntohs((u_short)lport)) != 0);
}

/*
 * The MTU assumed when answering a connection request: IPv4 has always
 * answered with its default MSS, whatever the route; so the MTU that
 * gives it.
 */
static u_int32_t
in_synmtu(const void *faddr, const struct ifnet *zone)
{
    return (tcp_mssdflt + sizeof(struct ip) + sizeof(struct tcphdr));
}

/*
 * The route to the PCB's peer, kept in the PCB (inp_route): looked up if
 * there is none or it went down. No reference for the caller.
 */
static struct rtentry *
in_pcbroute(struct inpcb *inp)
{
    struct route *ro = &inp->inp_route;
    struct rtentry *rt = ro->ro_rt;

    if(rt == NULL || !(rt->rt_flags & RTF_UP)) {
        /* No route yet, so try to acquire one */
        if(inp->inp_faddr.s_addr != INADDR_ANY) {
            struct sockaddr_in *rodst_saddr = (struct sockaddr_in *)&ro->ro_dst;

            ro->ro_dst.sa_family = AF_INET;
            ro->ro_dst.sa_len = sizeof(ro->ro_dst);
            rodst_saddr->sin_addr = inp->inp_faddr;
            rtalloc(ro);
            rt = ro->ro_rt;
        }
    }
    return (rt);
}

extern const struct inetfamily in_family;

/*
 * inf_pcbinit: what a new IPv4 PCB starts with besides what
 * in_pcballoc() gave it - the default TTL.
 */
static void
in_pcbdefaults(struct inpcb *inp)
{
    in_pcbinit(inp);
    inp->inp_ip.ip_ttl = ip_defttl;
}

/* UDP and raw sockets (netinet/in_family_udp.c) */
int     in_rcvfrom(struct mbuf *, const void *, u_int, const void *, struct sockaddr *);
struct mbuf **in_savecontrol(struct inpcb *, const struct ifnet *, const void *,
                             int, int, struct mbuf **);
void    in_unreach(struct mbuf *, int);
int     in_dgoutput(struct mbuf *, struct inpcb *, const void *, struct ifnet *,
                    struct mbuf *, int, int, int);

void in_rawattach(struct inpcb *);
int  in_rawctloutput(int, struct socket *, int, int, struct mbuf **);
int  in_rawoutput(struct mbuf *, struct inpcb *, const void *, const void *, int, int, struct ifnet *, const int *);

/* a protocol outside inetsw[] takes an IP protocol number (netinet/ip_input.c) */
int  ip_proto_register(int, struct protosw *);
int  ip_proto_unregister(int, struct protosw *);

const struct inetfamily in_family = {
    .inf_size           = sizeof(struct inetfamily),
    .inf_family         = AF_INET,
    .inf_optlevel       = IPPROTO_IP,

    .inf_ctloutput      = ip_ctloutput,
    .inf_sa_multicast   = in_sa_multicast,
    .inf_peer_local     = in_peer_local,
    .inf_pcbfree        = in_pcbfree,
    .inf_pcbifgone      = in_pcbmoptifgone,

    /* per packet */
    .inf_addrlen        = sizeof(struct in_addr),
    .inf_hdrlen         = sizeof(struct ip),
    .inf_mssdflt        = &tcp_mssdflt,
    .inf_bhmtu          = 1240,
    .inf_flags          = INF_HASDF,
    .inf_rcvinfo        = in_rcvinfo,
    .inf_cksum          = in_tpcksum,
    .inf_output         = in_tpoutput,
    .inf_addr_multicast = in_addr_multicast,
    .inf_acceptok       = in_acceptok,
    .inf_mtu            = in_synmtu,
    .inf_pcbroute       = in_pcbroute,

    /* PCB layer (netinet/in_family_pcb.c) */
    .inf_salen          = sizeof(struct sockaddr_in),
    .inf_ctlerrmap      = inetctlerrmap,
    .inf_pcbinit        = in_pcbdefaults,
    .inf_pcbanyfamily   = in_pcbanyfamily,
    .inf_sockaddr_parse = in_sockaddr_parse,
    .inf_mksockaddr     = in_mksockaddr,
    .inf_addr_from      = in_addr_from,
    .inf_pcbzone        = in_pcbzone,
    .inf_ownaddr        = in_ownaddr,
    .inf_selectsrc      = in_selectsrc,
    .inf_pcbbindaddr    = in_pcbbindaddr,
    .inf_pcbladdr       = in_pcbladdr,

    /* UDP and raw sockets (netinet/in_family_udp.c) */
    .inf_rcvopt_off     = offsetof(struct inpcb, inp_flags),
    .inf_rcvopt_mask    = INP_CONTROLOPTS,
    .inf_rcvfrom        = in_rcvfrom,
    .inf_savecontrol    = in_savecontrol,
    .inf_unreach        = in_unreach,
    .inf_dgoutput       = in_dgoutput,
    /* no inf_pmtunotify: the route keeps the path MTU, sockets are not told */
    .inf_control        = in_control,

    /* raw sockets (netinet/in_family_raw.c) */
    .inf_rawflags       = INFR_HDR | INFR_SRCLATE,
    .inf_rawattach      = in_rawattach,
    .inf_rawctloutput   = in_rawctloutput,
    .inf_rawoutput      = in_rawoutput,

    /* protocols outside inetsw[] */
    .inf_proto_register   = ip_proto_register,
    .inf_proto_unregister = ip_proto_unregister,
};
