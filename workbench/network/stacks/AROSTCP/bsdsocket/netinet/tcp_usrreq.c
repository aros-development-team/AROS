/*
 * Copyright (C) 1982, 1986, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
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
 *	From: @(#)tcp_usrreq.c	8.2 (Berkeley) 1/3/94
 *	$Id$
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/synch.h>
#ifdef ENABLE_SYSCTL
#include <sys/sysctl.h>
#endif

#include <sys/domain.h>

#include <net/route.h>
#include <net/if.h>
#include <net/netmodule.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>         /* struct inpcb holds one; not used here */
#include <netinet/in_pcb.h>
#include <netinet/inetfamily.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#ifdef TCPDEBUG
#include <netinet/tcp_debug.h>
#endif

#include "tcp_compat.h"
#include "tcp_syncache.h"           /* syncache_listen() */
#include <protos/kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

/*
 * TCP protocol interface to socket abstraction.
 */
extern	char *tcpstates[];

/*
 * tcp_disconnect()/tcp_close()/tcp_drop()/tcp_timers()/sototcpcb() yield the
 * resulting tcpcb (NULL once the block has been freed).  That result is read
 * only by the TCPDEBUG trace at the end of tcp_usrreq(), so it is assigned back
 * to tp only in debug builds -- otherwise it is a dead store when TCPDEBUG is
 * disabled.  The wrapped call is always evaluated.
 */
#ifdef TCPDEBUG
#define TP_DEBUG_ASSIGN(tp, expr)	((tp) = (expr))
#else
#define TP_DEBUG_ASSIGN(tp, expr)	((void)(expr))
#endif

/*
 * Process a TCP user request for TCP tb.  If this is a send request
 * then m is the mbuf chain of send data.  If this is a timer expiration
 * (called from the software clock routine), then timertype tells which timer.
 */
/*
 * A connect found another PCB with the same four addresses and ports
 * (inpcb_connect()). If that is an earlier incarnation of the connection
 * still in TIME_WAIT, for less than MSL, with a peer that sent CC options
 * (RFC 1644 section 3.3: TIME-WAIT truncation), it is closed and the
 * connect goes on; else the address is in use. The rule is TCP's and has
 * nothing of a family in it: it applies to every family.
 */
static int
tcp_connreuse(struct inpcb *oinp)
{
    struct tcpcb *otp = intotcpcb(oinp);

    if(otp == NULL || otp->t_state != TCPS_TIME_WAIT ||
            otp->t_duration >= TCPTV_MSL || !(otp->t_flags & TF_RCVD_CC))
        return (0);
    (void) tcp_close(otp);
    return (1);
}

/*ARGSUSED*/
int
tcp_usrreq(so, req, m, nam, control)
struct socket *so;
int req;
struct mbuf *m, *nam, *control;
{
    register struct inpcb *inp;
    register struct tcpcb *tp = 0;
    int s;
    int error = 0;
#ifdef TCPDEBUG
    int ostate;
#endif

    if(req == PRU_CONTROL) {
        /*
         * An ioctl: the family of the socket knows its address requests.
         * (The socket's, not the PCB's: a connection of another family
         * that the socket accepted is still that socket to its owner.)
         */
        const struct inetfamily *inf =
            inetfamily_find(so->so_proto->pr_domain->dom_family);

        if(inf == NULL || inf->inf_control == NULL)
            return (EOPNOTSUPP);
        return ((*inf->inf_control)(so, (int)(long)m, (caddr_t)nam,
                                    (struct ifnet *)control));
    }
    if(control && control->m_len) {
        m_freem(control);
        if(m)
            m_freem(m);
        return (EINVAL);
    }

    s = splnet();
    inp = sotoinpcb(so);
    /*
     * When a TCP is attached to a socket, then there will be
     * a (struct inpcb) pointed at by the socket, and this
     * structure will point at a subsidary (struct tcpcb).
     */
    if(inp == 0 && req != PRU_ATTACH) {
        splx(s);
        return (EINVAL);		/* XXX */
    }
    if(inp) {
        tp = intotcpcb(inp);
        /* WHAT IF TP IS 0? */
#ifdef KPROF
        tcp_acounts[tp->t_state][req]++;
#endif
#ifdef TCPDEBUG
        ostate = tp->t_state;
    } else
        ostate = 0;
#else /* TCPDEBUG */
    }
#endif /* TCPDEBUG */

    switch(req)
    {

        /*
         * TCP attaches to socket via PRU_ATTACH, reserving space,
         * and an internet control block.
         */
    case PRU_ATTACH:
        if(inp) {
            error = EISCONN;
            break;
        }
        error = tcp_attach(so);
        if(error)
            break;
        if((so->so_options & SO_LINGER) && so->so_linger.tv_sec == 0)
            so->so_linger.tv_sec = TCP_LINGERTIME;
        TP_DEBUG_ASSIGN(tp, sototcpcb(so));
        break;

        /*
         * PRU_DETACH detaches the TCP protocol from the socket.
         * If the protocol state is non-embryonic, then can't
         * do this directly: have to initiate a PRU_DISCONNECT,
         * which may finish later; embryonic TCB's can just
         * be discarded here.
         */
    case PRU_DETACH:
        if(tp->t_state > TCPS_LISTEN)
            TP_DEBUG_ASSIGN(tp, tcp_disconnect(tp));
        else
            TP_DEBUG_ASSIGN(tp, tcp_close(tp));
        break;

        /*
         * Give the socket an address.
         */
    case PRU_BIND:
        /*
         * A connection has no multicast end: refuse such an address,
         * whichever family it is of.
         */
        if((*inp->inp_family->inf_sa_multicast)(nam)) {
            error = EAFNOSUPPORT;
            break;
        }
        error = inpcb_bind(inp, nam);
        break;

        /*
         * Prepare to accept connections.
         */
    case PRU_LISTEN:
        /*
         * The SYN cache's table comes with the first listener, of either
         * family: a machine that never listens does not pay for it.
         * Without it the socket cannot listen (ENOBUFS).
         */
        error = syncache_listen();
        if(error == 0 && inp->inp_lport == 0)
            error = inpcb_bind(inp, (struct mbuf *)0);
        if(error == 0)
            tp->t_state = TCPS_LISTEN;
        break;

        /*
         * Initiate connection to peer.
         * Create a template for use in transmissions on this connection.
         * Enter SYN_SENT state, and mark socket as connecting.
         * Start keep-alive timer, and seed output sequence space.
         * Send initial segment on connection.
         */
    case PRU_CONNECT:
        /*
         * Must disallow TCP ``connections'' to multicast addresses,
         * whichever family they are of.
         */
        if((*inp->inp_family->inf_sa_multicast)(nam)) {
            error = EAFNOSUPPORT;
            break;
        }

        if((error = tcp_connect(tp, nam)) != 0)
            break;
        error = tcp_output(tp);
        break;

        /*
         * Create a TCP connection between two sockets.
         */
    case PRU_CONNECT2:
        error = EOPNOTSUPP;
        break;

        /*
         * Initiate disconnect from peer.
         * If connection never passed embryonic stage, just drop;
         * else if don't need to let data drain, then can just drop anyways,
         * else have to begin TCP shutdown process: mark socket disconnecting,
         * drain unread data, state switch to reflect user close, and
         * send segment (e.g. FIN) to peer.  Socket will be really disconnected
         * when peer sends FIN and acks ours.
         *
         * SHOULD IMPLEMENT LATER PRU_CONNECT VIA REALLOC TCPCB.
         */
    case PRU_DISCONNECT:
        tp = tcp_disconnect(tp);
        break;

        /*
         * Accept a connection.  Essentially all the work is
         * done at higher levels; just return the address
         * of the peer, storing through addr.
         */
    case PRU_ACCEPT:
        inpcb_setpeeraddr(inp, nam);
        break;

        /*
         * Mark the connection as being incapable of further output.
         */
    case PRU_SHUTDOWN:
        socantsendmore(so);
        tp = tcp_usrclosed(tp);
        if(tp)
            error = tcp_output(tp);
        break;

        /*
         * After a receive, possibly send window update to peer.
         */
    case PRU_RCVD:
        (void) tcp_output(tp);
        break;

        /*
         * Do a send by putting data in output queue and updating urgent
         * marker if URG set.  Possibly send more data.
         */
    case PRU_SEND_EOF:
    case PRU_SEND:
        sbappend(&so->so_snd, m);
        if(nam && tp->t_state < TCPS_SYN_SENT) {
            /*
             * Do implied connect if not yet connected,
             * initialize window to default value, and
             * initialize maxseg/maxopd using peer's cached
             * MSS.
             */
            error = tcp_connect(tp, nam);
            if(error)
                break;
            tp->snd_wnd = TTCP_CLIENT_SND_WND;
            tcp_mss(tp, -1);
        }

        if(req == PRU_SEND_EOF) {
            /*
             * Close the send side of the connection after
             * the data is sent.
             */
            socantsendmore(so);
            tp = tcp_usrclosed(tp);
        }
        if(tp != NULL)
            error = tcp_output(tp);
        break;

        /*
         * Abort the TCP.
         */
    case PRU_ABORT:
        TP_DEBUG_ASSIGN(tp, tcp_drop(tp, ECONNABORTED));
        break;

    case PRU_SENSE:
        ((struct stat *) m)->st_blksize = so->so_snd.sb_hiwat;
        (void) splx(s);
        return (0);

    case PRU_RCVOOB:
        if((so->so_oobmark == 0 &&
                (so->so_state & SS_RCVATMARK) == 0) ||
                so->so_options & SO_OOBINLINE ||
                tp->t_oobflags & TCPOOB_HADDATA) {
            error = EINVAL;
            break;
        }
        if((tp->t_oobflags & TCPOOB_HAVEDATA) == 0) {
            error = EWOULDBLOCK;
            break;
        }
        m->m_len = 1;
        *mtod(m, caddr_t) = tp->t_iobc;
        if(((long)nam & MSG_PEEK) == 0)
            tp->t_oobflags ^= (TCPOOB_HAVEDATA | TCPOOB_HADDATA);
        break;

    case PRU_SENDOOB:
        if(sbspace(&so->so_snd) < -512) {
            m_freem(m);
            error = ENOBUFS;
            break;
        }
        /*
         * According to RFC961 (Assigned Protocols),
         * the urgent pointer points to the last octet
         * of urgent data.  We continue, however,
         * to consider it to indicate the first octet
         * of data past the urgent section.
         * Otherwise, snd_up should be one lower.
         */
        sbappend(&so->so_snd, m);
        tp->snd_up = tp->snd_una + so->so_snd.sb_cc;
        tp->t_force = 1;
        error = tcp_output(tp);
        tp->t_force = 0;
        break;

    /*
     * The addresses are written in the form of the socket's family, which
     * is not the PCB's for an IPv4 connection accepted on an IPv6
     * socket (inpcb_setsockaddr()).
     */
    case PRU_SOCKADDR:
        inpcb_setsockaddr(inp, nam);
        break;

    case PRU_PEERADDR:
        inpcb_setpeeraddr(inp, nam);
        break;

        /*
         * TCP slow timer went off; going through this
         * routine for tracing's sake.
         */
    case PRU_SLOWTIMO:
        TP_DEBUG_ASSIGN(tp, tcp_timers(tp, (long)nam));
#ifdef TCPDEBUG
        req |= (int)nam << 8;		/* for debug's sake */
#endif
        break;

    default:
        panic("tcp_usrreq");
    }
#ifdef TCPDEBUG
    if(tp && (so->so_options &SO_DEBUG))
        tcp_trace(TA_USER, ostate, tp, (struct tcpiphdr *)0, req);
#endif
    splx(s);
    return (error);
}

/*
 * Common subroutine to open a TCP connection to remote host specified
 * by the socket address in mbuf *nam.  Bind a local port number if
 * needed.  Have the PCB layer do the routing and choose a local host
 * address (interface).  If there is an existing incarnation
 * of the same connection in TIME-WAIT state and if the remote host was
 * sending CC options and if the connection duration was < MSL, then
 * truncate the previous TIME-WAIT state and proceed.
 * Initialize connection parameters and enter SYN-SENT state.
 */
int
tcp_connect(tp, nam)
register struct tcpcb *tp;
struct mbuf *nam;
{
    struct inpcb *inp = tp->t_inpcb;
    struct socket *so = inp->inp_socket;
    int error;

    if(inp->inp_lport == 0) {
        error = inpcb_bind(inp, (struct mbuf *)0);
        if(error)
            return error;
    }

    /*
     * The peer, and the local address the family chooses for it. An
     * earlier incarnation of this same connection still in TIME_WAIT
     * does not make the address "in use" (tcp_connreuse()).
     */
    error = inpcb_connect(inp, nam, tcp_connreuse);
    if(error)
        return error;

    tp->t_template = tcp_template(tp);
    if(tp->t_template == 0) {
        inpcb_disconnect(inp);
        return ENOBUFS;
    }

    /* Compute window scaling to request.  */
    tp->request_r_scale = tcp_rscale(so);

    soisconnecting(so);
    tcpstat.tcps_connattempt++;
    tp->t_state = TCPS_SYN_SENT;
    tp->t_timer[TCPT_KEEP] = TP_KEEPINIT(tp);
    tp->iss = tcp_new_isn(inp);
    tp->t_ts_offset = tcp_new_tsoff(inp);
    tcp_sendseqinit(tp);
    tp->cc_send = CC_INC(tcp_ccgen);

    return 0;
}

/*
 * TCP_INFO: what FreeBSD's tcp_fill_info() gives of the connection, in
 * its struct tcp_info (Linux's names and order). Times are in
 * microseconds; the fields this stack does not keep stay 0.
 */
#define TCP_TICK_USEC   (1000000 / PR_SLOWHZ)      /* t_idle counts slow ticks */

static void
tcp_fill_info(const struct tcpcb *tp, struct tcp_info *ti)
{
    bzero(ti, sizeof(*ti));
    ti->tcpi_state = tp->t_state;
    if((tp->t_flags & TF_REQ_TSTMP) && (tp->t_flags & TF_RCVD_TSTMP))
        ti->tcpi_options |= TCPI_OPT_TIMESTAMPS;
    if(tp->t_flags & TF_SACK_PERMIT)
        ti->tcpi_options |= TCPI_OPT_SACK;
    if((tp->t_flags & TF_REQ_SCALE) && (tp->t_flags & TF_RCVD_SCALE)) {
        ti->tcpi_options |= TCPI_OPT_WSCALE;
        ti->tcpi_snd_wscale = tp->snd_scale;
        ti->tcpi_rcv_wscale = tp->rcv_scale;
    }
    if(tp->t_flagsext & TF_ECN_PERMIT)
        ti->tcpi_options |= TCPI_OPT_ECN;

    ti->tcpi_rto = (u_int32_t)tp->t_rxtcur * 1000;
    ti->tcpi_last_data_recv = (u_int32_t)tp->t_idle * TCP_TICK_USEC;
    ti->tcpi_rtt = ((u_int32_t)tp->t_srtt * 1000) >> TCP_RTT_SHIFT;
    ti->tcpi_rttvar = ((u_int32_t)tp->t_rttvar * 1000) >> TCP_RTTVAR_SHIFT;
    ti->tcpi_snd_ssthresh = tp->snd_ssthresh;
    ti->tcpi_snd_cwnd = tp->snd_cwnd;
    ti->tcpi_snd_mss = tp->t_maxseg;
    ti->tcpi_rcv_mss = tp->t_maxseg;

    ti->tcpi_rcv_space = tp->rcv_wnd;
    ti->tcpi_rcv_nxt = tp->rcv_nxt;
    ti->tcpi_snd_wnd = tp->snd_wnd;
    ti->tcpi_snd_nxt = tp->snd_nxt;
    ti->tcpi_snd_una = tp->snd_una;
    ti->tcpi_snd_max = tp->snd_max;
    ti->tcpi_rcv_adv = tp->rcv_adv;
    ti->tcpi_dupacks = tp->t_dupacks;
}

int
tcp_ctloutput(op, so, level, optname, mp)
int op;
struct socket *so;
int level, optname;
struct mbuf **mp;
{
    int error = 0, s;
    struct inpcb *inp;
    register struct tcpcb *tp;
    register struct mbuf *m;
    register int i;

    s = splnet();
    inp = sotoinpcb(so);
    if(inp == NULL) {
        splx(s);
        if(op == PRCO_SETOPT && *mp)
            (void) m_free(*mp);
        return (ECONNRESET);
    }
    if(level != IPPROTO_TCP) {
        /*
         * An option of a family's level goes to that family, whichever
         * family the socket is of (as it always did). A level no family
         * of the running stack answers for is refused, as the IPv4 code
         * used to refuse what it did not know.
         */
        const struct inetfamily *inf = inetfamily_bylevel(level);

        if(inf != NULL)
            error = (*inf->inf_ctloutput)(op, so, level, optname, mp);
        else {
            error = EINVAL;
            if(op == PRCO_SETOPT && *mp)
                (void) m_free(*mp);
        }
        splx(s);
        return (error);
    }
    tp = intotcpcb(inp);

    switch(op) {

    case PRCO_SETOPT:
        m = *mp;
        switch(optname) {

        case TCP_NODELAY:
            if(m == NULL || m->m_len < sizeof(int))
                error = EINVAL;
            else if(*mtod(m, int *))
                tp->t_flags |= TF_NODELAY;
            else
                tp->t_flags &= ~TF_NODELAY;
            break;

        case TCP_MAXSEG:
            /* as FreeBSD: not above the MSS in use, nor (with the
             * room for options) below tcp_minmss */
            if(m && (i = *mtod(m, int *)) > 0 && i <= tp->t_maxseg &&
                    i + TCP_MAXOLEN >= TCP_MSS_FLOOR)
                tp->t_maxseg = i;
            else
                error = EINVAL;
            break;

        case TCP_NOOPT:
            if(m == NULL || m->m_len < sizeof(int))
                error = EINVAL;
            else if(*mtod(m, int *))
                tp->t_flags |= TF_NOOPT;
            else
                tp->t_flags &= ~TF_NOOPT;
            break;

        case TCP_NOPUSH:
            if(m == NULL || m->m_len < sizeof(int))
                error = EINVAL;
            else if(*mtod(m, int *))
                tp->t_flags |= TF_NOPUSH;
            else
                tp->t_flags &= ~TF_NOPUSH;
            break;

        /*
         * Keepalive per connection, as FreeBSD: times in seconds, the count
         * in probes; 0 gives back the system value. A change re-arms the
         * timer it concerns. Up to TCP_KEEP_MAXSECS, which the 16-bit
         * slow-tick timers hold.
         */
        case TCP_KEEPIDLE:
        case TCP_KEEPINTVL:
        case TCP_KEEPINIT:
        case TCP_KEEPCNT:
            if(m == NULL || m->m_len < sizeof(int) || (i = *mtod(m, int *)) < 0 ||
                    i > (optname == TCP_KEEPCNT ? 32767 : TCP_KEEP_MAXSECS)) {
                error = EINVAL;
                break;
            }
            switch(optname) {
            case TCP_KEEPIDLE:
                tp->t_keepidle = i * PR_SLOWHZ;
                if(tp->t_state >= TCPS_ESTABLISHED && tp->t_state <= TCPS_CLOSING)
                    tp->t_timer[TCPT_KEEP] = TP_KEEPIDLE(tp);
                break;
            case TCP_KEEPINIT:
                tp->t_keepinit = i * PR_SLOWHZ;
                if(tp->t_state == TCPS_SYN_SENT || tp->t_state == TCPS_SYN_RECEIVED)
                    tp->t_timer[TCPT_KEEP] = TP_KEEPINIT(tp);
                break;
            default:
                if(optname == TCP_KEEPINTVL)
                    tp->t_keepintvl = i * PR_SLOWHZ;
                else
                    tp->t_keepcnt = i;
                if(tp->t_state == TCPS_FIN_WAIT_2 && tp->t_timer[TCPT_2MSL])
                    tp->t_timer[TCPT_2MSL] = TP_MAXIDLE(tp);
                break;
            }
            break;

        case TCP_USER_TIMEOUT:      /* ms, RFC 5482; 0 is off */
            if(m == NULL || m->m_len < sizeof(int) || *mtod(m, int *) < 0)
                error = EINVAL;
            else
                tp->t_uto_ms = (u_int)*mtod(m, int *);
            break;

        default:
            error = ENOPROTOOPT;
            break;
        }
        if(m)
            (void) m_free(m);
        break;

    case PRCO_GETOPT:
        *mp = m = m_get(M_WAIT, MT_SOOPTS);
        if(m == NULL) {		/* allocator may fail here */
            error = ENOBUFS;
            break;
        }
        m->m_len = sizeof(int);

        switch(optname) {
        case TCP_NODELAY:
            *mtod(m, int *) = tp->t_flags & TF_NODELAY;
            break;
        case TCP_MAXSEG:
            *mtod(m, int *) = tp->t_maxseg;
            break;
        case TCP_NOOPT:
            *mtod(m, int *) = tp->t_flags & TF_NOOPT;
            break;
        case TCP_NOPUSH:
            *mtod(m, int *) = tp->t_flags & TF_NOPUSH;
            break;
        case TCP_KEEPIDLE:          /* what the connection uses */
            *mtod(m, int *) = TP_KEEPIDLE(tp) / PR_SLOWHZ;
            break;
        case TCP_KEEPINTVL:
            *mtod(m, int *) = TP_KEEPINTVL(tp) / PR_SLOWHZ;
            break;
        case TCP_KEEPINIT:
            *mtod(m, int *) = TP_KEEPINIT(tp) / PR_SLOWHZ;
            break;
        case TCP_KEEPCNT:
            *mtod(m, int *) = TP_KEEPCNT(tp);
            break;
        case TCP_USER_TIMEOUT:
            *mtod(m, int *) = (int)tp->t_uto_ms;
            break;
        case TCP_INFO:
            if(sizeof(struct tcp_info) > MLEN) {
                MCLGET(m, M_WAIT);
                if(!(m->m_flags & M_EXT)) {
                    error = ENOBUFS;
                    break;
                }
            }
            m->m_len = sizeof(struct tcp_info);
            tcp_fill_info(tp, mtod(m, struct tcp_info *));
            break;
        default:
            error = ENOPROTOOPT;
            break;
        }
        break;
    }
    splx(s);
    return (error);
}

/*
 * tcp_sendspace and tcp_recvspace are the default send and receive window
 * sizes, respectively.  These are obsolescent (this information should
 * be set by the route).
 */
#if defined(__mc68000__)
#define TCP_SPACE_DFLT	(1024 * 16)
#else
#define TCP_SPACE_DFLT	(1024 * 128)
#endif
u_long	tcp_sendspace = TCP_SPACE_DFLT;
u_long	tcp_recvspace = TCP_SPACE_DFLT;

/*
 * Attach TCP protocol to socket, allocating
 * internet protocol control block, tcp control block,
 * bufer space, and entering LISTEN state if to accept connections.
 */
int
tcp_attach(so)
struct socket *so;
{
    register struct tcpcb *tp;
    struct inpcb *inp;
    int error;

    if(so->so_snd.sb_hiwat == 0 || so->so_rcv.sb_hiwat == 0) {
        error = soreserve(so, tcp_sendspace, tcp_recvspace);
        if(error)
            return (error);
        /* the defaults grow as needed, until SO_SNDBUF/SO_RCVBUF */
        so->so_snd.sb_flags |= SB_AUTOSIZE;
        so->so_rcv.sb_flags |= SB_AUTOSIZE;
    }
    error = in_pcballoc(so, &tcbinfo);
    if(error)
        return (error);
    inp = sotoinpcb(so);
    tp = tcp_newtcpcb(inp);
    if(tp == 0) {
        int nofd = so->so_state & SS_NOFDREF;	/* XXX */

        so->so_state &= ~SS_NOFDREF;	/* don't free the socket yet */
        in_pcbdetach(inp);
        so->so_state |= nofd;
        return (ENOBUFS);
    }
    tp->t_state = TCPS_CLOSED;
    return (0);
}

/*
 * Initiate (or continue) disconnect.
 * If embryonic state, just send reset (once).
 * If in ``let data drain'' option and linger null, just drop.
 * If received data is still unread, drop with a reset too, so the peer
 * learns it was lost (RFC 9293 section 3.6.1; RFC 1122 4.2.2.13).
 * Otherwise (hard), mark socket disconnecting and drop
 * current input data; switch states based on user close, and
 * send segment to peer (with FIN).
 */
struct tcpcb *
tcp_disconnect(tp)
register struct tcpcb *tp;
{
    struct socket *so = tp->t_inpcb->inp_socket;

    if(tp->t_state < TCPS_ESTABLISHED)
        tp = tcp_close(tp);
    else if((so->so_options & SO_LINGER) && so->so_linger.tv_sec == 0)
        tp = tcp_drop(tp, 0);
    else if(so->so_rcv.sb_cc > 0)
        tp = tcp_drop(tp, ECONNRESET);
    else {
        soisdisconnecting(so);
        sbflush(&so->so_rcv);
        tp = tcp_usrclosed(tp);
        if(tp)
            (void) tcp_output(tp);
    }
    return (tp);
}

/*
 * User issued close, and wish to trail through shutdown states:
 * if never received SYN, just forget it.  If got a SYN from peer,
 * but haven't sent FIN, then go to FIN_WAIT_1 state to send peer a FIN.
 * If already got a FIN from peer, then almost done; go to LAST_ACK
 * state.  In all other cases, have already sent FIN to peer (e.g.
 * after PRU_SHUTDOWN), and just have to play tedious game waiting
 * for peer to send FIN or not respond to keep-alives, etc.
 * We can let the user exit from the close as soon as the FIN is acked.
 */
struct tcpcb *
tcp_usrclosed(tp)
register struct tcpcb *tp;
{

    switch(tp->t_state) {

    case TCPS_CLOSED:
    case TCPS_LISTEN:
        tp->t_state = TCPS_CLOSED;
        tp = tcp_close(tp);
        break;

    case TCPS_SYN_SENT:
    case TCPS_SYN_RECEIVED:
        tp->t_flags |= TF_NEEDFIN;
        break;

    case TCPS_ESTABLISHED:
        tp->t_state = TCPS_FIN_WAIT_1;
        break;

    case TCPS_CLOSE_WAIT:
        tp->t_state = TCPS_LAST_ACK;
        break;
    }
    if(tp && tp->t_state >= TCPS_FIN_WAIT_2)
        soisdisconnected(tp->t_inpcb->inp_socket);
    return (tp);
}

/*
 * TCP's variables and statistics, for whoever reaches them from outside
 * TCP: the configuration and ARexx variables (TCP, TCP_SENDSPACE,
 * TCP_RECVSPACE), programs that look the stack's symbols up (netstat:
 * "_tcb", "_tcpstat") and sysctl (net.inet.tcp.<id>). None of them names
 * TCP: the table is registered when TCP starts and each looks its name up
 * (net/netmodule.h).
 *
 * The names under net.inet.tcp are TCP's, whichever families the stack
 * runs. (net.inet.tcp.mssdflt is not here: the default MSS is each
 * family's own, and IPv4 registers its.)
 */
#ifdef ENABLE_SYSCTL
/*
 * The variables whose new value is checked: out of range is EINVAL and
 * changes nothing.
 */
static int
tcp_sysctl_checked(const struct netvar *nv, void *oldp, size_t *oldlenp,
                   void *newp, size_t newlen)
{
    switch(nv->nv_id) {
    case TCPCTL_RTTDFLT: {          /* the initial RTO, s */
        int v = tcp_rttdflt, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 1 || v * 1000 > TCPTV_REXMTMAX_MS))
            error = EINVAL;
        if(error == 0)
            tcp_rttdflt = v;
        return (error);
    }
    case TCPCTL_ABC_L: {            /* slow start, L * SMSS per ACK at most */
        int v = tcp_abc_l, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 1 || v > 16))
            error = EINVAL;
        if(error == 0)
            tcp_abc_l = v;
        return (error);
    }
    case TCPCTL_HYSTART: {
        int v = tcp_do_hystart, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 0 || v > 1))
            error = EINVAL;
        if(error == 0)
            tcp_do_hystart = v;
        return (error);
    }
    case TCPCTL_MINMSS: {           /* the least MSS, bytes; 0: 64 */
        int v = tcp_minmss, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 0 || v > 1460))
            error = EINVAL;
        if(error == 0)
            tcp_minmss = v;
        return (error);
    }
    case TCPCTL_REXMIT_MIN: {       /* the least RTO, ms */
        int v = tcp_rexmit_min, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 1 || v > TCPTV_REXMTMAX_MS))
            error = EINVAL;
        if(error == 0)
            tcp_rexmit_min = v;
        return (error);
    }
    case TCPCTL_RACK: {
        int v = tcp_do_rack, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 0 || v > 1))
            error = EINVAL;
        if(error == 0)
            tcp_do_rack = v;
        return (error);
    }
    case TCPCTL_TLP: {
        int v = tcp_do_tlp, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 0 || v > 1))
            error = EINVAL;
        if(error == 0)
            tcp_do_tlp = v;
        return (error);
    }
    case TCPCTL_SENDBUF_MAX:
    case TCPCTL_RECVBUF_MAX: {      /* autosizing ceilings, bytes; 0: off */
        u_long *p = nv->nv_id == TCPCTL_SENDBUF_MAX ? &tcp_autosndbuf_max : &tcp_autorcvbuf_max;
        int v = *p, error;

        /* bounded by autosizing's budget (tcp_autobuf_grow()), not sb_max:
         * at two bytes of the pool per byte, 3/8 of what the pool may use */
        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && newp && (v < 0 || (u_long)v > tcp_autobuf_one()))
            error = EINVAL;
        if(error == 0)
            *p = v;
        return (error);
    }
    case TCPCTL_ECN: {
        int v = tcp_do_ecn, error;

        error = sysctl_int(oldp, oldlenp, newp, newlen, &v);
        if(error == 0 && (v < 0 || v > 2))
            error = EINVAL;
        if(error == 0)
            tcp_do_ecn = v;
        return (error);
    }
    }
    return (ENOPROTOOPT);
}
#define TCPVAR_CHECKED(name, id) \
    { name, (void *)tcp_sysctl_checked, 0, NVT_SYSCTL, PF_INET, IPPROTO_TCP, id }
#endif

#define TCPVAR(name, var, id) \
    { name, &(var), sizeof(int), NVT_INT, PF_INET, IPPROTO_TCP, id }

static const struct netvar tcp_netvars[] = {
    { "tcpstat", &tcpstat, sizeof(tcpstat), NVT_STATS, PF_INET, IPPROTO_TCP, TCPCTL_STATS },
    /* the list of TCP's PCBs, which netstat walks: a block read from
     * outside, as the statistics are */
    { "tcb",     &tcb,     sizeof(tcb),     NVT_STATS },
    /* (the two buffer sizes are u_long; what is read and set is an int's
     * worth of them, as it always was) */
    TCPVAR("tcp_sendspace",          tcp_sendspace,          TCPCTL_SENDSPACE),
    TCPVAR("tcp_recvspace",          tcp_recvspace,          TCPCTL_RECVSPACE),
    TCPVAR("tcp_do_rfc1323",         tcp_do_rfc1323,         TCPCTL_DO_RFC1323),
    TCPVAR("tcp_do_rfc1644",         tcp_do_rfc1644,         TCPCTL_DO_RFC1644),
    TCPVAR("tcp_keepidle",           tcp_keepidle,           TCPCTL_KEEPIDLE),
    TCPVAR("tcp_keepintvl",          tcp_keepintvl,          TCPCTL_KEEPINTVL),
    TCPVAR("tcp_do_pmtud",           tcp_do_pmtud,           TCPCTL_PMTUD),
    TCPVAR("tcp_do_pmtud_blackhole", tcp_do_pmtud_blackhole, TCPCTL_PMTUD_BLACKHOLE),
    TCPVAR("tcp_do_prr",             tcp_do_prr,             TCPCTL_PRR),
#ifdef ENABLE_SYSCTL
    TCPVAR_CHECKED("tcp_rttdflt",        TCPCTL_RTTDFLT),
    TCPVAR_CHECKED("tcp_abc_l",          TCPCTL_ABC_L),
    TCPVAR_CHECKED("tcp_do_hystart",     TCPCTL_HYSTART),
    TCPVAR_CHECKED("tcp_minmss",         TCPCTL_MINMSS),
    TCPVAR_CHECKED("tcp_rexmit_min",     TCPCTL_REXMIT_MIN),
    TCPVAR_CHECKED("tcp_do_rack",        TCPCTL_RACK),
    TCPVAR_CHECKED("tcp_do_tlp",         TCPCTL_TLP),
    TCPVAR_CHECKED("tcp_autosndbuf_max", TCPCTL_SENDBUF_MAX),
    TCPVAR_CHECKED("tcp_autorcvbuf_max", TCPCTL_RECVBUF_MAX),
    TCPVAR_CHECKED("tcp_do_ecn",         TCPCTL_ECN),
#endif
};

/* Called first thing by tcp_init(): a value the configuration set for one
 * of the variables before TCP started is given to it here. */
void
tcp_vars_register(void)
{
    (void)netvar_register(tcp_netvars, sizeof(tcp_netvars) / sizeof(tcp_netvars[0]));
}

/*
 * TCP stops. Taken back first: the core then keeps what the configuration
 * had set for a variable it knows by a keyword (TCP_SENDSPACE,
 * TCP_RECVSPACE; kern/config_var.c) and gives it to the variable again at
 * the next start. Then this file's variables are what the file gives them.
 */
void
tcp_vars_unregister(void)
{
    (void)netvar_unregister(tcp_netvars);
    tcp_sendspace = TCP_SPACE_DFLT;
    tcp_recvspace = TCP_SPACE_DFLT;
}
