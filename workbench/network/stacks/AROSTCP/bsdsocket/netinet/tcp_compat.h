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

#endif
