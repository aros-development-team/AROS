/*
 * Copyright (C) 2025 The AROS Dev Team
 *
 * CUBIC congestion control algorithm (RFC 8312) for AROSTCP.
 *
 * CUBIC replaces the linear congestion avoidance growth of NewReno
 * with a cubic function of time since the last loss event:
 *
 *   W(t) = C * (t - K)^3 + W_max
 *
 * where:
 *   C     = 0.4 (scaling constant)
 *   K     = cbrt(W_max * (1-beta) / C) — time to reach W_max
 *   beta  = 0.7 (multiplicative decrease factor)
 *   W_max = window size before last loss event
 *   t     = elapsed time since last loss event
 *
 * This provides:
 *   - Faster window growth when far below W_max (concave region)
 *   - Cautious probing near W_max (plateau region)
 *   - Aggressive growth above W_max (convex region)
 *   - TCP-friendly mode to ensure fairness with Reno flows
 *
 * Time resolution: tcp_now ticks (500ms at PR_SLOWHZ=2).
 * All cwnd/ssthresh computations are in bytes using integer arithmetic
 * with 1024x fixed-point scaling where needed.
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
 * Integer cube root using Newton's method.
 * Returns floor(cbrt(x)) for unsigned long x.
 *
 * Newton iteration: y_{n+1} = (2*y_n + x/y_n^2) / 3
 * Converges in ~20 iterations for 32-bit values.
 */
u_long
tcp_cubic_root(u_long x)
{
    u_long y, y2;
    int i;

    if(x == 0)
        return 0;
    if(x < 8)
        return 1;

    /* Initial estimate via bit position */
    y = 1;
    {
        u_long tmp = x;
        int bits = 0;
        while(tmp > 1) {
            tmp >>= 1;
            bits++;
        }
        y = 1UL << ((bits + 2) / 3);
    }

    /* Newton iterations */
    for(i = 0; i < 20; i++) {
        u_long y_new;
        y2 = y * y;
        if(y2 == 0)
            break;
        y_new = (2 * y + x / y2) / 3;
        if(y_new >= y)
            break;
        y = y_new;
    }

    /* Verify: ensure y^3 <= x < (y+1)^3 */
    while(y * y * y > x && y > 0)
        y--;

    return y;
}

/*
 * Compute the CUBIC window target W(t) for the given elapsed time.
 *
 * W(t) = C * (t - K)^3 + origin_point   [in MSS units, then * MSS]
 *
 * Since PR_SLOWHZ=2, each tick is 0.5 seconds.
 * C_adj = C / Hz^3 = 0.4 / 8 = 0.05
 * In fixed-point (scale 1024): 51/1024 ~ 0.0498
 */
static u_long
cubic_window(struct tcpcb *tp, u_long t_ticks)
{
    struct cubic_state *cs = &tp->t_cubic;
    long delta_t, delta_t3, w_cubic;
    u_long mss = tp->t_maxseg;
    u_long origin_mss;

    if(mss == 0)
        mss = 1;

    origin_mss = cs->origin_point / mss;
    delta_t = (long)t_ticks - (long)cs->k;

    /* Clamp to avoid overflow in delta_t^3 */
    if(delta_t > 1000)
        delta_t = 1000;
    if(delta_t < -1000)
        delta_t = -1000;

    delta_t3 = delta_t * delta_t * delta_t;
    w_cubic = (long)origin_mss + (51L * delta_t3) / 1024L;

    if(w_cubic < 1)
        w_cubic = 1;

    return (u_long)w_cubic * mss;
}

/*
 * Initialize CUBIC state for a new connection.
 */
static void
cubic_init(struct tcpcb *tp)
{
    struct cubic_state *cs = &tp->t_cubic;

    cs->wmax = 0;
    cs->epoch_start = 0;
    cs->origin_point = 0;
    cs->k = 0;
    cs->tcp_cwnd = 0;
    cs->w_est = 0;
    cs->ack_cnt = 0;
    cs->flags = CUBIC_FLAG_RESET;
}

/*
 * Reset the CUBIC epoch after an idle period.
 */
static void
cubic_after_idle(struct tcpcb *tp)
{
    struct cubic_state *cs = &tp->t_cubic;

    cs->epoch_start = 0;
    cs->flags |= CUBIC_FLAG_RESET;
}

/*
 * CUBIC congestion avoidance: called on each non-duplicate ACK
 * when not in fast recovery.
 *
 * For slow start (cwnd <= ssthresh), uses standard exponential growth.
 * For congestion avoidance, computes the CUBIC target window.
 */
static void
cubic_on_ack(struct tcpcb *tp, u_int acked)
{
    struct cubic_state *cs = &tp->t_cubic;
    u_long mss = tp->t_maxseg;
    u_long cwnd = tp->snd_cwnd;
    u_long target, elapsed;
    u_long max_cwnd = (u_long)TCP_MAXWIN << tp->snd_scale;

    if(mss == 0)
        return;

    /* Slow start: standard exponential growth (RFC 8312 §5.1) */
    if(cwnd <= tp->snd_ssthresh) {
        tp->snd_cwnd = MIN(cwnd + mss, max_cwnd);
        return;
    }

    /* Start a new epoch if needed */
    if(cs->epoch_start == 0) {
        cs->epoch_start = tcp_now;
        cs->ack_cnt = 0;
        if(cwnd < cs->origin_point) {
            u_long delta_mss = (cs->origin_point - cwnd) / mss;
            cs->k = tcp_cubic_root(delta_mss * 1024 / 51);
        } else {
            cs->k = 0;
        }
        cs->tcp_cwnd = cwnd;
        cs->w_est = cwnd;
    }

    elapsed = tcp_now - cs->epoch_start;
    target = cubic_window(tp, elapsed);
    if(target > max_cwnd)
        target = max_cwnd;

    /*
     * TCP-friendly mode (RFC 8312 §5.8):
     * alpha = 3*(1-beta)/(1+beta) ~ 0.529 → fixed-point 542/1024
     */
    cs->ack_cnt += acked;
    {
        u_long alpha_scaled = 542;
        if(cs->w_est > 0) {
            u_long w_est_inc = (alpha_scaled * mss / 1024)
                               * cs->ack_cnt / cs->w_est;
            if(w_est_inc > 0) {
                cs->w_est += w_est_inc * mss;
                cs->ack_cnt = 0;
            }
        }
    }

    if(cs->w_est > target)
        target = cs->w_est;

    /* Adjust cwnd toward target */
    if(target > cwnd) {
        u_long inc = (target - cwnd) * mss / cwnd;
        if(inc < 1)
            inc = 1;
        if(inc > mss)
            inc = mss;
        tp->snd_cwnd = MIN(cwnd + inc, max_cwnd);
    } else {
        if(cwnd < max_cwnd)
            tp->snd_cwnd = MIN(cwnd + 1, max_cwnd);
    }
}

/*
 * CUBIC loss event: 3 duplicate ACKs (fast retransmit entry).
 *
 * ssthresh = cwnd * beta (0.7) — less aggressive than NewReno's 0.5.
 * Records W_max and pre-computes K for the next epoch.
 * The caller handles setting cwnd = 1 MSS and entering fast recovery.
 */
static void
cubic_on_loss(struct tcpcb *tp)
{
    struct cubic_state *cs = &tp->t_cubic;
    u_long cwnd = tp->snd_cwnd;
    u_long mss = tp->t_maxseg;
    u_long new_ssthresh;

    if(mss == 0)
        mss = 1;

    /* Fast convergence (RFC 8312 §5.6) */
    if(cs->wmax > 0 && cwnd < cs->wmax) {
        cs->wmax = cwnd * (CUBIC_SCALE_FACTOR + CUBIC_BETA_SCALED)
                   / (2 * CUBIC_SCALE_FACTOR);
    } else {
        cs->wmax = cwnd;
    }

    new_ssthresh = cwnd * CUBIC_BETA_SCALED / CUBIC_SCALE_FACTOR;
    if(new_ssthresh < 2 * mss)
        new_ssthresh = 2 * mss;
    tp->snd_ssthresh = new_ssthresh;

    /* Reset epoch and pre-compute K */
    cs->epoch_start = 0;
    cs->flags |= CUBIC_FLAG_RESET;
    {
        u_long wmax_mss = cs->wmax / mss;
        u_long delta_mss = wmax_mss * CUBIC_ONE_MINUS_BETA_SCALED
                           / CUBIC_SCALE_FACTOR;
        cs->k = tcp_cubic_root(delta_mss * 1024 / 51);
    }
    cs->origin_point = cs->wmax;
    cs->w_est = new_ssthresh;
}

/*
 * CUBIC retransmission timeout — full cwnd reset.
 * More conservative than packet loss: saves reduced W_max,
 * uses traditional halving for ssthresh, resets cwnd to 1 MSS.
 */
static void
cubic_on_rto(struct tcpcb *tp)
{
    struct cubic_state *cs = &tp->t_cubic;
    u_long mss = tp->t_maxseg;
    u_int win;

    if(mss == 0)
        mss = 1;

    cs->wmax = tp->snd_cwnd * CUBIC_BETA_SCALED / CUBIC_SCALE_FACTOR;
    if(cs->wmax < 2 * mss)
        cs->wmax = 2 * mss;

    win = MIN(tp->snd_wnd, tp->snd_cwnd) / 2 / mss;
    if(win < 2)
        win = 2;
    tp->snd_ssthresh = win * mss;
    tp->snd_cwnd = mss;

    cs->epoch_start = 0;
    cs->origin_point = cs->wmax;
    cs->w_est = 0;
    cs->ack_cnt = 0;
    cs->flags = CUBIC_FLAG_RESET;
}

/*
 * CUBIC ECN response — treat as packet loss (beta = 0.7),
 * but set cwnd = ssthresh (no slow start restart).
 */
static void
cubic_on_ecn(struct tcpcb *tp)
{
    cubic_on_loss(tp);
    tp->snd_cwnd = tp->snd_ssthresh;
}

/*
 * CUBIC algorithm descriptor.
 */
struct tcp_cc_algo tcp_cc_cubic = {
    .name           = "cubic",
    .id             = TCP_CC_CUBIC,
    .cc_init        = cubic_init,
    .cc_on_ack      = cubic_on_ack,
    .cc_on_loss     = cubic_on_loss,
    .cc_on_rto      = cubic_on_rto,
    .cc_on_ecn      = cubic_on_ecn,
    .cc_after_idle  = cubic_after_idle,
};

/*
 * CC framework globals — shared by all algorithms.
 */
struct tcp_cc_algo *tcp_cc_algos[TCP_CC_MAX];
int tcp_cc_default = TCP_CC_CUBIC;

/*
 * Initialize the congestion control framework.
 * Register the built-in algorithms. Called from tcp_init().
 */
void
tcp_cc_init(void)
{
    tcp_cc_algos[TCP_CC_NEWRENO] = &tcp_cc_newreno;
    tcp_cc_algos[TCP_CC_CUBIC] = &tcp_cc_cubic;
}
