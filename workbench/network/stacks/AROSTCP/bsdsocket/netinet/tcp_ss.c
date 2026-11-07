/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * Slow start: how much an ACK grows cwnd while cwnd < ssthresh, and, in
 * the first slow start of a connection, when it ends (HyStart++).
 *
 * By the bytes the ACK newly acknowledges, at most L * SMSS of them
 * (Appropriate Byte Counting, RFC 3465; net.inet.tcp.abc_l). L = 1, the
 * default, is RFC 5681's min(N, SMSS): 4.4BSD added one SMSS per ACK
 * whatever it acknowledged, so an ACK of a few bytes counted as a whole
 * segment. A larger L (FreeBSD uses 2) grows cwnd by up to L segments an
 * ACK when the receiver delays its ACKs; on a bottleneck bench (tests/tcp
 * in aros-matter) L = 2 overshot more - 30-50% more drops at the end of
 * slow start - for the same throughput, so it is not the default.
 *
 * HyStart++ (RFC 9406): standard slow start doubles cwnd every round trip
 * until a loss, overshooting the path by up to a window. HyStart++ watches
 * the minimum RTT of each round (rounds counted by sequence number,
 * windowEnd): when it grows by a threshold - the bottleneck queue
 * filling - the connection goes on in Conservative Slow Start (CSS, a
 * quarter of the growth) for CSS_ROUNDS rounds, then into congestion
 * avoidance with ssthresh = cwnd; an RTT that falls back below the CSS
 * baseline means the exit was premature, and slow start resumes. It is
 * used only while ssthresh still has its initial value (section 4.3): a
 * loss, ECN or the end of CSS sets ssthresh, and slow start is standard
 * from then on. Not paced, so an ACK applies at most L = 8 segments, the
 * RFC's value for that case; the other constants are its recommended ones.
 * net.inet.tcp.hystart turns it off.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#define HS_MIN_RTT_THRESH       4       /* ms */
#define HS_MAX_RTT_THRESH       16      /* ms */
#define HS_MIN_RTT_DIVISOR      8
#define HS_N_RTT_SAMPLE         8
#define HS_CSS_GROWTH_DIVISOR   4
#define HS_CSS_ROUNDS           5
#define HS_L                    8       /* not paced */

/* ssthresh as tcp_newtcpcb() sets it: no slow start has ended yet */
#define HS_INITIAL_SSTHRESH     ((u_long)TCP_MAXWIN << TCP_MAX_WINSHIFT)

int tcp_abc_l = 1;
int tcp_do_hystart = 1;

void
tcp_hystart_init(struct tcpcb *tp)
{
    struct tcp_hystart *hs = &tp->t_hystart;

    hs->last_min = hs->cur_min = hs->css_base = HS_INFINITY;
    hs->curr_rtt = 0;
    hs->window_end = 0;
    hs->samples = 0;
    hs->css_rounds = 0;
    hs->started = 0;
    hs->phase = tcp_do_hystart ? HS_SS : HS_DONE;
}

/*
 * HyStart++'s growth for this ACK in the first slow start; ends slow start
 * (ssthresh = cwnd) when CSS is over.
 */
static u_long
tcp_hystart_incr(struct tcpcb *tp, u_int acked)
{
    struct tcp_hystart *hs = &tp->t_hystart;
    tcp_seq ack = tp->snd_una + acked;  /* snd_una is not moved yet */
    u_long incr;

    /* the round ends when windowEnd is acknowledged */
    if(!hs->started || SEQ_GEQ(ack, hs->window_end)) {
        if(hs->started && hs->phase == HS_CSS &&
                ++hs->css_rounds >= HS_CSS_ROUNDS) {
            /* RTT inflation persisted throughout CSS */
            tp->snd_ssthresh = tp->snd_cwnd;
            hs->phase = HS_DONE;
            return 0;
        }
        hs->started = 1;
        hs->window_end = tp->snd_nxt;
        hs->last_min = hs->cur_min;
        hs->cur_min = HS_INFINITY;
        hs->samples = 0;
    }

    if(hs->curr_rtt != 0) {
        if(hs->curr_rtt < hs->cur_min)
            hs->cur_min = hs->curr_rtt;
        hs->samples++;
    }

    incr = MIN((u_long)acked, HS_L * (u_long)tp->t_maxseg);
    if(hs->phase == HS_SS) {
        if(hs->samples >= HS_N_RTT_SAMPLE && hs->cur_min != HS_INFINITY &&
                hs->last_min != HS_INFINITY) {
            u_int32_t thresh = hs->last_min / HS_MIN_RTT_DIVISOR;

            if(thresh < HS_MIN_RTT_THRESH)
                thresh = HS_MIN_RTT_THRESH;
            if(thresh > HS_MAX_RTT_THRESH)
                thresh = HS_MAX_RTT_THRESH;
            if(hs->cur_min >= hs->last_min + thresh) {
                hs->css_base = hs->cur_min;
                hs->css_rounds = 0;     /* this partial round counts */
                hs->phase = HS_CSS;
            }
        }
    } else {                            /* HS_CSS */
        incr /= HS_CSS_GROWTH_DIVISOR;
        if(hs->samples >= HS_N_RTT_SAMPLE && hs->cur_min < hs->css_base) {
            /* the exit was premature: resume slow start */
            hs->css_base = HS_INFINITY;
            hs->phase = HS_SS;
        }
    }
    return incr;
}

/* The bytes by which an ACK acknowledging `acked' new bytes grows cwnd in
 * slow start: HyStart++ in the first slow start, else byte counting. */
u_long
tcp_ss_incr(struct tcpcb *tp, u_int acked)
{
    if(tp->t_hystart.phase != HS_DONE) {
        if(tp->snd_ssthresh == HS_INITIAL_SSTHRESH)
            return tcp_hystart_incr(tp, acked);
        tp->t_hystart.phase = HS_DONE;  /* a loss or ECN ended it */
    }
    return MIN((u_long)acked, (u_long)tcp_abc_l * tp->t_maxseg);
}
