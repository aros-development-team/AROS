/*
 * Copyright (C) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Raw IPv6 sockets (RFC 3542 section 3).
 * Derived from KAME/FreeBSD 4.11 netinet6/raw_ip6.c.
 *
 * A raw IPv6 socket has an inpcb on its own list, so the IPv6 socket
 * options (hop limits, multicast interface, memberships, ancillary data
 * requests) are the ones ip6_ctloutput() keeps for UDP. What only a raw
 * socket has, its protocol and its ICMPv6 filter, lives in a struct
 * rip6pcb hung off inp_ppcb.
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

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>

#include <protos/net/route_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <protos/kern/kern_subr_protos.h>
#include <protos/netinet/in_cksum_protos.h>

/* Explicit declarations (in_pcb_protos.h has stale signatures) */
extern int  in_pcballoc(struct socket *, struct inpcbinfo *);
extern void in_pcbdetach(struct inpcb *);
extern int  soreserve(struct socket *, u_long, u_long);
extern void in6_setsockaddr(struct inpcb *, struct mbuf *);
extern void in6_setpeeraddr(struct inpcb *, struct mbuf *);
extern struct in6_ifaddr *in6_ifawithifp(struct ifnet *, struct in6_addr *);
extern int  ip6_ctloutput(int, struct socket *, int, int, struct mbuf **);
extern int  ip6_output(void *args, ...);
extern int  in6_control(struct socket *, int, caddr_t, struct ifnet *);
extern struct ifnet *ifnet;
extern struct in6_ifaddr *in6_ifaddr;
extern int  ip6_defhlim;

/* What only a raw socket has; inp_ppcb points to it. */
struct rip6pcb {
    int                 rp_proto;   /* protocol; 0 takes every protocol */
    struct icmp6_filter rp_filter;  /* ICMPv6 types it receives (ICMP6_FILTER) */
};
#define intorip6(inp)	((struct rip6pcb *)(inp)->inp_ppcb)

static struct inpcbhead  rip6cb;
static struct inpcbinfo  rip6cbinfo;
static u_long rip6_sendspace = 8192;
static u_long rip6_recvspace = 8192;

void
rip6_init(void)
{
    LIST_INIT(&rip6cb);
    rip6cbinfo.listhead = &rip6cb;
    rip6cbinfo.hashbase = phashinit(8, M_PCB, &rip6cbinfo.hashsize);
}

static struct ifnet *
rip6_ifbyindex(unsigned int index)
{
    struct ifnet *ifp;

    for(ifp = ifnet; ifp; ifp = ifp->if_next)
        if((unsigned int)ifp->if_index == index)
            return ifp;
    return NULL;
}

/* One of our addresses, usable as a source: not tentative or duplicated. */
static int
rip6_ownaddr(const struct in6_addr *addr)
{
    struct in6_ifaddr *ia;

    for(ia = in6_ifaddr; ia; ia = ia->ia_next)
        if(IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, addr))
            return !(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED));
    return 0;
}

/* ------------------------------------------------------------------
 * Input.
 * ------------------------------------------------------------------ */

/*
 * Queue m on inp's socket: without the IPv6 header and any extension
 * headers before off (RFC 3542 section 3), with the ancillary data the
 * socket asked for, taken from the header while it is still there.
 */
static void
rip6_append(struct inpcb *inp, struct mbuf *m, int off,
            struct sockaddr_in6 *from)
{
    struct socket *so = inp->inp_socket;
    struct mbuf *opts = NULL, **optp = &opts;

    if(inp->in6p_flags & (IN6P_PKTINFO | IN6P_HOPLIMIT)) {
        struct ip6_hdr ip6;

        bzero(&ip6, sizeof(ip6));
        m_copydata(m, 0, sizeof(ip6), (caddr_t)&ip6);
        if(inp->in6p_flags & IN6P_PKTINFO) {
            struct in6_pktinfo pi6;

            bzero(&pi6, sizeof(pi6));
            pi6.ipi6_addr    = ip6.ip6_dst;
            pi6.ipi6_ifindex = (m->m_pkthdr.rcvif != NULL)
                               ? m->m_pkthdr.rcvif->if_index : 0;
            *optp = sbcreatecontrol((caddr_t)&pi6, sizeof(pi6), IPV6_PKTINFO, IPPROTO_IPV6);
            if(*optp != NULL)
                optp = &(*optp)->m_next;
        }
        if(inp->in6p_flags & IN6P_HOPLIMIT) {
            int hlim = ip6.ip6_hlim;

            *optp = sbcreatecontrol((caddr_t)&hlim, sizeof(int), IPV6_HOPLIMIT, IPPROTO_IPV6);
        }
    }

    m_adj(m, off);
    if(sbappendaddr(&so->so_rcv, (struct sockaddr *)from, m, opts) == 0) {
        m_freem(m);
        if(opts)
            m_freem(opts);
        return;
    }
    sorwakeup(so);
}

/*
 * rip6_input - hand a packet to the raw sockets that want it.
 *
 * pr_input of the raw IPv6 protosw for protocols the stack has no handler
 * for, and called by icmp6_input() with every ICMPv6 message once the
 * kernel is done with it. m starts with the IPv6 header; off is where the
 * nxt header begins. Every socket of protocol nxt (or of protocol 0) whose
 * bound and connected addresses match gets a copy; ICMPv6 sockets only
 * the types their filter passes. Takes m.
 */
void
rip6_input(void *args, ...)
{
    struct mbuf *m = args;
    struct ip6_hdr *ip6;
    struct in6_addr src, dst;   /* the header is packed */
    struct inpcb *inp, *last = NULL;
    struct sockaddr_in6 from;
    int off, nxt;
    u_int8_t type = 0;
    va_list va;

    va_start(va, args);
    off = va_arg(va, int);
    nxt = va_arg(va, int);
    va_end(va);

    if(m->m_len < (int)sizeof(struct ip6_hdr) &&
            (m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL)
        return;
    ip6 = mtod(m, struct ip6_hdr *);
    src = ip6->ip6_src;
    dst = ip6->ip6_dst;

    if(nxt == IPPROTO_ICMPV6) {
        if(m->m_pkthdr.len < off + 1) {
            m_freem(m);
            return;
        }
        m_copydata(m, off, 1, (caddr_t)&type);
    }

    bzero(&from, sizeof(from));
    from.sin6_len    = sizeof(from);
    from.sin6_family = AF_INET6;
    from.sin6_addr   = src;

    for(inp = rip6cb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
        struct rip6pcb *rp = intorip6(inp);

        if(rp == NULL)
            continue;
        if(rp->rp_proto != 0 && rp->rp_proto != nxt)
            continue;
        if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6) &&
                !IN6_ARE_ADDR_EQUAL(&inp->inp_laddr6, &dst))
            continue;
        if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6) &&
                !IN6_ARE_ADDR_EQUAL(&inp->inp_faddr6, &src))
            continue;
        if(nxt == IPPROTO_ICMPV6 && ICMP6_FILTER_WILLBLOCK(type, &rp->rp_filter))
            continue;
        if(last != NULL) {
            struct mbuf *n = m_copym(m, 0, M_COPYALL, M_DONTWAIT);

            if(n != NULL)
                rip6_append(last, n, off, &from);
        }
        last = inp;
    }
    if(last != NULL)
        rip6_append(last, m, off, &from);
    else
        m_freem(m);
}

/* ------------------------------------------------------------------
 * Output.
 * ------------------------------------------------------------------ */

/* What a datagram's ancillary data asks for (RFC 3542 section 6). */
struct rip6_sendopts {
    struct in6_addr src;        /* IPV6_PKTINFO source, or unspecified */
    unsigned int    ifindex;    /* IPV6_PKTINFO interface, or 0 */
    int             hlim;       /* IPV6_HOPLIMIT, or -1 */
};

static int
rip6_ctlparse(struct mbuf *control, struct rip6_sendopts *o)
{
    struct cmsghdr *cm;
    int left;

    bzero(o, sizeof(*o));
    o->hlim = -1;
    if(control == NULL)
        return 0;

    cm = mtod(control, struct cmsghdr *);
    left = control->m_len;
    while(left > 0) {
        if(left < (int)sizeof(*cm) || cm->cmsg_len < sizeof(*cm) ||
                (int)cm->cmsg_len > left)
            return EINVAL;
        if(cm->cmsg_level == IPPROTO_IPV6) {
            switch(cm->cmsg_type) {
            case IPV6_PKTINFO: {
                struct in6_pktinfo pi6;

                if(cm->cmsg_len != CMSG_LEN(sizeof(pi6)))
                    return EINVAL;
                bcopy(CMSG_DATA(cm), (caddr_t)&pi6, sizeof(pi6));
                o->src     = pi6.ipi6_addr;
                o->ifindex = pi6.ipi6_ifindex;
                break;
            }
            case IPV6_HOPLIMIT: {
                int hlim = 0;

                if(cm->cmsg_len != CMSG_LEN(sizeof(int)))
                    return EINVAL;
                bcopy(CMSG_DATA(cm), (caddr_t)&hlim, sizeof(int));
                if(hlim < -1 || hlim > 255)
                    return EINVAL;
                o->hlim = hlim;
                break;
            }
            default:
                return ENOPROTOOPT;
            }
        }
        if(left < (int)_ALIGN(cm->cmsg_len))
            break;
        left -= _ALIGN(cm->cmsg_len);
        cm = (struct cmsghdr *)((caddr_t)cm + _ALIGN(cm->cmsg_len));
    }
    return 0;
}

/*
 * The source for a datagram to dst: the bound address, else one of the
 * interface it leaves by (the one asked for, the multicast interface, or
 * the route's). Unspecified when there is none.
 */
static void
rip6_selectsrc(struct inpcb *inp, struct in6_addr *dst, struct ifnet *oifp,
               struct in6_addr *src)
{
    struct in6_ifaddr *ia = NULL;

    if(!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
        *src = inp->inp_laddr6;
        return;
    }
    if(oifp == NULL && IN6_IS_ADDR_MULTICAST(dst) && inp->in6p_moptions)
        oifp = inp->in6p_moptions->im6o_multicast_ifp;
    if(oifp != NULL)
        ia = in6_ifawithifp(oifp, dst);
    else {
        struct route_in6 ro;

        bzero(&ro, sizeof(ro));
        ro.ro_dst.sin6_family = AF_INET6;
        ro.ro_dst.sin6_len    = sizeof(ro.ro_dst);
        ro.ro_dst.sin6_addr   = *dst;
        rtalloc((struct route *)&ro);
        if(ro.ro_rt != NULL) {
            ia = in6_ifawithifp(ro.ro_rt->rt_ifp, dst);
            RTFREE(ro.ro_rt);
        }
    }
    if(ia != NULL)
        *src = ia->ia_addr.sin6_addr;
    else
        bzero(src, sizeof(*src));
}

/*
 * rip6_send - send m, the data after the IPv6 header, to nam or to the
 * connected peer. The kernel computes the ICMPv6 checksum (RFC 3542
 * section 3.1). The hop limit is IPV6_HOPLIMIT from the ancillary data,
 * else IPV6_MULTICAST_HOPS or IPV6_UNICAST_HOPS, else the default (1 for
 * multicast). Takes m and control.
 */
static int
rip6_send(struct inpcb *inp, struct mbuf *m, struct mbuf *nam,
          struct mbuf *control)
{
    struct rip6pcb *rp = intorip6(inp);
    struct rip6_sendopts o;
    struct in6_addr dst, src;
    unsigned int scope = 0;
    struct ifnet *oifp = NULL;
    struct ip6_moptions oim6o, *im6o = inp->in6p_moptions;
    struct ip6_hdr *ip6;
    int plen, error;

    error = rip6_ctlparse(control, &o);
    if(control)
        m_freem(control);
    if(error)
        goto bad;

    if(nam != NULL) {
        struct sockaddr_in6 *sin6;

        if(inp->inp_socket->so_state & SS_ISCONNECTED) {
            error = EISCONN;
            goto bad;
        }
        if(nam->m_len < (int)sizeof(struct sockaddr_in6)) {
            error = EINVAL;
            goto bad;
        }
        sin6 = mtod(nam, struct sockaddr_in6 *);
        if(sin6->sin6_family != AF_INET6) {
            error = EAFNOSUPPORT;
            goto bad;
        }
        dst   = sin6->sin6_addr;
        scope = sin6->sin6_scope_id;
    } else {
        if(!(inp->inp_socket->so_state & SS_ISCONNECTED)) {
            error = ENOTCONN;
            goto bad;
        }
        dst = inp->inp_faddr6;
    }

    /* the interface asked for: IPV6_PKTINFO, else the destination's zone */
    if(o.ifindex == 0)
        o.ifindex = scope;
    if(o.ifindex != 0 && (oifp = rip6_ifbyindex(o.ifindex)) == NULL) {
        error = ENXIO;
        goto bad;
    }

    plen = m->m_pkthdr.len;
    if(rp->rp_proto == IPPROTO_ICMPV6 && plen < 4) {
        error = EINVAL;     /* no room for the checksum */
        goto bad;
    }

    M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
    if(m == NULL)
        return ENOBUFS;
    ip6 = mtod(m, struct ip6_hdr *);
    ip6->ip6_flow = 0;
    ip6->ip6_vfc  = IPV6_VERSION;	/* must follow ip6_flow (they overlap) */
    ip6->ip6_plen = htons((u_int16_t)plen);
    ip6->ip6_nxt  = (u_int8_t)rp->rp_proto;
    ip6->ip6_dst  = dst;

    if(!IN6_IS_ADDR_UNSPECIFIED(&o.src)) {
        if(!rip6_ownaddr(&o.src)) {
            error = EADDRNOTAVAIL;
            goto bad;
        }
        src = o.src;
    } else
        rip6_selectsrc(inp, &dst, oifp, &src);
    ip6->ip6_src = src;

    if(o.hlim >= 0)
        ip6->ip6_hlim = (u_int8_t)o.hlim;
    else if(IN6_IS_ADDR_MULTICAST(&dst))
        ip6->ip6_hlim = im6o ? im6o->im6o_multicast_hlim
                        : IP6_DEFAULT_MULTICAST_HLIM;
    else
        ip6->ip6_hlim = (inp->in6p_hops >= 0) ? (u_int8_t)inp->in6p_hops
                        : (u_int8_t)ip6_defhlim;

    if(rp->rp_proto == IPPROTO_ICMPV6) {
        u_int16_t sum = 0;
        int coff = sizeof(struct ip6_hdr) + 2;

        if(m->m_len < coff + 2 && (m = m_pullup(m, coff + 2)) == NULL)
            return ENOBUFS;
        bcopy((caddr_t)&sum, mtod(m, caddr_t) + coff, sizeof(sum));
        sum = in6_cksum(m, IPPROTO_ICMPV6, sizeof(struct ip6_hdr), plen);
        bcopy((caddr_t)&sum, mtod(m, caddr_t) + coff, sizeof(sum));
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
    return ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0,
                      im6o, (struct ifnet **)NULL, inp);

bad:
    if(m)
        m_freem(m);
    return error;
}

/* pr_output: rip6_output(m, so, nam, control) */
int
rip6_output(void *args, ...)
{
    struct mbuf *m = args, *nam, *control;
    struct socket *so;
    va_list va;

    va_start(va, args);
    so      = va_arg(va, struct socket *);
    nam     = va_arg(va, struct mbuf *);
    control = va_arg(va, struct mbuf *);
    va_end(va);

    if(sotoinpcb(so) == NULL) {
        if(control)
            m_freem(control);
        m_freem(m);
        return EINVAL;
    }
    return rip6_send(sotoinpcb(so), m, nam, control);
}

/* ------------------------------------------------------------------
 * Socket options: ICMP6_FILTER here, everything else ip6_ctloutput().
 * ------------------------------------------------------------------ */
int
rip6_ctloutput(int op, struct socket *so, int level, int optname,
               struct mbuf **m)
{
    struct inpcb *inp = sotoinpcb(so);
    struct rip6pcb *rp;
    int error = 0;

    if(level != IPPROTO_ICMPV6)
        return ip6_ctloutput(op, so, level, optname, m);

    if(inp == NULL || (rp = intorip6(inp)) == NULL)
        error = EINVAL;
    else if(rp->rp_proto != IPPROTO_ICMPV6 || optname != ICMP6_FILTER)
        error = ENOPROTOOPT;
    else if(op == PRCO_SETOPT) {
        if(*m == NULL || (*m)->m_len != sizeof(struct icmp6_filter))
            error = EINVAL;
        else
            bcopy(mtod(*m, caddr_t), (caddr_t)&rp->rp_filter,
                  sizeof(struct icmp6_filter));
    } else {
        *m = m_get(M_WAIT, MT_SOOPTS);
        if(*m == NULL)
            error = ENOBUFS;
        else {
            (*m)->m_len = sizeof(struct icmp6_filter);
            bcopy((caddr_t)&rp->rp_filter, mtod(*m, caddr_t),
                  sizeof(struct icmp6_filter));
        }
    }
    if(op == PRCO_SETOPT && *m) {
        m_freem(*m);
        *m = NULL;
    }
    return error;
}

/* ------------------------------------------------------------------
 * User requests.
 * ------------------------------------------------------------------ */
static void
rip6_detach(struct inpcb *inp)
{
    if(inp->inp_ppcb) {
        bsd_free(inp->inp_ppcb, M_PCB);
        inp->inp_ppcb = NULL;
    }
    in_pcbdetach(inp);
}

/* nam as an IPv6 socket address, or an error */
static int
rip6_sockaddr(struct mbuf *nam, struct sockaddr_in6 **sin6)
{
    if(nam == NULL || nam->m_len < (int)sizeof(struct sockaddr_in6))
        return EINVAL;
    *sin6 = mtod(nam, struct sockaddr_in6 *);
    if((*sin6)->sin6_family != AF_INET6)
        return EAFNOSUPPORT;
    return 0;
}

int
rip6_usrreq(struct socket *so, int req, struct mbuf *m,
            struct mbuf *nam, struct mbuf *control)
{
    struct inpcb *inp = sotoinpcb(so);
    struct sockaddr_in6 *sin6;
    int error = 0;
    int s;

    if(req == PRU_CONTROL)
        return in6_control(so, (int)(long)m, (caddr_t)nam,
                           (struct ifnet *)control);

    if(inp == NULL && req != PRU_ATTACH) {
        error = EINVAL;
        goto release;
    }

    switch(req) {

    case PRU_ATTACH: {
        struct rip6pcb *rp;

        if(inp != NULL) {
            error = EINVAL;
            break;
        }
        MALLOC(rp, struct rip6pcb *, sizeof(*rp), M_PCB, M_WAITOK);
        if(rp == NULL) {
            error = ENOBUFS;
            break;
        }
        rp->rp_proto = (int)(long)nam;
        ICMP6_FILTER_SETPASSALL(&rp->rp_filter);
        error = soreserve(so, rip6_sendspace, rip6_recvspace);
        if(error == 0) {
            s = splnet();
            error = in_pcballoc(so, &rip6cbinfo);
            splx(s);
        }
        if(error) {
            bsd_free(rp, M_PCB);
            break;
        }
        inp = sotoinpcb(so);
        inp->inp_ppcb = (caddr_t)rp;
        inp->in6p_hops = -1;    /* use the system default hop limit */
        break;
    }

    case PRU_DETACH:
        s = splnet();
        rip6_detach(inp);
        splx(s);
        break;

    case PRU_ABORT:
        soisdisconnected(so);
        s = splnet();
        rip6_detach(inp);
        splx(s);
        break;

    case PRU_BIND:
        if((error = rip6_sockaddr(nam, &sin6)) != 0)
            break;
        if(!IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
                !rip6_ownaddr(&sin6->sin6_addr)) {
            error = EADDRNOTAVAIL;
            break;
        }
        inp->inp_laddr6 = sin6->sin6_addr;
        break;

    case PRU_CONNECT: {
        struct ifnet *oifp = NULL;

        if((error = rip6_sockaddr(nam, &sin6)) != 0)
            break;
        if(sin6->sin6_scope_id != 0 &&
                (oifp = rip6_ifbyindex(sin6->sin6_scope_id)) == NULL) {
            error = ENXIO;
            break;
        }
        inp->inp_faddr6 = sin6->sin6_addr;
        /* fix the source now, so getsockname() reports it */
        rip6_selectsrc(inp, &inp->inp_faddr6, oifp, &inp->inp_laddr6);
        soisconnected(so);
        break;
    }

    case PRU_DISCONNECT:
        if(!(so->so_state & SS_ISCONNECTED)) {
            error = ENOTCONN;
            break;
        }
        bzero(&inp->inp_faddr6, sizeof(inp->inp_faddr6));
        so->so_state &= ~SS_ISCONNECTED;
        break;

    case PRU_SHUTDOWN:
        socantsendmore(so);
        break;

    case PRU_SEND:
        return rip6_send(inp, m, nam, control);

    case PRU_SOCKADDR:
        in6_setsockaddr(inp, nam);
        break;

    case PRU_PEERADDR:
        in6_setpeeraddr(inp, nam);
        break;

    case PRU_SENSE:
        return 0;

    case PRU_RCVD:
    case PRU_RCVOOB:
        return EOPNOTSUPP;

    default:
        error = EOPNOTSUPP;
        break;
    }

release:
    if(control)
        m_freem(control);
    if(m)
        m_freem(m);
    return error;
}

#endif /* INET6 */
