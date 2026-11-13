/*
 * Copyright (C) 2025 The AROS Dev Team
 *
 * Pluggable congestion control framework for AROSTCP.
 *
 * Provides an abstract interface for TCP congestion control algorithms,
 * with two built-in implementations:
 *   - NewReno (RFC 3782): traditional AIMD, conservative fallback
 *   - CUBIC (RFC 8312): modern default for high-BDP networks
 *
 * Each algorithm provides callbacks for the key CC events:
 *   - init:       initialize per-connection CC state
 *   - on_ack:     non-duplicate ACK received (cwnd growth)
 *   - on_loss:    packet loss detected (3 dup ACKs / fast retransmit)
 *   - on_rto:     retransmission timeout (full cwnd reset)
 *   - on_ecn:     ECN congestion signal received
 *   - after_idle: connection resumes after idle period
 */

#ifndef _NETINET_TCP_CC_H_
#define _NETINET_TCP_CC_H_

struct tcpcb;   /* forward declaration */

/*
 * TCP_CC_NEWRENO and TCP_CC_CUBIC are defined in <netinet/tcp_var.h>.
 * TCP_CC_MAX is the total number of built-in algorithms.
 */
#define TCP_CC_MAX      2

/*
 * Congestion control algorithm operations.
 *
 * Each algorithm registers one of these structs. The TCP stack
 * calls through the function pointers at the appropriate events.
 * All callbacks receive the tcpcb and may modify snd_cwnd/snd_ssthresh.
 */
struct tcp_cc_algo {
    const char  *name;          /* algorithm name (for diagnostics) */
    int         id;             /* TCP_CC_NEWRENO, TCP_CC_CUBIC, etc. */

    /* Initialize per-connection CC state (called from tcp_newtcpcb) */
    void        (*cc_init)(struct tcpcb *tp);

    /*
     * ACK received — grow congestion window.
     * Called for each non-duplicate ACK when not in fast recovery.
     * 'acked' is the number of newly acknowledged bytes.
     */
    void        (*cc_on_ack)(struct tcpcb *tp, u_int acked);

    /*
     * Packet loss — multiplicative decrease.
     * Called on 3 duplicate ACKs (fast retransmit entry).
     * Must set snd_ssthresh. Caller will set snd_cwnd = 1 MSS
     * and enter fast recovery.
     */
    void        (*cc_on_loss)(struct tcpcb *tp);

    /*
     * Retransmission timeout — full cwnd reset.
     * Called when the retransmit timer expires.
     * Must set both snd_ssthresh and snd_cwnd.
     */
    void        (*cc_on_rto)(struct tcpcb *tp);

    /*
     * ECN congestion experienced.
     * Called when peer sends ECE flag. Treated as a congestion signal
     * similar to loss. Must set snd_ssthresh and snd_cwnd.
     */
    void        (*cc_on_ecn)(struct tcpcb *tp);

    /*
     * Connection resumes after idle period.
     * Called when the connection has been idle for >= 1 RTO.
     * May reset epoch state for time-based algorithms.
     */
    void        (*cc_after_idle)(struct tcpcb *tp);
};

/*
 * Algorithm registry — indexed by TCP_CC_* identifier.
 */
extern struct tcp_cc_algo *tcp_cc_algos[TCP_CC_MAX];

/*
 * Global sysctl: default CC algorithm for new connections.
 * 0 = NewReno, 1 = CUBIC (default).
 */
extern int tcp_cc_default;

/*
 * Per-algorithm structs (defined in tcp_cc_newreno.c / tcp_cc_cubic.c).
 */
extern struct tcp_cc_algo tcp_cc_newreno;
extern struct tcp_cc_algo tcp_cc_cubic;

/*
 * Convenience macros to call through the CC algorithm for a connection.
 * Falls back to NewReno if the algo pointer is somehow NULL.
 */
#define CC_ALGO(tp) \
    (tcp_cc_algos[(tp)->t_cc_algo] ? tcp_cc_algos[(tp)->t_cc_algo] : &tcp_cc_newreno)

#define CC_INIT(tp)             CC_ALGO(tp)->cc_init(tp)
#define CC_ON_ACK(tp, acked)    CC_ALGO(tp)->cc_on_ack(tp, acked)
#define CC_ON_LOSS(tp)          CC_ALGO(tp)->cc_on_loss(tp)
#define CC_ON_RTO(tp)           CC_ALGO(tp)->cc_on_rto(tp)
#define CC_ON_ECN(tp)           CC_ALGO(tp)->cc_on_ecn(tp)
#define CC_AFTER_IDLE(tp)       CC_ALGO(tp)->cc_after_idle(tp)

/*
 * Initialize the CC framework (register built-in algorithms).
 * Called from tcp_init().
 */
void    tcp_cc_init(void);

/*
 * CUBIC-specific constants (RFC 8312 Section 5).
 *
 * C = 0.4 (scaling factor)
 * beta_cubic = 0.7 (multiplicative decrease factor)
 *
 * For integer arithmetic we use fixed-point scaling (1024x).
 */
#define CUBIC_C_SCALED              410     /* C * 1024 = 0.4 * 1024 */
#define CUBIC_BETA_SCALED           717     /* beta * 1024 = 0.7 * 1024 */
#define CUBIC_ONE_MINUS_BETA_SCALED 307     /* (1-beta) * 1024 */
#define CUBIC_SCALE_FACTOR          1024

/*
 * Integer cube root: returns floor(cbrt(x)).
 * Used by CUBIC to compute K = cbrt(W_max * (1-beta) / C).
 */
u_long  tcp_cubic_root(u_long x);

#endif /* _NETINET_TCP_CC_H_ */

