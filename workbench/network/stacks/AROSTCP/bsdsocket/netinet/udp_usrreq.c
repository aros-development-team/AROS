/*
 * Copyright (C) 1982, 1986, 1988, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2006
 *      Pavel Fedin
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
 *	@(#)udp_usrreq.c	8.4 (Berkeley) 1/21/94
 * $Id$
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/synch.h>
#include <stddef.h>      /* offsetof */

#include <net/route.h>
#include <net/if.h>
#include <net/netmodule.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>         /* struct inpcb holds one; not used here */
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <kern/kern_subr_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 *
 * One implementation for every address family. What a family alone knows
 * - its header, its pseudo-header, its socket address, which destinations
 * are shared by several sockets, its ancillary data, its source address
 * selection, its ICMP - is asked of the family of the packet or of the PCB
 * (netinet/inetfamily.h). Nothing here tests which family that is.
 */
#ifndef	COMPAT_42
#define	UDPCKSUM_DFLT	1
#else
#define	UDPCKSUM_DFLT	0		/* XXX */
#endif
int	udpcksum = UDPCKSUM_DFLT;

struct	inpcbhead udb;		/* from udp_var.h */
struct	inpcbinfo udbinfo;

#ifndef UDBHASHSIZE
#define UDBHASHSIZE 64
#endif

struct	udpstat udpstat;	/* from udp_var.h: datagrams of every family */

static	void udp_detach __P((struct inpcb *));
static	void udp_notify __P((struct inpcb *, int));
static	int udp_output __P((struct inpcb *, struct mbuf *, struct mbuf *,
	    struct mbuf *));

#define	UDP_SENDSPACE_DFLT	9216
#define	UDP_RECVSPACE_DFLT	(256 * (1024 + 16))
u_long	udp_sendspace = UDP_SENDSPACE_DFLT;	/* really max datagram size */
u_long	udp_recvspace = UDP_RECVSPACE_DFLT;
/* 256 1K datagrams, each with a 16 byte socket address: a deeper socket
 * receive buffer lets the UDP receiver absorb bursts (e.g. sockperf
 * under-load) while the owning task is briefly off-CPU, instead of
 * overflowing at ~40 datagrams and dropping the flood. */

/*
 * UDP's variables and statistics, for whoever reaches them from outside
 * UDP: the configuration and ARexx variables (UDP), programs that look the
 * stack's symbols up (netstat: "_udb", "_udpstat") and sysctl
 * (net.inet.udp.<id>). None of them names UDP: the table is registered
 * when UDP starts and each looks its name up (net/netmodule.h). The names
 * under net.inet.udp are UDP's, whichever families the stack runs.
 */
static const struct netvar udp_netvars[] = {
    { "udpstat",  &udpstat,  sizeof(udpstat), NVT_STATS, PF_INET, IPPROTO_UDP, UDPCTL_STATS },
    /* the list of UDP's PCBs, which netstat walks: a block read from
     * outside, as the statistics are */
    { "udb",      &udb,      sizeof(udb),     NVT_STATS },
    { "udpcksum", &udpcksum, sizeof(int),     NVT_INT,   PF_INET, IPPROTO_UDP, UDPCTL_CHECKSUM },
    /* (the two buffer sizes are u_long; what is read and set is an int's
     * worth of them, as it always was) */
    { "udp_sendspace", &udp_sendspace, sizeof(int), NVT_INT, PF_INET, IPPROTO_UDP, UDPCTL_MAXDGRAM },
    { "udp_recvspace", &udp_recvspace, sizeof(int), NVT_INT, PF_INET, IPPROTO_UDP, UDPCTL_RECVSPACE },
};

/*
 * The UDP header of a datagram in an mbuf, behind a network header that
 * starts at an even address and is a multiple of 4 bytes long, or where
 * udp_output() puts it: at an even address. struct udphdr is packed, so
 * through a plain pointer to it each field is read and written a byte at a
 * time on machines that cannot access a word at an odd address (the
 * 68000).
 */
typedef struct udphdr udp_ahdr_t __attribute__((__aligned__(2)));

/*
 * A datagram on its way to the sockets: what udp_input() found out about
 * it once, for each socket it is given to.
 */
struct udp_rcv {
    const struct inetfamily *ur_family;
    const struct ifnet *ur_rcvif;
    union inp_addr ur_src, ur_dst;      /* copies: inf_addrlen bytes are set */
    u_short     ur_sport;               /* network byte order */
    int         ur_tos, ur_hlim;
    struct sockaddr_storage ur_from;    /* the sender, in the family's form */
};

void
udp_init()
{
    LIST_INIT(&udb);
    udbinfo.listhead = &udb;
    udbinfo.hashbase = phashinit(UDBHASHSIZE, M_PCB, &udbinfo.hashsize);
    (void)netvar_register(udp_netvars, sizeof(udp_netvars) / sizeof(udp_netvars[0]));
}

/*
 * UDP stops: the reverse of udp_init(), called by the transport module's
 * stop (netinet/inet_transport.c) with no socket or PCB left. The
 * variables are taken back, the PCB hash table goes back to the pool, and
 * UDP's variables are what the file gives them.
 */
void
udp_deinit()
{
    (void)netvar_unregister(udp_netvars);
    if(udbinfo.hashbase != NULL)
        bsd_free((caddr_t)udbinfo.hashbase, M_PCB);
    bzero((caddr_t)&udbinfo, sizeof(udbinfo));
    bzero((caddr_t)&udb, sizeof(udb));
    bzero((caddr_t)&udpstat, sizeof(udpstat));
    udpcksum = UDPCKSUM_DFLT;
    udp_sendspace = UDP_SENDSPACE_DFLT;
    udp_recvspace = UDP_RECVSPACE_DFLT;
}

/*
 * udp_append - hand one datagram to one socket. m is its data, the headers
 * already taken off; with it go the sender's address and the ancillary
 * data the socket asked for: the receive time (SO_TIMESTAMP) first, then
 * what its family makes of the packet. m is consumed.
 */
static void
udp_append(struct inpcb *inp, struct mbuf *m, struct udp_rcv *ur)
{
    const struct inetfamily *inf = ur->ur_family;
    struct socket *so = inp->inp_socket;
    struct sockaddr *from = (struct sockaddr *)&ur->ur_from;
    struct sockaddr_storage mapped;
    struct mbuf *opts, **mp = &opts;

    opts = sbcreatetimestamp(so);
    if(opts)
        mp = &opts->m_next;
    if(inp->inp_family == inf) {
        if(INF_WANTSCONTROL(inp, inf))
            (void)(*inf->inf_savecontrol)(inp, ur->ur_rcvif, &ur->ur_dst,
                                          ur->ur_tos, ur->ur_hlim, mp);
    } else {
        /*
         * A socket of another family that listens for this one too: it is
         * told the sender in its own family's form, as the PCB layer names
         * the peer of such a connection. (A family that has no form for
         * the address leaves it as it came.)
         */
        const struct inetfamily *sof = inp->inp_family;
        union inp_addr a;

        bzero((caddr_t)&a, sizeof(a));
        if((*sof->inf_addr_from)(&a, inf, &ur->ur_src) == 0) {
            (*sof->inf_mksockaddr)((struct sockaddr *)&mapped, &a, ur->ur_sport,
                                   (const struct ifnet *)NULL);
            from = (struct sockaddr *)&mapped;
        }
    }
    if(sbappendaddr(&so->so_rcv, from, m, opts) == 0) {
        m_freem(m);
        if(opts)
            m_freem(opts);
        udpstat.udps_fullsock++;
    } else
        sorwakeup(so);
}

/*
 * udp_input - a datagram of family inf has arrived: m starts at the
 * family's network header, the UDP header is at off. Called by each
 * family's input with its own instance of the family interface.
 */
void
udp_input(struct mbuf *m, int off, const struct inetfamily *inf)
{
    register udp_ahdr_t *uh;
    register struct inpcb *inp;
    struct udp_rcv ur;
    const void *srcp, *dstp;
    u_int alen = inf->inf_addrlen;
    u_short dport;
    int len, avail, hdrs;

    udpstat.udps_ipackets++;

    /*
     * Get network and UDP header together in first mbuf.
     */
    if(m->m_len < off + (int)sizeof(struct udphdr)) {
        if((m = m_pullup(m, off + sizeof(struct udphdr))) == 0) {
            udpstat.udps_hdrops++;
            return;
        }
    }

    /*
     * What the network header says: who it is from and to, its TOS and
     * TTL for the sockets that want them, and how much there is from the
     * UDP header on. The addresses are copied: the family may use its
     * header's room for the checksum.
     */
    if((*inf->inf_rcvinfo)(m, off, &srcp, &dstp, &ur.ur_tos, &ur.ur_hlim, &avail) != 0) {
        udpstat.udps_hdrops++;
        goto bad;
    }
    /* a word at a time: a network header is even-aligned in its mbuf */
    inpcb_addrcopy(&ur.ur_src, srcp, alen);
    inpcb_addrcopy(&ur.ur_dst, dstp, alen);
    uh = (udp_ahdr_t *)(mtod(m, caddr_t) + off);

    /*
     * Make mbuf data length reflect UDP length.
     * If not enough data to reflect UDP length, drop.
     */
    len = ntohs((u_short)uh->uh_ulen);
    if(len > avail || len < (int)sizeof(struct udphdr)) {
        udpstat.udps_badlen++;
        goto bad;
    }
    if(len != avail)
        m_adj(m, len - avail);

    /*
     * Checksum extended UDP header and data. A checksum of zero is "none
     * was made": taken as it is where the family allows that, a bad
     * datagram where the checksum is required.
     */
    if(uh->uh_sum == 0) {
        if(inf->inf_flags & INF_CKSUM_REQUIRED) {
            udpstat.udps_badsum++;
            goto bad;
        }
    } else if((udpcksum || (inf->inf_flags & INF_CKSUM_REQUIRED)) &&
              (*inf->inf_cksum)(m, IPPROTO_UDP, off, len, &ur.ur_src, &ur.ur_dst) != 0) {
        udpstat.udps_badsum++;
        goto bad;
    }

    /*
     * The sender as a socket address of the family, and whether the
     * destination is one that several sockets share.
     */
    ur.ur_family = inf;
    ur.ur_rcvif = m->m_pkthdr.rcvif;
    ur.ur_sport = uh->uh_sport;
    dport = uh->uh_dport;
    hdrs = off + sizeof(struct udphdr);

    if((*inf->inf_rcvfrom)(m, &ur.ur_src, ur.ur_sport, &ur.ur_dst,
                           (struct sockaddr *)&ur.ur_from)) {
        struct inpcb *last = NULL;
        /*
         * Deliver a multicast or broadcast datagram to *all* sockets
         * for which the local and remote addresses and ports match
         * those of the incoming datagram.  This allows more than
         * one process to receive multi/broadcasts on the same port.
         * (This really ought to be done for unicast datagrams as
         * well, but that would cause problems with existing
         * applications that open both address-specific sockets and
         * a wildcard socket listening to the same port -- they would
         * end up receiving duplicates of every unicast datagram.
         * Those applications open the multiple sockets to overcome an
         * inadequacy of the UDP socket interface, but for backwards
         * compatibility we avoid the problem here rather than
         * fixing the interface.  Maybe 4.5BSD will remedy this?)
         *
         * The family's input has already checked that the host listens
         * to the address on the receiving interface; the sockets are not
         * filtered by their own memberships.
         */
        m->m_len -= hdrs;
        m->m_pkthdr.len -= hdrs;
        m->m_data += hdrs;
        /*
         * Locate pcb(s) for datagram.
         * (Algorithm copied from raw_intr().)
         */
        for(inp = udb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
            if(inp->inp_lport != dport)
                continue;
            /* a socket of the family, or one that stands for every family */
            if(inp->inp_family != inf && !inpcb_anyfamily(inp))
                continue;
            /* a socket bound to an interface only hears that interface,
             * and nothing once the interface is gone */
            if((inp->inp_boundif && inp->inp_boundif != ur.ur_rcvif) ||
                    (inp->inp_flags & INP_BOUNDGONE))
                continue;
            if(!inpcb_addrany(&inp->inp_la, alen) &&
                    !inpcb_addreq(&inp->inp_la, &ur.ur_dst, alen))
                continue;
            if(!inpcb_addrany(&inp->inp_fa, alen) &&
                    (!inpcb_addreq(&inp->inp_fa, &ur.ur_src, alen) ||
                     inp->inp_fport != ur.ur_sport))
                continue;

            if(last != NULL) {
                struct mbuf *n;

                if((n = m_copy(m, 0, M_COPYALL)) != NULL)
                    udp_append(last, n, &ur);
            }
            last = inp;
            /*
             * Don't look for additional matches if this one does
             * not have either the SO_REUSEPORT or SO_REUSEADDR
             * socket options set.  This heuristic avoids searching
             * through all pcbs in the common case of a non-shared
             * port.  It * assumes that an application will never
             * clear these options after setting them.
             */
            if((last->inp_socket->so_options & (SO_REUSEPORT | SO_REUSEADDR)) == 0)
                break;
        }

        if(last == NULL) {
            /*
             * No matching pcb found; discard datagram.
             * (No need to tell the sender about a broadcast
             * or multicast datgram.)
             */
            udpstat.udps_noportbcast++;
            goto bad;
        }
        udp_append(last, m, &ur);
        return;
    }

    /*
     * Locate pcb for datagram: the exact match, else the best of those
     * that listen on the port, among the sockets that hear the interface
     * it came in by (SO_BINDTODEVICE, a zone).
     */
    inp = inpcb_lookup(&udbinfo, inf, &ur.ur_src, ur.ur_sport, &ur.ur_dst, dport,
                       ur.ur_rcvif, INPLOOKUP_WILDCARD | INPLOOKUP_ATSPLNET);
    if(inp == NULL) {
        udpstat.udps_noport++;
        if(m->m_flags & (M_BCAST | M_MCAST)) {
            udpstat.udps_noportbcast++;
            goto bad;
        }
        /* "port unreachable", as the family says it; it takes the packet */
        (*inf->inf_unreach)(m, off);
        return;
    }

    /*
     * Stuff source address and datagram in user buffer.
     */
    m->m_len -= hdrs;
    m->m_pkthdr.len -= hdrs;
    m->m_data += hdrs;
    udp_append(inp, m, &ur);
    return;
bad:
    m_freem(m);
}

/*
 * Notify a udp user of an asynchronous error;
 * just wake up so that he can collect error status.
 */
static void
udp_notify(inp, _errno)
register struct inpcb *inp;
int _errno;
{
    inp->inp_socket->so_error = _errno;
    sorwakeup(inp->inp_socket);
    sowwakeup(inp->inp_socket);
}

/*
 * udp_ctlnotify - what a family's ICMP says about a datagram we sent
 * (struct inctlparam, netinet/inetfamily.h).
 *
 * "Too big" (PRC_MSGSIZE) is not an error for a datagram socket: the
 * route keeps the path MTU and the family's output fragments to it, or
 * says EMSGSIZE to a socket that forbids fragmenting. Where the family
 * has a notification for it (inf_pmtunotify), each socket that may have
 * sent the datagram is offered the new path MTU; the family gives it to
 * those that asked.
 */
void
udp_ctlnotify(int cmd, const struct inctlparam *cp)
{
    const struct inetfamily *inf = cp->icp_family;

    if((unsigned)cmd >= PRC_NCMDS)
        return;
    if(cmd == PRC_MSGSIZE) {
        u_int alen = inf->inf_addrlen;
        struct inpcb *inp;

        if(inf->inf_pmtunotify == NULL || cp->icp_laddr == NULL ||
                cp->icp_dst == NULL)
            return;
        for(inp = udb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
            if(inp->inp_family != inf || inp->inp_lport != cp->icp_lport)
                continue;
            if(!inpcb_addrany(&inp->inp_la, alen) &&
                    !inpcb_addreq(&inp->inp_la, cp->icp_laddr, alen))
                continue;
            if(!inpcb_addrany(&inp->inp_fa, alen) &&
                    (!inpcb_addreq(&inp->inp_fa, cp->icp_faddr, alen) ||
                     inp->inp_fport != cp->icp_fport))
                continue;
            (*inf->inf_pmtunotify)(inp, cp->icp_dst, cp->icp_mtu);
        }
        return;
    }
    /* only what is an error for the socket, or a redirect (its route) */
    if(!PRC_IS_REDIRECT(cmd) && inf->inf_ctlerrmap[cmd] == 0)
        return;
    inpcb_notify(&udb, inf, cp->icp_faddr, cp->icp_fport, cp->icp_laddr,
                 cp->icp_lport, cmd, udp_notify);
}

/*
 * udp_output - send the datagram m from inp's socket: to the address in
 * addr, or with addr NULL to the peer the socket is connected to.
 *
 * A datagram sent with an address leaves the PCB as it is: the
 * destination, and the source address if the socket is bound to none, are
 * this datagram's only, so the socket goes on receiving on every address
 * it listens on. Only a port it lacks is bound for good.
 */
static int
udp_output(struct inpcb *inp, struct mbuf *m, struct mbuf *addr,
           struct mbuf *control)
{
    const struct inetfamily *inf = inp->inp_family;
    register udp_ahdr_t *uh;
    register int len = m->m_pkthdr.len;
    const void *faddr = NULL;           /* NULL: the PCB's peer */
    struct ifnet *zone = NULL;
    u_short fport;
    int error, s;

    if(inp->inp_flags & INP_BOUNDGONE) {
        error = ENXIO;                  /* its interface went away */
        goto release;
    }
    if(addr) {
        if(!inpcb_addrany(&inp->inp_fa, inf->inf_addrlen)) {
            error = EISCONN;
            goto release;
        }
        if((error = (*inf->inf_sockaddr_parse)(addr, &faddr, &fport, &zone)) != 0)
            goto release;
        if(fport == 0) {
            error = EADDRNOTAVAIL;
            goto release;
        }
        if(inp->inp_lport == 0) {
            s = splnet();
            error = inpcb_bind(inp, (struct mbuf *)0);
            splx(s);
            if(error)
                goto release;
        }
    } else {
        if(inpcb_addrany(&inp->inp_fa, inf->inf_addrlen)) {
            error = ENOTCONN;
            goto release;
        }
        fport = inp->inp_fport;
    }

    /*
     * Get room for the UDP header and, in the same mbuf, for the family's
     * network header in front of it, which is left to the family.
     */
    M_PREPEND(m, sizeof(struct udphdr) + inf->inf_hdrlen, M_DONTWAIT);
    if(m == 0) {
        error = ENOBUFS;
        goto release;
    }
    m->m_data += inf->inf_hdrlen;
    m->m_len -= inf->inf_hdrlen;
    m->m_pkthdr.len -= inf->inf_hdrlen;

    /*
     * Fill in the UDP header, ports and length put into network format.
     */
    uh = mtod(m, udp_ahdr_t *);
    uh->uh_sport = inp->inp_lport;
    uh->uh_dport = fport;
    uh->uh_ulen = htons((u_short)(len + sizeof(struct udphdr)));
    uh->uh_sum = 0;

    /*
     * The family makes the checksum over its pseudo-header, puts its
     * header in front and sends. Where the checksum is optional it is left
     * out (zero) when UDP checksums are turned off.
     */
    udpstat.udps_opackets++;
    return ((*inf->inf_dgoutput)(m, inp, faddr, zone, control, IPPROTO_UDP,
                                 (udpcksum || (inf->inf_flags & INF_CKSUM_REQUIRED)) ?
                                 (int)offsetof(struct udphdr, uh_sum) : -1,
                                 INF_DG_ROOM | INF_DG_NOTZERO));

release:
    if(control)
        m_freem(control);
    if(m)
        m_freem(m);
    return (error);
}

/*ARGSUSED*/
int
udp_usrreq(so, req, m, addr, control)
struct socket *so;
int req;
struct mbuf *m, *addr, *control;
{
    struct inpcb *inp = sotoinpcb(so);
    int error = 0;
    int s;

    if(req == PRU_CONTROL) {
        /* an ioctl: the family of the socket knows its address requests */
        const struct inetfamily *inf = inp ? inp->inp_family :
            inetfamily_find(so->so_proto->pr_domain->dom_family);

        if(inf == NULL || inf->inf_control == NULL)
            return (EOPNOTSUPP);
        return ((*inf->inf_control)(so, (int)(long)m, (caddr_t)addr,
                                    (struct ifnet *)control));
    }
    if(inp == NULL && req != PRU_ATTACH) {
        error = EINVAL;
        goto release;
    }
    /*
     * Note: need to block udp_input while changing
     * the udp pcb queue and/or pcb addresses.
     */
    switch(req) {

    case PRU_ATTACH:
        if(inp != NULL) {
            error = EINVAL;
            break;
        }
        s = splnet();
        /* the PCB starts with its family's defaults (inf_pcbinit) */
        error = in_pcballoc(so, &udbinfo);
        splx(s);
        if(error)
            break;
        error = soreserve(so, udp_sendspace, udp_recvspace);
        break;

    case PRU_DETACH:
        udp_detach(inp);
        break;

    case PRU_BIND:
        s = splnet();
        error = inpcb_bind(inp, addr);
        splx(s);
        break;

    case PRU_LISTEN:
        error = EOPNOTSUPP;
        break;

    case PRU_CONNECT:
        if(!inpcb_addrany(&inp->inp_fa, inp->inp_family->inf_addrlen)) {
            error = EISCONN;
            break;
        }
        s = splnet();
        error = inpcb_connect(inp, addr, NULL);
        splx(s);
        if(error == 0)
            soisconnected(so);
        break;

    case PRU_CONNECT2:
        error = EOPNOTSUPP;
        break;

    case PRU_ACCEPT:
        error = EOPNOTSUPP;
        break;

    case PRU_DISCONNECT:
        if(inpcb_addrany(&inp->inp_fa, inp->inp_family->inf_addrlen)) {
            error = ENOTCONN;
            break;
        }
        s = splnet();
        /*
         * The local address goes with the peer: it may have been chosen
         * for that peer, and the socket listens on every address again.
         * (It is not part of what the PCB is filed under.)
         */
        bzero((caddr_t)&inp->inp_la, sizeof(inp->inp_la));
        inpcb_disconnect(inp);
        splx(s);
        so->so_state &= ~SS_ISCONNECTED;		/* XXX */
        break;

    case PRU_SHUTDOWN:
        socantsendmore(so);
        break;

    case PRU_SEND:
        return (udp_output(inp, m, addr, control));

    case PRU_ABORT:
        soisdisconnected(so);
        udp_detach(inp);
        break;

    case PRU_SOCKADDR:
        inpcb_setsockaddr(inp, addr);
        break;

    case PRU_PEERADDR:
        inpcb_setpeeraddr(inp, addr);
        break;

    case PRU_SENSE:
        /*
         * stat: don't bother with a blocksize.
         */
        return (0);

    case PRU_SENDOOB:
    case PRU_FASTTIMO:
    case PRU_SLOWTIMO:
    case PRU_PROTORCV:
    case PRU_PROTOSEND:
        error =  EOPNOTSUPP;
        break;

    case PRU_RCVD:
    case PRU_RCVOOB:
        return (EOPNOTSUPP);	/* do not free mbuf's */

    default:
        panic("udp_usrreq");
    }

release:
    if(control)
        m_freem(control);
    if(m)
        m_freem(m);
    return (error);
}

static void
udp_detach(inp)
struct inpcb *inp;
{
    int s = splnet();

    in_pcbdetach(inp);
    splx(s);
}
