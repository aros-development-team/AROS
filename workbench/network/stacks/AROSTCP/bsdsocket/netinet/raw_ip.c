/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 Pavel Fedin
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * Copyright (C) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
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
 *	@(#)raw_ip.c	7.8 (Berkeley) 7/25/90
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/errno.h>
#include <sys/synch.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>             /* struct inpcb has one inside */
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>
#include <netinet/inet_transport.h>

#include <netinet/raw_ip_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

/*
 * Raw sockets, for every family: one implementation.
 *
 * A raw socket is a PCB of its family on the list below, with a struct
 * rawpcb behind it (its protocol, and the offset of a checksum the stack
 * is to make and verify). Nothing here knows a family: where the families
 * differ - whether the socket is handed the network header, a filter on
 * what it receives, ancillary data, the socket options only a raw socket
 * has, what a control message may ask for, and how the header of an
 * outgoing packet is made, header-included mode among it - the family is
 * asked (netinet/inetfamily.h, "raw sockets").
 *
 * The PCBs of all families are on one list: each packet is matched
 * against those of its own family.
 */

/* one chain: a raw PCB has no ports, so there is nothing to hash by */
static struct inpcbhead ripcb;
static struct inpcbhead riphash[1];
static struct inpcbinfo ripcbinfo = {
    .listhead = &ripcb, .hashbase = riphash, .hashsize = 1
};

static u_long rip_sendspace = 8192;
static u_long rip_recvspace = 8192;

/* ------------------------------------------------------------------
 * Input.
 * ------------------------------------------------------------------ */

/*
 * A packet on its way to the raw sockets: what rip_deliver() found out
 * about it once, for each socket it is given to.
 */
struct rip_rcv {
    const struct ifnet *rr_rcvif;
    union inp_addr rr_src, rr_dst;      /* copies: inf_addrlen bytes are set */
    int         rr_tos, rr_hlim;
    struct sockaddr_storage rr_from;    /* the sender, in the family's form */
};

/*
 * Queue m on inp's socket, as its family presents a packet to a raw
 * socket: with the network header or from off on, and with the ancillary
 * data the socket asked its family for (as a datagram socket is given it).
 * Returns 1 if the socket took it. Takes m.
 */
static int
rip_append(struct inpcb *inp, struct mbuf *m, int off, struct rip_rcv *rr)
{
    const struct inetfamily *inf = inp->inp_family;
    struct socket *so = inp->inp_socket;
    struct sockaddr *from = (struct sockaddr *)&rr->rr_from;
    struct mbuf *opts = NULL;

    if(INF_WANTSCONTROL(inp, inf))
        (void)(*inf->inf_savecontrol)(inp, rr->rr_rcvif, &rr->rr_dst,
                                      rr->rr_tos, rr->rr_hlim, &opts);
    if(!(inf->inf_rawflags & INFR_HDR))
        m_adj(m, off);
    if(sbappendaddr(&so->so_rcv, from, m, opts) == 0) {
        /* should notify about lost packet */
        m_freem(m);
        if(opts)
            m_freem(opts);
        return (0);
    }
    sorwakeup(so);
    return (1);
}

/*
 * rip_deliver - hand a packet of family inf to the raw sockets that want
 * it. Called by the family's input for protocols the stack has no handler
 * for, and by a protocol of the family (ICMP) with what it has seen. m
 * starts at the network header; off is where the header of protocol proto
 * begins. Every socket of the family and of that protocol (or of protocol
 * 0) whose bound and connected addresses match, that is not bound to
 * another interface, that does not filter the packet out and - if it has
 * the stack verify a checksum - for which the checksum is good, gets a
 * copy. flags: RIP_CKSUMOK. Returns the number of sockets that took it.
 * Takes m.
 */
int
rip_deliver(struct mbuf *m, int off, int proto, const struct inetfamily *inf,
            int flags)
{
    struct inpcb *inp, *last = NULL;
    struct rip_rcv rr;
    const struct ifnet *rcvif = m->m_pkthdr.rcvif;
    const void *sp, *dp;
    u_int alen = inf->inf_addrlen;
    int ulen;
    int csum = -1;              /* checksum verified: -1 not yet, 1 good, 0 bad */
    int sockets = 0;

    /* nobody listens, most of the time: nothing is looked at then */
    if(ripcb.lh_first == NULL ||
            (*inf->inf_rcvinfo)(m, off, &sp, &dp, &rr.rr_tos, &rr.rr_hlim, &ulen) != 0) {
        m_freem(m);
        return (0);
    }
    /* the addresses are in the packet, which is about to be handed on */
    bcopy(sp, (caddr_t)&rr.rr_src, alen);
    bcopy(dp, (caddr_t)&rr.rr_dst, alen);
    rr.rr_rcvif = rcvif;
    rr.rr_from.ss_len = 0;            /* not made yet */

    for(inp = ripcb.lh_first; inp != NULL; inp = inp->inp_list.le_next) {
        struct rawpcb *rp = intorawpcb(inp);

        if(inp->inp_family != inf || rp == NULL)
            continue;
        if(rp->rp_proto != 0 && rp->rp_proto != proto)
            continue;
        /* bound to an interface that is gone, or to another one */
        if((inp->inp_flags & INP_BOUNDGONE) ||
                (rcvif != NULL && inp->inp_boundif != NULL && inp->inp_boundif != rcvif))
            continue;
        if(!inpcb_addrany(&inp->inp_la, alen) && !inpcb_addreq(&inp->inp_la, &rr.rr_dst, alen))
            continue;
        if(!inpcb_addrany(&inp->inp_fa, alen) && !inpcb_addreq(&inp->inp_fa, &rr.rr_src, alen))
            continue;
        if((rp->rp_flags & RPF_FILTER) && (*inf->inf_rawfilter)(inp, m, off, proto))
            continue;
        if(rp->rp_cksum >= 0 && !(flags & RIP_CKSUMOK)) {
            /* one with a wrong checksum is not for it */
            if(m->m_pkthdr.len - off < rp->rp_cksum + 2)
                continue;
            if(csum < 0)
                csum = (*inf->inf_cksum)(m, proto, off, m->m_pkthdr.len - off,
                                         &rr.rr_src, &rr.rr_dst) == 0;
            if(!csum)
                continue;
        }
        if(rr.rr_from.ss_len == 0)
            (*inf->inf_mksockaddr)((struct sockaddr *)&rr.rr_from, &rr.rr_src, 0, rcvif);
        if(last != NULL) {
            struct mbuf *n = m_copym(m, 0, M_COPYALL, M_DONTWAIT);

            if(n != NULL)
                sockets += rip_append(last, n, off, &rr);
        }
        last = inp;
    }
    if(last != NULL)
        sockets += rip_append(last, m, off, &rr);
    else
        m_freem(m);
    return (sockets);
}

/* ------------------------------------------------------------------
 * Output.
 * ------------------------------------------------------------------ */

/*
 * rip_send - send m to nam, or to the connected peer. What a control
 * message may ask for is the family's to say; the source is the one the
 * control message names (it must be ours), else the bound address, else
 * the one the family selects - or leaves to its output (INFR_SRCLATE); if
 * none can be selected the packet goes to the family's output without
 * one, which sends it so or refuses. The interface is the one the control
 * message names, else the zone of the destination, else the one the
 * socket is bound to. In header-included mode (INP_HDRINCL, which a
 * family's own socket option sets) the data is the whole packet and
 * nothing is selected. Takes m and control.
 */
static int
rip_send(struct inpcb *inp, struct mbuf *m, struct mbuf *nam, struct mbuf *control)
{
    const struct inetfamily *inf = inp->inp_family;
    struct rawpcb *rp = intorawpcb(inp);
    struct socket *so = inp->inp_socket;
    union inp_addr src, dst, osrc;
    struct ifnet *oifp = NULL, *zone = NULL;
    int opts[INF_RAWOPTS], *optp = NULL;
    u_int alen = inf->inf_addrlen;
    int cksumoff = -1;
    int error = 0;

    bzero((caddr_t)&src, sizeof(src));
    bzero((caddr_t)&osrc, sizeof(osrc));
    bzero((caddr_t)&dst, sizeof(dst));
    if(control != NULL) {
        if(inf->inf_rawsendopts == NULL)
            error = control->m_len ? EOPNOTSUPP : 0;
        else {
            bzero((caddr_t)opts, sizeof(opts));
            error = (*inf->inf_rawsendopts)(inp, control, &osrc, &oifp, opts);
            optp = opts;
        }
        m_freem(control);
        if(error)
            goto bad;
    }
    if(inp->inp_flags & INP_BOUNDGONE) {
        error = ENXIO;                  /* its interface went away */
        goto bad;
    }

    if(nam != NULL) {
        const void *addr;
        u_short port;

        if(so->so_state & SS_ISCONNECTED) {
            error = EISCONN;
            goto bad;
        }
        if((error = (*inf->inf_sockaddr_parse)(nam, &addr, &port, &zone)) != 0)
            goto bad;
        bcopy(addr, (caddr_t)&dst, alen);
    } else {
        if(!(so->so_state & SS_ISCONNECTED)) {
            error = ENOTCONN;
            goto bad;
        }
        dst = inp->inp_fa;
    }

    if(oifp == NULL)
        oifp = zone;
    if(oifp == NULL)
        oifp = inp->inp_boundif;

    if(!(inp->inp_flags & INP_HDRINCL)) {
        if(!inpcb_addrany(&osrc, alen)) {
            if(!(*inf->inf_ownaddr)(&osrc, NULL)) {
                error = EADDRNOTAVAIL;
                goto bad;
            }
            src = osrc;
        } else if(!inpcb_addrany(&inp->inp_la, alen))
            src = inp->inp_la;
        else if((inf->inf_rawflags & INFR_SRCLATE) ||
                (*inf->inf_selectsrc)(inp, &dst, oifp, &src) != 0)
            bzero((caddr_t)&src, sizeof(src));

        cksumoff = rp->rp_cksum;
        if(cksumoff >= 0 && m->m_pkthdr.len < cksumoff + 2) {
            error = EINVAL;             /* no room for the checksum */
            goto bad;
        }
    }
    return ((*inf->inf_rawoutput)(m, inp, &src, &dst, rp->rp_proto, cksumoff,
                                  oifp, optp));

bad:
    if(m)
        m_freem(m);
    return (error);
}

/*
 * pr_output of the raw protocols: rip_output(m, so, nam, control). (The
 * socket layer sends through rip_usrreq(); nothing calls this today.)
 */
int
rip_output(void *arg, ...)
{
    struct mbuf *m = arg, *nam, *control;
    struct socket *so;
    va_list va;

    va_start(va, arg);
    so      = va_arg(va, struct socket *);
    nam     = va_arg(va, struct mbuf *);
    control = va_arg(va, struct mbuf *);
    va_end(va);

    if(sotoinpcb(so) == NULL || intorawpcb(sotoinpcb(so)) == NULL) {
        if(control)
            m_freem(control);
        m_freem(m);
        return (EINVAL);
    }
    return (rip_send(sotoinpcb(so), m, nam, control));
}

/* Interface ifp is going away: no raw socket may go on naming it. */
void
rip_ifgone(struct ifnet *ifp)
{
    in_pcbifgone(&ripcb, ifp);
}

/* The raw sockets that still use family inf (in_pcbcount(), netinet/in_pcb.c). */
long
rip_pcbcount(const struct inetfamily *inf, int (*holds)(const struct inpcb *inp))
{
    return (in_pcbcount(&ripcb, inf, holds));
}

/*
 * The transports stop, with no raw socket left: the list is as the file
 * has it. (The raw sockets allocate nothing of their own; the one hash
 * chain is static.)
 */
void
rip_deinit(void)
{
    bzero((caddr_t)&ripcb, sizeof(ripcb));
    bzero((caddr_t)riphash, sizeof(riphash));
    ripcbinfo.lastport = 0;
    rip_sendspace = 8192;
    rip_recvspace = 8192;
}

/* ------------------------------------------------------------------
 * Socket options: those only a raw socket of the family has, then the
 * family's own.
 * ------------------------------------------------------------------ */
int
rip_ctloutput(op, so, level, optname, m)
int op;
struct socket *so;
int level, optname;
struct mbuf **m;
{
    struct inpcb *inp = sotoinpcb(so);
    const struct inetfamily *inf;
    int error;

    if(inp == NULL || intorawpcb(inp) == NULL) {
        if(op == PRCO_SETOPT && m != NULL && *m != NULL) {
            (void)m_free(*m);
            *m = NULL;
        }
        return (EINVAL);
    }
    inf = inp->inp_family;
    if(inf->inf_rawctloutput != NULL &&
            (error = (*inf->inf_rawctloutput)(op, so, level, optname, m)) >= 0)
        return (error);
    return ((*inf->inf_ctloutput)(op, so, level, optname, m));
}

/* ------------------------------------------------------------------
 * User requests.
 * ------------------------------------------------------------------ */
static void
rip_detach(struct inpcb *inp)
{
    if(inp->inp_ppcb) {
        bsd_free(inp->inp_ppcb, M_PCB);
        inp->inp_ppcb = NULL;
    }
    in_pcbdetach(inp);
}

int
rip_usrreq(so, req, m, nam, control)
register struct socket *so;
int req;
struct mbuf *m, *nam, *control;
{
    struct inpcb *inp = sotoinpcb(so);
    const struct inetfamily *inf;
    const void *addr = NULL;
    struct ifnet *zone = NULL;
    u_short port;
    int error = 0;
    int s;

    if(req == PRU_CONTROL) {
        /* the family's interface requests; the arguments are not mbufs */
        inf = inp ? inp->inp_family :
              inetfamily_find(so->so_proto->pr_domain->dom_family);
        if(inf == NULL || inf->inf_control == NULL)
            return (EOPNOTSUPP);
        return ((*inf->inf_control)(so, (int)(long)m, (caddr_t)nam,
                                    (struct ifnet *)control));
    }

    if(inp == NULL && req != PRU_ATTACH) {
        error = EINVAL;
        goto release;
    }

    switch(req) {

    case PRU_ATTACH: {
        struct rawpcb *rp;

        if(inp != NULL) {
            error = EINVAL;
            break;
        }
        if((error = soreserve(so, rip_sendspace, rip_recvspace)) != 0)
            break;
        s = splnet();
        error = in_pcballoc(so, &ripcbinfo);
        splx(s);
        if(error)
            break;
        inp = sotoinpcb(so);
        inf = inp->inp_family;
        rp = NULL;
        if(inf->inf_rawoutput == NULL)
            error = EPROTONOSUPPORT;    /* the family has no raw sockets */
        else {
            MALLOC(rp, struct rawpcb *, sizeof(*rp) + inf->inf_rawprivlen,
                   M_PCB, M_WAITOK);
            if(rp == NULL)
                error = ENOBUFS;
        }
        if(error) {
            s = splnet();
            in_pcbdetach(inp);
            splx(s);
            break;
        }
        bzero((caddr_t)rp, sizeof(*rp) + inf->inf_rawprivlen);
        rp->rp_proto = (int)(long)nam;
        rp->rp_cksum = -1;
        inp->inp_ppcb = (caddr_t)rp;
        if(inf->inf_rawattach != NULL)
            (*inf->inf_rawattach)(inp);
        break;
    }

    case PRU_DETACH:
        s = splnet();
        rip_detach(inp);
        splx(s);
        break;

    case PRU_ABORT:
        soisdisconnected(so);
        s = splnet();
        rip_detach(inp);
        splx(s);
        break;

    /*
     * The address must be one of ours, or "any". Rebinding is allowed. A
     * raw socket has no port: the one in the address is not kept.
     */
    case PRU_BIND:
        inf = inp->inp_family;
        if(nam == NULL) {
            error = EINVAL;
            break;
        }
        if((error = (*inf->inf_sockaddr_parse)(nam, &addr, &port, &zone)) != 0)
            break;
        if(!inpcb_addrany(addr, inf->inf_addrlen) &&
                !(*inf->inf_ownaddr)(addr, zone)) {
            error = EADDRNOTAVAIL;
            break;
        }
        inpcb_setconn(inp, addr, 0, &inp->inp_fa, 0, zone);
        break;

    /*
     * From now on only the peer's packets are received and send() goes
     * to it. The source is fixed now, so that getsockname() reports it -
     * unless the family's output is what chooses it (INFR_SRCLATE).
     */
    case PRU_CONNECT: {
        union inp_addr laddr;
        const void *lp = &inp->inp_la;

        inf = inp->inp_family;
        if(nam == NULL) {
            error = EINVAL;
            break;
        }
        if((error = (*inf->inf_sockaddr_parse)(nam, &addr, &port, &zone)) != 0)
            break;
        bzero((caddr_t)&laddr, sizeof(laddr));
        if(inpcb_addrany(&inp->inp_la, inf->inf_addrlen) &&
                !(inf->inf_rawflags & INFR_SRCLATE) &&
                (*inf->inf_selectsrc)(inp, addr, zone ? zone : inp->inp_boundif,
                                      &laddr) == 0)
            lp = &laddr;
        inpcb_setconn(inp, lp, 0, addr, 0, zone);
        soisconnected(so);
        break;
    }

    case PRU_DISCONNECT:
        if(!(so->so_state & SS_ISCONNECTED)) {
            error = ENOTCONN;
            break;
        }
        inpcb_setconn(inp, &inp->inp_la, 0, NULL, 0, NULL);
        so->so_state &= ~SS_ISCONNECTED;
        break;

    /*
     * Mark the connection as being incapable of further input.
     */
    case PRU_SHUTDOWN:
        socantsendmore(so);
        break;

    case PRU_SEND:
        return (rip_send(inp, m, nam, control));

    case PRU_SOCKADDR:
        inpcb_setsockaddr(inp, nam);
        break;

    case PRU_PEERADDR:
        inpcb_setpeeraddr(inp, nam);
        break;

    case PRU_SENSE:
        /*
         * stat: don't bother with a blocksize.
         */
        return (0);

    /*
     * Not supported.
     */
    case PRU_RCVOOB:
    case PRU_RCVD:
        return (EOPNOTSUPP);

    default:
        error = EOPNOTSUPP;
        break;
    }

release:
    if(control)
        m_freem(control);
    if(m)
        m_freem(m);
    return (error);
}
