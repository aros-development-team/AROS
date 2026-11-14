/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the raw-socket implementation (netinet/raw_ip.c) asks of IPv6: the
 * IPv6 family's implementations of the "raw sockets" operations of struct
 * inetfamily (netinet/inetfamily.h), and IPv6's hand-over of a received
 * packet to the raw sockets (RFC 3542 section 3). Everything here is what
 * only an IPv6 raw socket has: the ICMPv6 filter, IPV6_CHECKSUM and the
 * checksum the stack always makes for ICMPv6, what a datagram's ancillary
 * data may ask for, hop limits and the traffic class in the header made
 * for it. (The ancillary data it receives is in6_savecontrol()'s, as for
 * a datagram socket.) Who gets a
 * packet, binding, connecting and sending are not here: they are the same
 * for every family.
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
#include <sys/queue.h>
#include <stdarg.h>
#include <string.h>      /* memcpy */

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/inetfamily.h>

#include <kern/uipc_socket2_protos.h>
#include <protos/kern/kern_subr_protos.h>
#include <protos/netinet/in_cksum_protos.h>
#include <netinet/raw_ip_protos.h>
#include <net/netcore_map.h>        /* always the last include */

extern const struct inetfamily in6_family;

/* The family's own part of a raw socket (behind its struct rawpcb). */
struct in6_rawpriv {
    struct icmp6_filter rp6_filter;     /* ICMPv6 types it receives (ICMP6_FILTER) */
};
#define intorip6(inp)   ((struct in6_rawpriv *)rawpcbpriv(intorawpcb(inp)))

/*
 * What in6_rawsendopts() leaves in a datagram's send options for
 * in6_rawoutput() (RFC 3542 section 6): opts[0] says which were given.
 */
#define RO6_HLIM        0x01    /* IPV6_HOPLIMIT, in opts[1] */
#define RO6_TCLASS      0x02    /* IPV6_TCLASS, in opts[2] */
#define RO6_MINMTU      0x04    /* IPV6_USE_MIN_MTU, in opts[3] */
#define RO6_DONTFRAG    0x08    /* IPV6_DONTFRAG was given, and is ... */
#define RO6_DF          0x10    /* ... set */

/*
 * inf_rawattach. The hop limit is the system's default until set. An
 * ICMPv6 socket receives every type until it sets a filter, and has its
 * checksum (at offset 2 of the message) made and verified always (RFC
 * 3542 section 3.1).
 */
void
in6_rawattach(struct inpcb *inp)
{
    struct rawpcb *rp = intorawpcb(inp);

    inp->in6p_hops = -1;
    ICMP6_FILTER_SETPASSALL(&intorip6(inp)->rp6_filter);
    if(rp->rp_proto == IPPROTO_ICMPV6)
        rp->rp_cksum = 2;
}

/*
 * inf_rawfilter: an ICMPv6 message of a type the socket's filter blocks
 * is not for it. (Asked once the socket has set a filter.)
 */
int
in6_rawfilter(struct inpcb *inp, struct mbuf *m, int off, int proto)
{
    u_int8_t type;

    if(proto != IPPROTO_ICMPV6 || m->m_pkthdr.len < off + 1)
        return (0);
    m_copydata(m, off, 1, (caddr_t)&type);
    return (ICMP6_FILTER_WILLBLOCK(type, &intorip6(inp)->rp6_filter) != 0);
}

/*
 * IPV6_CHECKSUM (RFC 3542 section 3.1): the offset, in the data, of a
 * 16-bit checksum over the data and the pseudo-header (RFC 8200 section
 * 8.1) that the kernel computes on output and checks on input; -1, the
 * default, is none. An ICMPv6 socket has its checksum computed and checked
 * always, and cannot set it (EINVAL; it reads 2, its offset); an odd offset
 * is refused, as FreeBSD does.
 */
static int
in6_rawcksumopt(int op, struct rawpcb *rp, struct mbuf **m)
{
    int error = 0;

    if(op == PRCO_SETOPT) {
        if(*m == NULL || (*m)->m_len != sizeof(int))
            error = EINVAL;
        else if(rp->rp_proto == IPPROTO_ICMPV6)
            error = EINVAL;
        else {
            int v = *mtod(*m, int *);

            if(v < -1 || (v >= 0 && (v & 1)))
                error = EINVAL;
            else
                rp->rp_cksum = v;
        }
    } else {
        *m = m_get(M_WAIT, MT_SOOPTS);
        if(*m == NULL)
            error = ENOBUFS;
        else {
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = rp->rp_cksum;
        }
    }
    return (error);
}

/*
 * inf_rawctloutput: IPV6_CHECKSUM, and the options of level ICMPv6 -
 * ICMP6_FILTER, on an ICMPv6 socket. Everything else is ip6_ctloutput()'s.
 */
int
in6_rawctloutput(int op, struct socket *so, int level, int optname, struct mbuf **m)
{
    struct inpcb *inp = sotoinpcb(so);
    struct rawpcb *rp = intorawpcb(inp);
    int error = 0;

    if(level == IPPROTO_IPV6 && optname == IPV6_CHECKSUM)
        error = in6_rawcksumopt(op, rp, m);
    else if(level != IPPROTO_ICMPV6)
        return (-1);
    else if(rp->rp_proto != IPPROTO_ICMPV6 || optname != ICMP6_FILTER)
        error = ENOPROTOOPT;
    else if(op == PRCO_SETOPT) {
        if(*m == NULL || (*m)->m_len != sizeof(struct icmp6_filter))
            error = EINVAL;
        else {
            bcopy(mtod(*m, caddr_t), (caddr_t)&intorip6(inp)->rp6_filter,
                  sizeof(struct icmp6_filter));
            /* from now on the socket's packets are put to the filter */
            rp->rp_flags |= RPF_FILTER;
        }
    } else {
        *m = m_get(M_WAIT, MT_SOOPTS);
        if(*m == NULL)
            error = ENOBUFS;
        else {
            (*m)->m_len = sizeof(struct icmp6_filter);
            bcopy((caddr_t)&intorip6(inp)->rp6_filter, mtod(*m, caddr_t),
                  sizeof(struct icmp6_filter));
        }
    }
    if(op == PRCO_SETOPT && *m) {
        m_freem(*m);
        *m = NULL;
    }
    return (error);
}

/*
 * inf_rawsendopts: what a datagram's ancillary data asks for (RFC 3542
 * section 6): IPV6_PKTINFO gives the source and the interface, the others
 * go to in6_rawoutput() in opts.
 */
int
in6_rawsendopts(struct inpcb *inp, struct mbuf *control, void *src,
                struct ifnet **oifp, int *opts)
{
    struct cmsghdr *cm;
    int left;

    cm = mtod(control, struct cmsghdr *);
    left = control->m_len;
    while(left > 0) {
        if(left < (int)sizeof(*cm) || cm->cmsg_len < sizeof(*cm) ||
                (int)cm->cmsg_len > left)
            return (EINVAL);
        if(cm->cmsg_level == IPPROTO_IPV6) {
            switch(cm->cmsg_type) {
            case IPV6_PKTINFO: {
                struct in6_pktinfo pi6;

                if(cm->cmsg_len != CMSG_LEN(sizeof(pi6)))
                    return (EINVAL);
                bcopy(CMSG_DATA(cm), (caddr_t)&pi6, sizeof(pi6));
                *(struct in6_addr *)src = pi6.ipi6_addr;
                if(pi6.ipi6_ifindex != 0) {
                    struct ifnet *ifp;

                    for(ifp = ifnet_list; ifp; ifp = ifp->if_next)
                        if((unsigned int)ifp->if_index == pi6.ipi6_ifindex)
                            break;
                    if(ifp == NULL)
                        return (ENXIO);
                    *oifp = ifp;
                }
                break;
            }
            case IPV6_HOPLIMIT: {
                int hlim;

                if(cm->cmsg_len != CMSG_LEN(sizeof(int)))
                    return (EINVAL);
                memcpy(&hlim, CMSG_DATA(cm), sizeof(int));
                if(hlim < -1 || hlim > 255)
                    return (EINVAL);
                opts[0] &= ~RO6_HLIM;       /* -1: the socket's */
                if(hlim >= 0) {
                    opts[0] |= RO6_HLIM;
                    opts[1] = hlim;
                }
                break;
            }
            case IPV6_TCLASS: {         /* section 6.5: -1 is 0 */
                int v;

                if(cm->cmsg_len != CMSG_LEN(sizeof(int)))
                    return (EINVAL);
                memcpy(&v, CMSG_DATA(cm), sizeof(int));
                if(v < -1 || v > 255)
                    return (EINVAL);
                opts[0] |= RO6_TCLASS;
                opts[2] = v < 0 ? 0 : v;
                break;
            }
            case IPV6_USE_MIN_MTU:      /* RFC 3542 section 11 */
            case IPV6_DONTFRAG: {
                int v;

                if(cm->cmsg_len != CMSG_LEN(sizeof(int)))
                    return (EINVAL);
                memcpy(&v, CMSG_DATA(cm), sizeof(int));
                if(cm->cmsg_type == IPV6_DONTFRAG)
                    opts[0] = (opts[0] & ~RO6_DF) | RO6_DONTFRAG | (v ? RO6_DF : 0);
                else if(v < -1 || v > 1)
                    return (EINVAL);
                else {
                    opts[0] |= RO6_MINMTU;
                    opts[3] = v;
                }
                break;
            }
            default:
                return (ENOPROTOOPT);
            }
        }
        if(left < (int)_ALIGN(cm->cmsg_len))
            break;
        left -= _ALIGN(cm->cmsg_len);
        cm = (struct cmsghdr *)((caddr_t)cm + _ALIGN(cm->cmsg_len));
    }
    return (0);
}

/*
 * inf_rawoutput - send m, the data after the IPv6 header. The checksum at
 * cksumoff (ICMPv6's, or the IPV6_CHECKSUM one) is computed here. The hop
 * limit is IPV6_HOPLIMIT from the ancillary data, else IPV6_MULTICAST_HOPS
 * or IPV6_UNICAST_HOPS, else the default (1 for multicast). A source that
 * is unspecified stays so: a raw socket may send from :: (a router
 * solicitation, a duplicate address probe).
 */
int
in6_rawoutput(struct mbuf *m, struct inpcb *inp, const void *laddr, const void *faddr,
    int proto, int cksumoff, struct ifnet *oifp, const int *opts)
{
    const struct in6_addr *dst = (const struct in6_addr *)faddr;
    struct ip6_moptions oim6o, *im6o = inp->in6p_moptions;
    int plen = m->m_pkthdr.len;
    int hlim = -1;
    int minmtu = IN6P_MINMTU_OPT(inp);
    int dontfrag = (inp->in6p_flags & IN6P_DONTFRAG) != 0;
    int tclass = inp->in6p_tclass;
    struct ip6_hdr *ip6;

    if(opts != NULL) {
        if(opts[0] & RO6_HLIM)
            hlim = opts[1];
        if(opts[0] & RO6_TCLASS)
            tclass = opts[2];
        if(opts[0] & RO6_MINMTU)
            minmtu = opts[3];
        if(opts[0] & RO6_DONTFRAG)
            dontfrag = (opts[0] & RO6_DF) != 0;
    }

    M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
    if(m == NULL)
        return (ENOBUFS);
    ip6 = mtod(m, struct ip6_hdr *);
    /* version 6 and the traffic class; no flow label */
    ip6->ip6_flow = htonl(0x60000000 | ((u_int32_t)tclass << 20));
    ip6->ip6_plen = htons((u_int16_t)plen);
    ip6->ip6_nxt  = (u_int8_t)proto;
    ip6->ip6_src  = *(const struct in6_addr *)laddr;
    ip6->ip6_dst  = *dst;

    if(hlim >= 0)
        ip6->ip6_hlim = (u_int8_t)hlim;
    else if(IN6_IS_ADDR_MULTICAST(dst))
        ip6->ip6_hlim = im6o ? im6o->im6o_multicast_hlim
                        : IP6_DEFAULT_MULTICAST_HLIM;
    else
        ip6->ip6_hlim = (inp->in6p_hops >= 0) ? (u_int8_t)inp->in6p_hops
                        : (u_int8_t)ip6_defhlim;

    if(cksumoff >= 0) {
        u_int16_t sum = 0;
        int coff = sizeof(struct ip6_hdr) + cksumoff;

        m_copyback(m, coff, sizeof(sum), (caddr_t)&sum);
        sum = in6_cksum(m, proto, sizeof(struct ip6_hdr), plen);
        m_copyback(m, coff, sizeof(sum), (caddr_t)&sum);
    }

    if(oifp != NULL) {
        /* hand the interface on the way multicast options do */
        if(im6o != NULL)
            oim6o = *im6o;
        else {
            bzero(&oim6o, sizeof(oim6o));
            oim6o.im6o_multicast_hlim = IP6_DEFAULT_MULTICAST_HLIM;
            oim6o.im6o_multicast_loop = 1;
        }
        oim6o.im6o_multicast_ifp = oifp;
        im6o = &oim6o;
    }
    return (ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL,
                       ip6_outflags(minmtu, dontfrag, dst),
                       im6o, (struct ifnet **)NULL, inp));
}

#endif /* INET6 */
