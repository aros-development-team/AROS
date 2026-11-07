/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * Slow start: how much an ACK grows cwnd while cwnd < ssthresh.
 *
 * By the bytes the ACK newly acknowledges, at most L * SMSS of them
 * (Appropriate Byte Counting, RFC 3465; net.inet.tcp.abc_l). L = 1, the
 * default, is RFC 5681's min(N, SMSS): 4.4BSD added one SMSS per ACK
 * whatever it acknowledged, so an ACK of a few bytes counted as a whole
 * segment. A larger L (FreeBSD uses 2) grows cwnd by up to L segments an
 * ACK when the receiver delays its ACKs; on a bottleneck bench (tests/tcp
 * in aros-matter) L = 2 overshot more - 30-50% more drops at the end of
 * slow start - for the same throughput, so it is not the default.
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

int tcp_abc_l = 1;

/* The bytes by which an ACK acknowledging `acked' new bytes grows cwnd in
 * slow start. */
u_long
tcp_ss_incr(struct tcpcb *tp, u_int acked)
{
    return MIN((u_long)acked, (u_long)tcp_abc_l * tp->t_maxseg);
}
