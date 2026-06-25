#ifndef NETINET_TCP_COMPAT_H
#define NETINET_TCP_COMPAT_H

/* Legacy definitions used by AROSTCP - code using these needs updated */

/* increment for tcp_iss each second */
#define	TCP_ISSINCR	(125*1024)

/* timestamp wrap-around time */
#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

extern int	tcp_iss;		/* tcp initial send seq # */
extern int	tcp_ccgen;		/* global connection count */

/* Macro to increment a CC: skip 0 which has a special meaning */
#define CC_INC(c)	(++(c) == 0 ? ++(c) : (c))

/*
 * RFC 6528: Generate a cryptographic per-connection ISN.
 * ISN = tcp_iss (monotonic clock) + hash(local, remote, secret)
 */
struct inpcb;
tcp_seq	tcp_new_isn(struct inpcb *inp);

/*
 * ISN hash primitive (Murmur-style keyed hash).
 */
u_int32_t tcp_isn_hash(u_int32_t, u_int32_t, u_int32_t, u_int32_t,
	    u_int32_t *);

/*
 * SYN cookie support (RFC 4987).
 */
extern int tcp_syncookies;
extern u_int32_t tcp_secret_key[];
tcp_seq	 tcp_syncookie_generate(struct in_addr, struct in_addr,
	    u_int16_t, u_int16_t, u_int16_t);
u_int16_t tcp_syncookie_validate(struct in_addr, struct in_addr,
	    u_int16_t, u_int16_t, tcp_seq);

/*
 * Dual-stack (IPv4/IPv6) support.
 *
 * tcp_isipv6() returns non-zero for a TCP socket in the AF_INET6 domain.
 * The TCP engine uses it to branch between the IPv4 (in_pcb*, ip_output,
 * in_cksum, struct ip) and IPv6 (in6_pcb*, ip6_output, in6_cksum,
 * struct ip6_hdr) paths.
 */
struct inpcb;
struct mbuf;
struct in6_addr;
struct inpcbhead;
int  tcp_isipv6(struct inpcb *inp);

/* IPv6 protocol control block helpers (netinet6/in6_pcb.c). */
int  in6_pcbbind(struct inpcb *, struct mbuf *);
int  in6_pcbconnect(struct inpcb *, struct mbuf *);
void in6_pcbdisconnect(struct inpcb *);
void in6_setsockaddr(struct inpcb *, struct mbuf *);
void in6_setpeeraddr(struct inpcb *, struct mbuf *);
struct inpcb *in6_pcblookup(struct inpcbhead *, struct in6_addr *, u_int,
	    struct in6_addr *, u_int);

/* IPv6 output path (netinet6/ip6_output.c, netinet6/in6.c). */
extern int  ip6_defhlim;		/* default IPv6 hop limit */
extern int  ip6_output(void *, ...);	/* (m, opt, ro, flags, im6o, ifpp, inp) */

/*
 * Shared dual-stack TCP/IPv6 output helper (netinet/tcp_subr.c).  m must start
 * at the TCP header (hdr+opts+data, length tlen); prepends and fills the IPv6
 * header, computes the checksum and calls ip6_output().
 */
struct ip6_moptions;
int  tcp_v6output(struct mbuf *, struct in6_addr *, struct in6_addr *,
	    int hlim, int tlen, struct inpcb *);

#endif
