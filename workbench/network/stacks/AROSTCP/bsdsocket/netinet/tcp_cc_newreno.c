/*
 * Copyright (C) 1982, 1986, 1988, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2025 The AROS Dev Team
 *
 * NewReno congestion control for AROSTCP (RFC 3782).
 *
 * This is the classic BSD AIMD (Additive Increase / Multiplicative Decrease)
 * congestion control extracted into the pluggable CC framework.
 *
 * Slow start: cwnd += MSS per ACK (exponential growth)
 * Congestion avoidance: cwnd += MSS^2/cwnd per ACK (linear growth)
 * Loss: ssthresh = cwnd/2, cwnd = 1 MSS
 * RTO: ssthresh = cwnd/2, cwnd = 1 MSS
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_cc.h>

#include <conf.h>

/*
 * NewReno init — no per-connection state needed beyond the
 * standard cwnd/ssthresh already in struct tcpcb.
 */
static void
newreno_init(struct tcpcb *tp)
{
    /* Nothing to initialize — NewReno uses only cwnd/ssthresh */
    (void)tp;
}

/*
 * NewReno ACK processing — grow congestion window.
 *
 * Slow start (cwnd <= ssthresh):
 *   cwnd += MSS per ACK  →  exponential growth
 *
 * Congestion avoidance (cwnd > ssthresh):
 *   cwnd += MSS^2 / cwnd per ACK  →  ~1 MSS per RTT (linear)
 */
static void
newreno_on_ack(struct tcpcb *tp, u_int acked)
{
    u_int cw = tp->snd_cwnd;
    u_int incr = tp->t_maxseg;
    u_long max_cwnd = (u_long)TCP_MAXWIN << tp->snd_scale;

    (void)acked;

    if(cw > tp->snd_ssthresh)
        incr = incr * incr / cw;

    tp->snd_cwnd = MIN(cw + incr, max_cwnd);
}

/*
 * NewReno loss — multiplicative decrease (beta = 0.5).
 *
 * ssthresh = max(FlightSize/2, 2*MSS)
 *
 * Note: the caller (tcp_input.c) handles setting cwnd = 1 MSS,
 * entering fast recovery, and the dup-ACK inflation.
 */
static void
newreno_on_loss(struct tcpcb *tp)
{
    u_int win;

    win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
    if(win < 2)
        win = 2;
    tp->snd_ssthresh = win * tp->t_maxseg;
}

/*
 * NewReno RTO — full congestion window reset.
 *
 * ssthresh = max(FlightSize/2, 2*MSS)
 * cwnd = 1 MSS
 */
static void
newreno_on_rto(struct tcpcb *tp)
{
    u_int win;

    win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
    if(win < 2)
        win = 2;
    tp->snd_ssthresh = win * tp->t_maxseg;
    tp->snd_cwnd = tp->t_maxseg;
}

/*
 * NewReno ECN response — same as loss (beta = 0.5).
 *
 * ssthresh = max(FlightSize/2, 2*MSS)
 * cwnd = ssthresh (no slow start restart on ECN)
 */
static void
newreno_on_ecn(struct tcpcb *tp)
{
    u_int ecn_win;

    ecn_win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
    if(ecn_win < 2)
        ecn_win = 2;
    tp->snd_ssthresh = ecn_win * tp->t_maxseg;
    tp->snd_cwnd = tp->snd_ssthresh;
}

/*
 * NewReno after idle — no special handling needed.
 * The idle cwnd reset to IW10 is done in tcp_output.c before
 * calling the CC framework.
 */
static void
newreno_after_idle(struct tcpcb *tp)
{
    (void)tp;
}

/*
 * NewReno algorithm descriptor.
 */
struct tcp_cc_algo tcp_cc_newreno = {
    .name           = "newreno",
    .id             = TCP_CC_NEWRENO,
    .cc_init        = newreno_init,
    .cc_on_ack      = newreno_on_ack,
    .cc_on_loss     = newreno_on_loss,
    .cc_on_rto      = newreno_on_rto,
    .cc_on_ecn      = newreno_on_ecn,
    .cc_after_idle  = newreno_after_idle,
};
