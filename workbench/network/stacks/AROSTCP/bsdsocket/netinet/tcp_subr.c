/*
 * Copyright (C) 1982, 1986, 1988, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2006 Pavel Fedin
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
 *	@(#)tcp_subr.c	8.1 (Berkeley) 6/10/93
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
#include <sys/queue.h>

#include <net/route.h>
#include <net/if.h>
#include <net/netmodule.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/inetfamily.h>
#include <stddef.h>		/* offsetof */
#include <protos/net/route_protos.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include "tcp_compat.h"
#include <netinet/tcp_cc.h>
#include "tcp_syncache.h"

#include <kern/amiga_includes.h>

#include <kern/kern_subr_protos.h>
#include <protos/kern/kern_random_protos.h>
#include <kern/uipc_socket2_protos.h>
#include <net/netcore_map.h>        /* always the last include */

u_int32_t tcp_secret_key[4];

/*
 * The keyed hash of a connection: SipHash-2-4 (Aumasson and Bernstein) over
 * its addresses - all 128 bits for IPv6 - its ports, a word of the
 * caller's, and a byte that keeps the uses apart (TCPH_*). The ISN (RFC
 * 6528), the SYN cookies (RFC 4987), the SYN cache buckets and the timestamp
 * offsets need a function no one can compute without the key, which RFC
 * 6528 asks for; Linux uses SipHash for them.
 */
#define SIP_ROTL(x, b)  (((x) << (b)) | ((x) >> (64 - (b))))
#define SIP_ROUND(v0, v1, v2, v3) do { \
        v0 += v1; v1 = SIP_ROTL(v1, 13); v1 ^= v0; v0 = SIP_ROTL(v0, 32); \
        v2 += v3; v3 = SIP_ROTL(v3, 16); v3 ^= v2; \
        v0 += v3; v3 = SIP_ROTL(v3, 21); v3 ^= v0; \
        v2 += v1; v1 = SIP_ROTL(v1, 17); v1 ^= v2; v2 = SIP_ROTL(v2, 32); \
    } while(0)

/* n (0..8) bytes at p as a little-endian word */
static uint64_t
sip_le64(const u_char *p, size_t n)
{
    uint64_t m = 0;

    while(n--)
        m |= (uint64_t)p[n] << (8 * n);
    return m;
}

static uint64_t
tcp_siphash(const u_int32_t *key, const u_char *p, size_t len)
{
    uint64_t k0 = key[0] | (uint64_t)key[1] << 32;
    uint64_t k1 = key[2] | (uint64_t)key[3] << 32;
    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL, v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL, v3 = k1 ^ 0x7465646279746573ULL;
    uint64_t m;
    size_t left = len;

    for(; left >= 8; p += 8, left -= 8) {
        m = sip_le64(p, 8);
        v3 ^= m;
        SIP_ROUND(v0, v1, v2, v3);
        SIP_ROUND(v0, v1, v2, v3);
        v0 ^= m;
    }
    m = sip_le64(p, left) | (uint64_t)len << 56;
    v3 ^= m;
    SIP_ROUND(v0, v1, v2, v3);
    SIP_ROUND(v0, v1, v2, v3);
    v0 ^= m;
    v2 ^= 0xff;
    SIP_ROUND(v0, v1, v2, v3);
    SIP_ROUND(v0, v1, v2, v3);
    SIP_ROUND(v0, v1, v2, v3);
    SIP_ROUND(v0, v1, v2, v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

/* laddr and faddr: addresses of alen bytes (a family's inf_addrlen) */
u_int32_t
tcp_conn_hash(const u_int32_t *key, int use, int addrlen, const void *laddr,
    const void *faddr, u_int16_t lport, u_int16_t fport, u_int32_t extra)
{
    u_char buf[1 + 2 * INP_ADDRMAX + 2 * sizeof(u_int16_t) + sizeof(u_int32_t)];
    size_t alen = addrlen, n = 0;

    buf[n++] = (u_char)use;
    bcopy(laddr, buf + n, alen);
    n += alen;
    bcopy(faddr, buf + n, alen);
    n += alen;
    bcopy(&lport, buf + n, sizeof(lport));
    n += sizeof(lport);
    bcopy(&fport, buf + n, sizeof(fport));
    n += sizeof(fport);
    bcopy(&extra, buf + n, sizeof(extra));
    n += sizeof(extra);
    return (u_int32_t)tcp_siphash(key, buf, n);
}

/* inp's address length and addresses, for tcp_conn_hash() */
#define INP_HASHADDRS(inp) (inp)->inp_family->inf_addrlen, \
        (const void *)&(inp)->inp_la, (const void *)&(inp)->inp_fa

/*
 * Generate a per-connection ISN per RFC 6528:
 *   ISN = M + F(localip, localport, remoteip, remoteport, secret_key)
 * where M is the monotonic tcp_iss clock component.
 */
tcp_seq
tcp_new_isn(struct inpcb *inp)
{
	return ((tcp_seq)tcp_iss + tcp_conn_hash(tcp_secret_key, TCPH_ISN,
	    INP_HASHADDRS(inp), inp->inp_lport, inp->inp_fport, 0));
}

/*
 * The timestamp offset of a connection (RFC 7323 section 5.4): its TSval is
 * tcp_now plus a hash of its addresses and ports (tcp_conn_hash()), so that the TSval tells
 * neither the time since the stack started nor how two connections relate,
 * as FreeBSD and Linux do. The key is not the ISN's: with the same one,
 * ISN - TSval would give the ISN clock away.
 */
u_int32_t tcp_ts_key[4];

u_int32_t
tcp_new_tsoff(struct inpcb *inp)
{
	return tcp_conn_hash(tcp_ts_key, TCPH_TSOFF, INP_HASHADDRS(inp),
	    inp->inp_lport, inp->inp_fport, 0);
}

/*
 * SYN cookie support (RFC 4987).
 *
 * Cookie = ISN encoding:
 *   bits 31..8: tcp_conn_hash(secret, addresses, ports, counter)
 *   bits  7..5: MSS index (8 common MSS values)
 *   bits  4..0: counter mod 32 (to validate freshness)
 *
 * The counter is tcp_iss / TCP_ISSINCR, which advances once a second.
 */

/* Common MSS values for encoding (3 bits = 8 entries) */
static u_int16_t tcp_sc_msstab[] = {
	216, 536, 1024, 1220, 1360, 1440, 1460, 8960
};
#define TCP_SC_MSS_ENTRIES (sizeof(tcp_sc_msstab)/sizeof(tcp_sc_msstab[0]))

int tcp_syncookies = 1;		/* SYN cookies enabled by default */

static int
tcp_sc_mss_encode(u_int16_t mss)
{
	int i, best = 0;

	for(i = 0; i < TCP_SC_MSS_ENTRIES; i++) {
		if(tcp_sc_msstab[i] <= mss)
			best = i;
	}
	return best;
}

tcp_seq
tcp_syncookie_generate(int addrlen, const void *laddr, const void *faddr,
    u_int16_t lport, u_int16_t fport, u_int16_t mss)
{
	u_int32_t counter;
	u_int32_t hash;
	int mss_idx;

	counter = (u_int32_t)(tcp_iss / TCP_ISSINCR);
	mss_idx = tcp_sc_mss_encode(mss);

	hash = tcp_conn_hash(tcp_secret_key, TCPH_COOKIE, addrlen, laddr, faddr,
	    lport, fport, counter);

	return ((hash & 0xFFFFFF00) |
	    ((mss_idx & 0x7) << 5) | (counter & 0x1F));
}

u_int16_t
tcp_syncookie_validate(int addrlen, const void *laddr, const void *faddr,
    u_int16_t lport, u_int16_t fport, tcp_seq ack_seq)
{
	tcp_seq cookie = ack_seq - 1;
	u_int32_t counter_now;
	u_int32_t counter_then;
	u_int32_t hash;
	int mss_idx;
	int diff;
	u_int32_t try_counter;
	tcp_seq expected;

	counter_now  = (u_int32_t)(tcp_iss / TCP_ISSINCR);
	counter_then = cookie & 0x1F;
	mss_idx      = (cookie >> 5) & 0x7;

	diff = (counter_now & 0x1F) - counter_then;
	if(diff < 0) diff += 32;
	if(diff > 6) /* about 6 s: the counter advances once a second */
		return 0;

	/* Try current counter epoch */
	try_counter = (counter_now & ~0x1F) | counter_then;
	hash = tcp_conn_hash(tcp_secret_key, TCPH_COOKIE, addrlen, laddr, faddr,
	    lport, fport, try_counter);
	expected = (hash & 0xFFFFFF00) |
	    ((mss_idx & 0x7) << 5) | (counter_then & 0x1F);
	if(expected == cookie)
		return tcp_sc_msstab[mss_idx];

	/* Try previous epoch in case counter wrapped */
	if(try_counter >= 32) {
		try_counter -= 32;
		hash = tcp_conn_hash(tcp_secret_key, TCPH_COOKIE, addrlen, laddr,
		    faddr, lport, fport, try_counter);
		expected = (hash & 0xFFFFFF00) |
		    ((mss_idx & 0x7) << 5) | (counter_then & 0x1F);
		if(expected == cookie)
			return tcp_sc_msstab[mss_idx];
	}

	return 0;  /* invalid cookie */
}

/* patchable/settable parameters for tcp */
/* the default MSS and the default TTL/hop limit are each family's own */
int 	tcp_rttdflt = TCPTV_RTOBASE;	/* initial RTO, s (net.inet.tcp.rttdflt) */
/* The retransmission timeout is never below this (ms; net.inet.tcp.rexmit_min).
 * RFC 6298 section 2.4 says it SHOULD be at least 1 s; FreeBSD and Linux use
 * about 200 ms, and so does this stack. */
int	tcp_rexmit_min = TCPTV_REXMTMIN_MS;
/* The least MSS a connection uses, whatever the peer offers, a route's MTU
 * or an ICMP "too big" says (TCP_MINMSS; net.inet.tcp.minmss, 0: 64 bytes):
 * a peer offering a tiny MSS would otherwise have the connection send - and
 * queue for reassembly - thousands of tiny segments, each in an mbuf or a
 * cluster (FreeBSD's tcp_minmss). */
int	tcp_minmss = TCP_MINMSS;
/*
 * Socket buffer autosizing (as FreeBSD's net.inet.tcp.sendbuf_max and
 * recvbuf_max): a buffer the application did not size (SB_AUTOSIZE) grows
 * up to these while it is what limits the connection; 0 is off. Set when
 * the stack starts from the memory free then (tcp_autobuf_init()).
 */
u_long	tcp_autosndbuf_max;
u_long	tcp_autorcvbuf_max;
int	tcp_do_rfc1323 = 1;
int	tcp_do_rfc1644 = 0;	/* T/TCP (RFC 1644) - off: Historic, RFC 6247 */
int	tcp_do_sack = 1;	/* enable SACK (RFC 2018) */
int	tcp_do_newreno = 1;	/* enable NewReno (RFC 3782) */
/* ECN (RFC 3168): 0 off, 1 ask for it and accept it, 2 accept it when the
 * peer asks (the default of Linux, FreeBSD and Windows) */
int	tcp_do_ecn = 2;
/* PRR (RFC 6937) in loss recovery with SACK: off until a measurement shows
 * it helps here; sysctl net.inet.tcp.prr */
int	tcp_do_prr = 0;
int	tcp_do_pmtud = 1;	/* IPv4 path MTU discovery: DF on (RFC 1191) */
/* path MTU black-hole detection (RFC 2923 section 2.1): on, as in XNU */
int	tcp_do_pmtud_blackhole = 1;
static	void tcp_cleartaocache(void);


/*
 * Target size of TCP PCB hash table. Will be rounded down to a prime
 * number.
 */
#ifndef TCBHASHSIZE
#define TCBHASHSIZE	128
#endif

/*
 * Autosizing follows the memory the machine has. The mbuf pool takes its
 * memory from the system as it needs it and keeps it while the stack runs
 * (uipc_mbuf.c), up to mbconf.maxmem, which is never below 4 MB however
 * little memory there is: a buffer that grows is a lasting claim on system
 * memory. So:
 * - the ceilings, set when the stack starts: off below TCP_AUTOBUF_MINFREE
 *   of free memory, else 1/TCP_AUTOBUF_DIV of it, up to TCP_AUTOBUF_MAX
 *   and to what the budget below gives one buffer (a ceiling no larger
 *   than the default buffer grows nothing);
 * - what the pool may use (tcp_autobuf_pool()): what it holds, and what it
 *   may still take from the system without leaving less than
 *   TCP_AUTOBUF_MINFREE free, within mbconf.maxmem - free memory read when
 *   a buffer is to grow, not only once;
 * - the budget: what autosizing has given all live connections stays
 *   within 3/8 of that (a byte of buffer counts as two of the pool,
 *   sb_mbmax: a 1448-byte segment sits in a 2 KB cluster and an mbuf), and
 *   goes back at tcp_close() or in TIME_WAIT (tcp_autobuf_release()).
 */
#define	TCP_AUTOBUF_MINFREE	(16UL * 1024 * 1024)
#define	TCP_AUTOBUF_DIV		64
#define	TCP_AUTOBUF_MAX		(1024UL * 1024)

u_long	tcp_autobuf_used;

void
tcp_autobuf_init()
{
    u_long avail = AvailMem(MEMF_ANY);
    u_long c = 0;

    if(avail >= TCP_AUTOBUF_MINFREE)
        c = MIN(MIN(avail / TCP_AUTOBUF_DIV, TCP_AUTOBUF_MAX), tcp_autobuf_one())
            & ~(TCP_AUTOSNDBUF_INC - 1);
    tcp_autosndbuf_max = tcp_autorcvbuf_max = c;
}

u_long
tcp_autobuf_pool()
{
    u_long avail = AvailMem(MEMF_ANY);
    u_long spare = avail > TCP_AUTOBUF_MINFREE ? avail - TCP_AUTOBUF_MINFREE : 0;

    return MIN((u_long)mbconfig.maxmem * 1024, mbstats.m_memused + spare);
}

/* the most one buffer may grow to: the whole budget, at two bytes of the
 * pool per byte (also the most the sysctls take) */
u_long
tcp_autobuf_one()
{
    return tcp_autobuf_pool() / 8 * 3 / 2;
}

/*
 * Grow an autosized buffer (SB_AUTOSIZE) of a connection towards want
 * bytes, no more than max (its ceiling). The pool is shared by every
 * connection and by the interfaces' receive buffers, and runs out for all
 * of them at once (mbstat.m_drops): a buffer grows by what is left of the
 * budget, if less than asked, and nothing grows while half of what the
 * pool may use is in clusters in use. A buffer sbreserve_max() refuses
 * stops growing. 1 if it grew.
 */
int
tcp_autobuf_grow(tp, sb, want, max)
struct tcpcb *tp;
struct sockbuf *sb;
u_long want;
u_long max;
{
    u_long old = sb->sb_hiwat;
    u_long pool, cap, inuse;

    want = MIN(want, max);
    if(want <= old)
        return 0;
    pool = tcp_autobuf_pool();
    cap = pool / 8 * 3;
    inuse = (mbstats.m_clusters - mbstats.m_clfree) * mbconfig.mclbytes;
    if(inuse >= pool / 2 || tcp_autobuf_used >= cap)
        return 0;
    if((want - old) * 2 > cap - tcp_autobuf_used)
        want = old + (cap - tcp_autobuf_used) / 2;
    if(want - old < mbconfig.mclbytes)
        return 0;
    /* max bounds the buffer, as sb_max does SO_SNDBUF/SO_RCVBUF; its mbuf
     * storage (sb_mbmax) may be twice the bytes */
    if(!sbreserve_max(sb, want, 2 * max)) {
        sb->sb_flags &= ~SB_AUTOSIZE;
        return 0;
    }
    tp->t_autobuf += 2 * (sb->sb_hiwat - old);
    tcp_autobuf_used += 2 * (sb->sb_hiwat - old);
    return 1;
}

/*
 * Give a connection's share of the budget back: at tcp_close(), and when
 * it enters TIME_WAIT, where its buffers hold nothing and never will (a
 * connection that closed first waits there 2 MSL, a minute; holding its
 * share, a few of them kept every new connection from growing).
 */
void
tcp_autobuf_release(tp)
struct tcpcb *tp;
{
    tcp_autobuf_used -= tp->t_autobuf;
    tp->t_autobuf = 0;
}

/*
 * The window scale to ask for (RFC 7323 section 2.2): enough for the
 * receive buffer, or for what autosizing may grow it to (with autosizing
 * off, a ceiling of 0, the buffer alone); the scale is fixed at the
 * handshake.
 */
int
tcp_rscale(so)
struct socket *so;
{
    u_long want = so->so_rcv.sb_hiwat;
    int scale = 0;

    if((so->so_rcv.sb_flags & SB_AUTOSIZE) && tcp_autorcvbuf_max > want)
        want = tcp_autorcvbuf_max;
    while(scale < TCP_MAX_WINSHIFT && ((u_long)TCP_MAXWIN << scale) < want)
        scale++;
    return scale;
}

/*
 * The transports' part in the life of an interface (net/netmodule.h): no
 * TCP or UDP socket of either family may go on naming one that is gone.
 */
void tcp_ifgone(struct ifnet *ifp);

static int
transport_ifevent(int event, struct ifnet *ifp)
{
    if(event == NFE_DETACHED) {
        in_pcbifgone(&udb, ifp);
        rip_ifgone(ifp);        /* raw sockets of every family */
        tcp_ifgone(ifp);
    }
    return (0);
}

static const struct netfamily_ifops transport_ifops = { AF_UNSPEC, 30, transport_ifevent, NULL };

/*
 * Tcp initialization
 */
void
tcp_init()
{
    /* first: what the configuration set for a variable is given to it */
    tcp_vars_register();
    (void)netfamily_ifops_register(&transport_ifops);

    /*
     * The ISN secret key (RFC 6528), also the SYN cookies' (tcp_syncookie_*),
     * and the start of the ISN clock, from arc4random_buf() (kern_random.c).
     */
    kern_random(tcp_secret_key, sizeof(tcp_secret_key));
    kern_random(tcp_ts_key, sizeof(tcp_ts_key));        /* RFC 7323 5.4 */
    tcp_iss = (tcp_seq)kern_random32();
    tcp_ccgen = 1;
    tcp_cc_init();
    tcp_cleartaocache();
    LIST_INIT(&tcb);
    tcbinfo.listhead = &tcb;
    tcbinfo.hashbase = phashinit(TCBHASHSIZE, M_PCB, &tcbinfo.hashsize);
    syncache_init();
    tcp_autobuf_init();
    tcp_rack_init();
    /* room for a TCP header behind the largest network header */
    if(max_protohdr < TCP_NETHDR_MAX + sizeof(struct tcphdr))
        max_protohdr = TCP_NETHDR_MAX + sizeof(struct tcphdr);
    if(max_linkhdr + TCP_NETHDR_MAX + sizeof(struct tcphdr) + TCP_MAXOLEN > MHLEN)
        panic("tcp_init");
}

/*
 * TCP stops: the reverse of tcp_init(), called by the transport module's
 * stop (netinet/inet_transport.c) after the transports' domain is detached
 * - no timer of TCP's runs any more - and with no socket, PCB or bound
 * family left (transport_can_stop()).
 *
 * In this order: the registrations (the interface events, the variables -
 * first, while the variables still hold what the core is to keep of them);
 * the SYN cache's entries and table; the PCB hash table; the trace buffer
 * if one was allocated; then every variable of TCP's files back to what
 * the file gives it, so that the next start - of this copy of the code or
 * of one loaded anew - begins the same. max_protohdr, the core's, stays
 * as tcp_init() raised it: other protocols may count on it by now.
 */
void
tcp_deinit()
{
    (void)netfamily_ifops_unregister(&transport_ifops);
    tcp_vars_unregister();

    syncache_deinit();
    if(tcbinfo.hashbase != NULL)
        bsd_free((caddr_t)tcbinfo.hashbase, M_PCB);
    tcp_debug_free();

    tcp_input_defaults();       /* tcb, tcbinfo, tcpstat, the clocks */
    tcp_timer_defaults();
    tcp_cc_defaults();
    tcp_rack_defaults();
    tcp_ss_defaults();
    /* this file's */
    tcp_syncookies = 1;
    tcp_rttdflt = TCPTV_RTOBASE;
    tcp_rexmit_min = TCPTV_REXMTMIN_MS;
    tcp_minmss = TCP_MINMSS;
    tcp_autosndbuf_max = tcp_autorcvbuf_max = 0;
    tcp_do_rfc1323 = 1;
    tcp_do_rfc1644 = 0;
    tcp_do_sack = 1;
    tcp_do_newreno = 1;
    tcp_do_ecn = 2;
    tcp_do_prr = 0;
    tcp_do_pmtud = 1;
    tcp_do_pmtud_blackhole = 1;
    tcp_autobuf_used = 0;
    bzero((caddr_t)tcp_secret_key, sizeof(tcp_secret_key));
    bzero((caddr_t)tcp_ts_key, sizeof(tcp_ts_key));
}

/*
 * Create template to be used to send tcp packets on a connection.
 * Call after host entry created, allocates an mbuf and fills
 * in a skeletal tcp header, minimizing the amount of work
 * necessary when the connection is used. The addresses are the PCB's and
 * go into the network header when a segment is sent (inf_output).
 */
struct tcphdr *
tcp_template(tp)
struct tcpcb *tp;
{
    register struct inpcb *inp = tp->t_inpcb;
    register tcp_ahdr_t *n;      /* in an mbuf: even-aligned */

    if((n = (tcp_ahdr_t *)tp->t_template) == 0) {
        register struct mbuf *m;
        m = m_get(M_DONTWAIT, MT_HEADER);
        if(m == NULL)
            return (0);
        m->m_len = sizeof(struct tcphdr);
        n = mtod(m, tcp_ahdr_t *);
    }
    n->th_sport = inp->inp_lport;
    n->th_dport = inp->inp_fport;
    n->th_seq = 0;
    n->th_ack = 0;
    n->th_x2 = 0;
    n->th_off = 5;
    n->th_flags = 0;
    n->th_win = 0;
    n->th_sum = 0;
    n->th_urp = 0;
    return ((struct tcphdr *)n);
}

/*
 * Send a single message to the TCP at address faddr, from laddr
 * (addresses of family inf). If m == 0, then we make a copy of the
 * tcphdr at th and send it as it is: this is used to force keep alive
 * messages out using the TCP template for a connection tp->t_template.
 * Else m is a segment received, th its TCP header in it and laddr/faddr
 * its destination and source: we send a message with the given flags back
 * to the TCP which originated it (its ports swapped), and discard the mbuf
 * containing it and any other attached mbufs.
 *
 * In any case the ack and sequence number of the transmitted
 * segment are as specified by the parameters.
 */
void
tcp_respond(tp, inf, laddr, faddr, th, m, ack, seq, flags)
struct tcpcb *tp;
const struct inetfamily *inf;
const void *laddr, *faddr;
register tcp_ahdr_t *th;
register struct mbuf *m;
tcp_seq ack, seq;
int flags;
{
    register int tlen;
    int win = 0;
    int oflags = INF_OUT_BARE;

    if(tp)
        win = sbspace(&tp->t_inpcb->inp_socket->so_rcv);
    if(m == 0) {
        m = m_gethdr(M_DONTWAIT, MT_HEADER);
        if(m == NULL)
            return;
#ifdef TCP_COMPAT_42
        tlen = 1;
#else
        tlen = 0;
#endif
        m->m_data += max_linkhdr + inf->inf_hdrlen;
        *mtod(m, tcp_ahdr_t *) = *th;
        th = mtod(m, tcp_ahdr_t *);
        flags = TH_ACK;
    } else {
        m_freem(m->m_next);
        m->m_next = 0;
        m->m_data = (caddr_t)th;
        tlen = 0;
        /*
         * Swap the ports to bounce the segment back to its origin. The
         * network header it came with lies in front of th (tcp_input()
         * keeps the two together): the answer's goes there.
         */
        oflags |= INF_OUT_ROOM;
#define xchg(a,b,type) { type t; t=a; a=b; b=t; }
        xchg(th->th_dport, th->th_sport, u_short);
#undef xchg
    }
    tlen += sizeof(struct tcphdr);
    m->m_len = tlen;
    m->m_pkthdr.len = tlen;
    m->m_pkthdr.rcvif = (struct ifnet *) 0;
    th->th_seq = htonl(seq);
    th->th_ack = htonl(ack);
    th->th_x2 = 0;
    th->th_off = sizeof(struct tcphdr) >> 2;
    th->th_flags = flags;
    /*
     * Clamp the advertised window to the 16-bit field.  The window is only
     * scaled on an established connection (rcv_scale != 0); on a listen
     * socket rcv_scale is 0, and with large socket buffers the raw value
     * would otherwise overflow the u_short field (e.g. 128KB -> 0).
     */
    if(tp)
        win >>= tp->rcv_scale;
    if(win > TCP_MAXWIN)
        win = TCP_MAXWIN;
    else if(win < 0)
        win = 0;
    th->th_win = htons((u_short)win);
    th->th_urp = 0;
    /* the family fills in the checksum and its header and sends: over the
     * connection's route when there is one, with its own defaults else */
    (void)(*inf->inf_output)(m, tp ? tp->t_inpcb : NULL, laddr, faddr, IPPROTO_TCP,
                             offsetof(struct tcphdr, th_sum), 0, 0, oflags);
}

/*
 * Create a new TCP control block, making an
 * empty reassembly queue and hooking it to the argument
 * protocol control block.
 */
struct tcpcb *
tcp_newtcpcb(inp)
struct inpcb *inp;
{
    register struct tcpcb *tp;

    tp = bsd_malloc(sizeof(*tp), M_PCB, M_NOWAIT);
    if(tp == NULL)
        return ((struct tcpcb *)0);
    bzero((char *) tp, sizeof(struct tcpcb));
    tp->t_segq = NULL;
    tp->t_maxseg = tp->t_maxopd = *inp->inp_family->inf_mssdflt;

    if(tcp_do_rfc1323)
        tp->t_flags = (TF_REQ_SCALE | TF_REQ_TSTMP);
    if(tcp_do_rfc1644)
        tp->t_flags |= TF_REQ_CC;
    tp->t_flagsext = 0;
    if(tcp_do_newreno)
        tp->t_flagsext |= TF_NEWRENO;
    if(tcp_do_sack)
        tp->t_flags |= TF_SACK_PERMIT;	/* request SACK in SYN */
    tp->t_num_sack_blks = 0;
    tp->t_num_dsack_blks = 0;
    tp->snd_fack = 0;
    tp->snd_awnd = 0;
    /* PRR / SACK-recovery state (RFC 6937 / RFC 6675); bzero'd above. */
    tp->snd_sacked = 0;
    tp->snd_prr_out = 0;
    tp->snd_prr_delivered = 0;
    tp->snd_prr_recover_fs = 0;
    tp->snd_prr_hiack = 0;
    tp->snd_ecn_recover = 0;
    tp->t_inpcb = inp;
    /*
     * No RTT estimate yet (srtt 0): the RTO is tcp_rttdflt, 1 s (RFC 6298
     * section 2.1), until the first measurement. rttvar is seeded so that
     * TCP_REXMTVAL() gives it, as in 4.4BSD and FreeBSD: a SYN that times
     * out is then resent after 1, 2, 4 s... (section 5.5), not after the
     * 200 ms floor.
     */
    tp->t_srtt = TCPTV_SRTTBASE;
    tp->t_rttvar = (tcp_rttdflt * 1000 << TCP_RTTVAR_SHIFT) / 4;
    tp->t_rttmin = tcp_rexmit_min;
    TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
                  tp->t_rttmin, TCPTV_REXMTMAX_MS);
    tp->snd_cwnd = TCP_MAXWIN << TCP_MAX_WINSHIFT;
    tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
    /* Initialize congestion control algorithm */
    tp->t_cc_algo = tcp_cc_default;
    CC_INIT(tp);
    tcp_hystart_init(tp);
    inp->inp_ppcb = (caddr_t)tp;
    return (tp);
}

/*
 * tcp_ifgone - interface ifp is going away. A connection bound to it (an
 * IPv6 zone, SO_BINDTODEVICE) has lost its link: it is closed with ENXIO,
 * without a reset, which could only leave by another link. A listening
 * socket bound to it hears nothing more (in_pcbifgone()).
 */
void
tcp_ifgone(struct ifnet *ifp)
{
    struct inpcb *inp, *next;

    for(inp = tcb.lh_first; inp != NULL; inp = next) {
        struct tcpcb *tp = intotcpcb(inp);

        next = inp->inp_list.le_next;
        if(inp->inp_boundif != ifp || tp == NULL || tp->t_state == TCPS_LISTEN)
            continue;
        inp->inp_socket->so_error = ENXIO;
        tcp_close(tp);
    }
    in_pcbifgone(&tcb, ifp);
}

/*
 * Drop a TCP connection, reporting
 * the specified error.  If connection is synchronized,
 * then send a RST to peer.
 */
struct tcpcb *
tcp_drop(tp, _errno)
register struct tcpcb *tp;
int _errno;
{
    struct socket *so = tp->t_inpcb->inp_socket;

    if(TCPS_HAVERCVDSYN(tp->t_state)) {
        tp->t_state = TCPS_CLOSED;
        (void) tcp_output(tp);
        tcpstat.tcps_drops++;
    } else
        tcpstat.tcps_conndrops++;
    if(_errno == ETIMEDOUT && tp->t_softerror)
        _errno = tp->t_softerror;
    so->so_error = _errno;
    return (tcp_close(tp));
}

/*
 * Is rt the default route? Its key is then all zero behind the family (the
 * address, and the fields around it that a routing key leaves zero).
 */
static int
tcp_rt_isdefault(const struct rtentry *rt)
{
    const struct sockaddr *sa = rt_key(rt);
    const u_char *p = (const u_char *)sa->sa_data;
    const u_char *e = (const u_char *)sa + sa->sa_len;

    while(p < e)
        if(*p++)
            return (0);
    return (1);
}

/*
 * Close a TCP control block:
 *	discard all space held by the tcp
 *	discard internet protocol block
 *	wake up any sleepers
 */
struct tcpcb *
tcp_close(tp)
register struct tcpcb *tp;
{
    struct inpcb *inp = tp->t_inpcb;
    struct socket *so = inp->inp_socket;
    register struct mbuf *q;
    register struct mbuf *nq;
#ifdef RTV_RTT
    register struct rtentry *rt;

    /*
     * If we sent enough data to get some meaningful characteristics,
     * save them in the routing entry.  'Enough' is arbitrarily
     * defined as the sendpipesize (default 4K) * 16.  This would
     * give us 16 rtt samples assuming we only get one sample per
     * window (the usual case on a long haul net).  16 samples is
     * enough for the srtt filter to converge to within 5% of the correct
     * value; fewer samples and we could save a very bogus rtt.
     *
     * Don't update the default route's characteristics and don't
     * update anything that the user "locked".
     */
    if(SEQ_LT(tp->iss + so->so_snd.sb_hiwat * 16, tp->snd_max) &&
            (rt = inp->inp_route.ro_rt) && !tcp_rt_isdefault(rt)) {
        register u_long i = 0;

        if((rt->rt_rmx.rmx_locks & RTV_RTT) == 0) {
            i = ((u_long)tp->t_srtt * (RTM_RTTUNIT / 1000)) >> TCP_RTT_SHIFT;
            if(rt->rt_rmx.rmx_rtt && i)
                /*
                 * filter this update to half the old & half
                 * the new values, converting scale.
                 * See route.h and tcp_var.h for a
                 * description of the scaling constants.
                 */
                rt->rt_rmx.rmx_rtt =
                    (rt->rt_rmx.rmx_rtt + i) / 2;
            else
                rt->rt_rmx.rmx_rtt = i;
        }
        if((rt->rt_rmx.rmx_locks & RTV_RTTVAR) == 0) {
            i = ((u_long)tp->t_rttvar * (RTM_RTTUNIT / 1000)) >> TCP_RTTVAR_SHIFT;
            if(rt->rt_rmx.rmx_rttvar && i)
                rt->rt_rmx.rmx_rttvar =
                    (rt->rt_rmx.rmx_rttvar + i) / 2;
            else
                rt->rt_rmx.rmx_rttvar = i;
        }
        /*
         * update the pipelimit (ssthresh) if it has been updated
         * already or if a pipesize was specified & the threshhold
         * got below half the pipesize.  I.e., wait for bad news
         * before we start updating, then update on both good
         * and bad news.
         */
        if(((rt->rt_rmx.rmx_locks & RTV_SSTHRESH) == 0 &&
                ((i = tp->snd_ssthresh) != 0) && rt->rt_rmx.rmx_ssthresh) ||
                i < (rt->rt_rmx.rmx_sendpipe / 2)) {
            /*
             * convert the limit from user data bytes to
             * packets then to packet data bytes.
             */
            i = (i + tp->t_maxseg / 2) / tp->t_maxseg;
            if(i < 2)
                i = 2;
            i *= (u_long)(tp->t_maxseg + inp->inp_family->inf_hdrlen +
                          sizeof(struct tcphdr));
            if(rt->rt_rmx.rmx_ssthresh)
                rt->rt_rmx.rmx_ssthresh =
                    (rt->rt_rmx.rmx_ssthresh + i) / 2;
            else
                rt->rt_rmx.rmx_ssthresh = i;
        }
    }
#endif /* RTV_RTT */
    /* Drop any half-open syncache entries for a closing listen socket. */
    if(so->so_options & SO_ACCEPTCONN)
        syncache_purge(so);
    /* free the reassembly queue, if any */
    for(q = tp->t_segq; q; q = nq) {
        nq = q->m_nextpkt;
        tp->t_segq = nq;
        m_freem(q);
    }
    tcp_rack_detach(tp);                /* RACK's map, if any */
    if(tp->t_template)
        (void) m_free(dtom(tp->t_template));
    tcp_autobuf_release(tp);
    bsd_free(tp, M_PCB);
    inp->inp_ppcb = 0;
    soisdisconnected(so);
    in_pcbdetach(inp);
    tcpstat.tcps_closed++;
    return ((struct tcpcb *)0);
}

void
tcp_drain()
{

}

/*
 * Notify a tcp user of an asynchronous error;
 * store error as soft error, but wake up user
 * (for now, won't do anything until can select for soft error).
 */
void
tcp_notify(inp, error)
struct inpcb *inp;
int error;
{
    register struct tcpcb *tp = (struct tcpcb *)inp->inp_ppcb;
    register struct socket *so = inp->inp_socket;

    /*
     * Ignore some errors if we are hooked up.
     * If connection hasn't completed, has retransmitted several times,
     * and receives a second error, give up now.  This is better
     * than waiting a long time to establish a connection that
     * can never complete.
     */
    if(tp->t_state == TCPS_ESTABLISHED &&
            (error == EHOSTUNREACH || error == ENETUNREACH ||
             error == EHOSTDOWN)) {
        return;
    } else if(tp->t_state < TCPS_ESTABLISHED && tp->t_rxtshift > 3 &&
              tp->t_softerror)
        so->so_error = error;
    else
        tp->t_softerror = error;
    wakeup((caddr_t) &so->so_timeo);
    sorwakeup(so);
    sowwakeup(so);
}

void tcp_mtudisc(struct tcpcb *, u_int32_t);

/*
 * tcp_ctlnotify - what a family's ICMP says about a segment we sent (struct
 * inctlparam, netinet/inetfamily.h). tcp_notify() keeps an error as a soft
 * error. A "fragmentation needed" / Packet Too Big (PRC_MSGSIZE; RFC 1191,
 * RFC 8201) about a segment of a connection under way, the path MTU with
 * it: only for a segment still unacknowledged - anything else is old, or
 * forged blind (RFC 5927 section 7.2) - the connection sends smaller ones
 * (tcp_mtudisc()).
 */
void
tcp_ctlnotify(int cmd, const struct inctlparam *cp)
{
    const struct inetfamily *inf = cp->icp_family;

    if((unsigned)cmd >= PRC_NCMDS)
        return;
    if(cmd == PRC_MSGSIZE) {
        struct inpcb *inp;
        struct tcpcb *tp;

        if(!cp->icp_haveseq || cp->icp_laddr == NULL)
            return;
        /* the connection itself: a listening socket sent no segment */
        inp = inpcb_lookup(&tcbinfo, inf, cp->icp_faddr, cp->icp_fport,
                           cp->icp_laddr, cp->icp_lport,
                           (const struct ifnet *)NULL, 0);
        if(inp == NULL || (tp = intotcpcb(inp)) == NULL)
            return;
        if(SEQ_LT(cp->icp_seq, tp->snd_una) || SEQ_GEQ(cp->icp_seq, tp->snd_max))
            return;
        tcp_mtudisc(tp, cp->icp_mtu);
        return;
    }
    /* only what is an error for the connection, a source quench, or a
     * redirect (its route) */
    if(cmd != PRC_QUENCH && !PRC_IS_REDIRECT(cmd) && inf->inf_ctlerrmap[cmd] == 0)
        return;
    inpcb_notify(&tcb, inf, cp->icp_faddr, cp->icp_fport, cp->icp_laddr,
                 cp->icp_lport, cmd, cmd == PRC_QUENCH ? tcp_quench : tcp_notify);
}

/*
 * tcp_setmtu - send segments of at most mtu bytes, headers included, to tp's
 * peer: t_maxopd and t_maxseg as tcp_mss() sets them. Only ever lowers them;
 * returns whether it did. With mtu 0, the MTU is the connection's route's.
 */
static int
tcp_setmtu(struct tcpcb *tp, u_int32_t mtu)
{
    const struct inetfamily *inf = tp->t_inpcb->inp_family;
    int hdrs = inf->inf_hdrlen + sizeof(struct tcphdr), mss;

    if(mtu == 0) {
        struct rtentry *rt = tp->t_inpcb->inp_route.ro_rt;

        if(rt == NULL)
            return 0;
        mtu = rt->rt_rmx.rmx_mtu ? rt->rt_rmx.rmx_mtu :
              rt->rt_ifp ? rt->rt_ifp->if_mtu : 0;
    }
    mss = (int)mtu - hdrs;
    if(mss < TCP_MSS_FLOOR) {
        /* never below the floor: where routers fragment (INF_HASDF), the
         * segments go without DF and are fragmented on the way, as
         * FreeBSD's tcp_mtudisc() */
        mss = TCP_MSS_FLOOR;
        if(inf->inf_flags & INF_HASDF)
            tp->t_flagsext |= TF_PMTUD_NODF;
    }

    if(mss >= tp->t_maxopd)
        return 0;
    tp->t_maxopd = mss;
    if((tp->t_flags & (TF_REQ_TSTMP | TF_NOOPT)) == TF_REQ_TSTMP &&
            (tp->t_flags & TF_RCVD_TSTMP) == TF_RCVD_TSTMP)
        mss -= TCPOLEN_TSTAMP_APPA;
    if(mss >= tp->t_maxseg)
        return 0;
    tp->t_maxseg = mss;
    return 1;
}

/*
 * tcp_mtudisc - the path to tp's peer takes packets of at most mtu bytes
 * (RFC 1191, RFC 8201 section 5.2): send segments that fit, and resend
 * what was not acknowledged, as FreeBSD's tcp_mtudisc() does. With mtu 0,
 * the MTU is the connection's route's (after the family's output said
 * EMSGSIZE).
 */
void
tcp_mtudisc(struct tcpcb *tp, u_int32_t mtu)
{
    if(!tcp_setmtu(tp, mtu))
        return;
    tp->t_rtttime = 0;              /* do not time the resent segment */
    tp->snd_nxt = tp->snd_una;
    (void)tcp_output(tp);
}

/*
 * tcp_pmtud_blackhole - called by the retransmission timer, t_rxtshift just
 * raised. A path that drops large packets without telling us (ICMP
 * filtered: RFC 2923 section 2.1) loses every full-sized segment, so after
 * the 2nd timeout in a row with one unacknowledged, try segments that fit
 * the family's fall-back MTU (inf_bhmtu: the IPv6 minimum MTU, 1280; for
 * IPv4, 1240 bytes); after the 4th, a connection of a family with a DF bit
 * stops setting it, so routers fragment again. After the
 * 6th, it was not the path: the MSS and DF come back. A lowered MSS that
 * gets through is kept, as a learned path MTU is. FreeBSD's
 * tcp_timer_rexmt() does the same, with the same steps.
 */
void
tcp_pmtud_blackhole(struct tcpcb *tp)
{
    const struct inetfamily *inf = tp->t_inpcb->inp_family;
    u_int32_t mtu = inf->inf_bhmtu;

    if(!tcp_do_pmtud_blackhole ||
            (tp->t_state != TCPS_ESTABLISHED && tp->t_state != TCPS_CLOSE_WAIT))
        return;
    if((inf->inf_flags & INF_HASDF) && !tcp_do_pmtud)   /* no DF: routers fragment */
        return;

    if(tp->t_rxtshift >= 6) {
        if(tp->t_flagsext & TF_PMTUD_BH) {
            tp->t_maxseg = tp->t_pmtud_saved_maxseg;
            tp->t_maxopd = tp->t_pmtud_saved_maxopd;
            tp->t_flagsext &= ~(TF_PMTUD_BH | TF_PMTUD_NODF);
        }
        return;
    }
    if(tp->t_rxtshift < 2 || (tp->t_rxtshift & 1) ||
            (u_long)(tp->snd_max - tp->snd_una) < tp->t_maxseg)
        return;

    if(!(tp->t_flagsext & TF_PMTUD_BH)) {
        tp->t_pmtud_saved_maxseg = tp->t_maxseg;
        tp->t_pmtud_saved_maxopd = tp->t_maxopd;
    }
    if(tcp_setmtu(tp, mtu))
        tp->t_flagsext |= TF_PMTUD_BH;
    else if(tp->t_flagsext & TF_PMTUD_BH)
        tp->t_flagsext |= TF_PMTUD_NODF;    /* only read where there is a DF bit */
}

/*
 * When a source quench is received, close congestion window
 * to one segment.  We will gradually open it again as we proceed.
 */
void
tcp_quench(inp, _errno)
struct inpcb *inp;
int _errno;
{
    struct tcpcb *tp = intotcpcb(inp);

    if(tp) {
        CC_ON_RTO(tp);
    }
}

/*
 * Return a pointer to the cached information about the remote host.
 * The cached information is stored in the protocol specific part of
 * the route metrics.
 */
struct rmxp_tao *
tcp_gettaocache(inp)
struct inpcb *inp;
{
    const struct inetfamily *inf = inp->inp_family;
    struct rtentry *rt;

    /* the cache is in the route a PCB keeps: a family whose PCBs keep none
     * (INF_RTREF) has no cache */
    if(inf->inf_flags & INF_RTREF)
        return NULL;
    rt = (*inf->inf_pcbroute)(inp);

    /* Make sure this is a host route and is up. */
    if(rt == NULL ||
            (rt->rt_flags & (RTF_UP | RTF_HOST)) != (RTF_UP | RTF_HOST))
        return NULL;

    return rmx_taop(rt->rt_rmx);
}

/*
 * Clear all the TAO cache entries, called from tcp_init.
 *
 * XXX
 * This routine is just an empty one, because we assume that the routing
 * routing tables are initialized at the same time when TCP, so there is
 * nothing in the cache left over.
 */
static void
tcp_cleartaocache(void)
{ }
