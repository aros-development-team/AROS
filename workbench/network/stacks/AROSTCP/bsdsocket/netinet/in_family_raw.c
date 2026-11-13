/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * What the raw-socket implementation (netinet/raw_ip.c) asks of IPv4: the
 * IPv4 family's implementations of the "raw sockets" operations of struct
 * inetfamily (netinet/inetfamily.h), and IPv4's hand-over of a received
 * packet to the raw sockets. Everything here is what only an IPv4 raw
 * socket has: it is handed the packet with its IP header; IP_HDRINCL, with
 * which the application sends the header too; the IP options, TOS and TTL
 * of the socket in the header made for it (TOS 0 and the largest TTL
 * unless set); a source address left to ip_output(). Who gets a packet,
 * binding, connecting and sending are not here: they are the same for
 * every family.
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
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/inetfamily.h>
#include <protos/kern/kern_random_protos.h>
#include <netinet/ip_output_protos.h>
#include <stdarg.h>
#include <net/netcore_map.h>        /* always the last include */

extern const struct inetfamily in_family;

/*
 * inf_rawattach: a datagram a raw socket has a header made for goes out
 * with TOS 0 and the largest TTL, as it always has, until IP_TOS and
 * IP_TTL say otherwise.
 */
void
in_rawattach(struct inpcb *inp)
{
    inp->inp_ip.ip_tos = 0;
    inp->inp_ip.ip_ttl = MAXTTL;
}

/*
 * inf_rawctloutput: IP_HDRINCL, the one option only a raw socket has. The
 * others (IP_OPTIONS among them) are ip_ctloutput()'s.
 */
int
in_rawctloutput(int op, struct socket *so, int level, int optname, struct mbuf **m)
{
    struct inpcb *inp = sotoinpcb(so);
    int error = 0;

    if(level != IPPROTO_IP || optname != IP_HDRINCL)
        return (-1);

    if(op == PRCO_SETOPT) {
        if(m == NULL || *m == NULL || (*m)->m_len < sizeof(int))
            error = EINVAL;
        else if(*mtod(*m, int *))
            inp->inp_flags |= INP_HDRINCL;
        else
            inp->inp_flags &= ~INP_HDRINCL;
        if(m != NULL && *m != NULL) {
            (void)m_free(*m);
            *m = NULL;
        }
    } else {
        *m = m_get(M_WAIT, MT_SOOPTS);
        if(*m == NULL)          /* allocator may fail here */
            error = ENOBUFS;
        else {
            (*m)->m_len = sizeof(int);
            *mtod(*m, int *) = (inp->inp_flags & INP_HDRINCL) ? 1 : 0;
        }
    }
    return (error);
}

/*
 * inf_rawoutput: generate the IP header and pass the packet to
 * ip_output(), with the options the user may have set up with a control
 * call; or, if the user handed us a complete IP packet (IP_HDRINCL), use
 * it as it is (IP_RAWOUTPUT: ip_output() keeps its header fields - its
 * fragment offset and flags among them - and gives it an identification
 * only when it has none), without the socket's options. Broadcasts are
 * allowed. A socket bound to an interface sends there.
 */
int
in_rawoutput(struct mbuf *m, struct inpcb *inp, const void *laddr, const void *faddr,
    int proto, int cksumoff, struct ifnet *oifp, const int *opts)
{
    int flags = (inp->inp_socket->so_options & SO_DONTROUTE) | IP_ALLOWBROADCAST;
    struct mbuf *ipopts = inp->inp_options;
    struct ip *ip;

    if(inp->inp_flags & INP_HDRINCL) {
        /* a header there must be, where ip_output() reads it */
        if(m->m_pkthdr.len < (int)sizeof(struct ip)) {
            m_freem(m);
            return (EINVAL);
        }
        if(m->m_len < (int)sizeof(struct ip) &&
                (m = m_pullup(m, sizeof(struct ip))) == NULL)
            return (ENOBUFS);
        ip = mtod(m, struct ip *);
        if(ip->ip_id == 0)
            kern_random(&ip->ip_id, sizeof(ip->ip_id));     /* as ip_output() would */
        flags |= IP_RAWOUTPUT;
        ipopts = NULL;
    } else {
        M_PREPEND(m, sizeof(struct ip), M_WAIT);
        if(m == NULL)
            return (ENOBUFS);
        ip = mtod(m, struct ip *);
        ip->ip_tos = inp->inp_ip.ip_tos;
        ip->ip_off = (inp->inp_flags & INP_DONTFRAG) ? IP_DF : 0;
        ip->ip_p = proto;
        ip->ip_len = m->m_pkthdr.len;
        /* "any": ip_output() takes the address of the interface it picks */
        ip->ip_src = *(const struct in_addr *)laddr;
        ip->ip_dst = *(const struct in_addr *)faddr;
        ip->ip_ttl = inp->inp_ip.ip_ttl;
    }
    if(oifp != NULL) {
        /* SO_BINDTODEVICE: ip_output() takes the interface from the header */
        m->m_pkthdr.rcvif = oifp;
        flags |= IP_BOUNDIF;
    }
    return (ip_output(m, ipopts, &inp->inp_route, flags, inp->inp_moptions));
}
