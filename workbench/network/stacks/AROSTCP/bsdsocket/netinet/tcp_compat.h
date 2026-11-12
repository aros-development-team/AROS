#ifndef NETINET_TCP_COMPAT_H
#define NETINET_TCP_COMPAT_H

/* Legacy definitions used by AROSTCP - code using these needs updated */

/* increment for tcp_iss each second */
#define	TCP_ISSINCR	(125*1024)

/* timestamp wrap-around time */
#define TCP_PAWS_IDLE	(24 * 24 * 60 * 60 * PR_SLOWHZ)

extern tcp_seq	tcp_iss;		/* tcp initial send seq # */
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
 * RFC 7323 section 5.4: a per-connection timestamp offset, from a key of
 * its own (tcp_ts_key).
 */
u_int32_t tcp_new_tsoff(struct inpcb *inp);
extern u_int32_t tcp_ts_key[];

/*
 * The keyed hash of a connection (SipHash-2-4), and what it is for.
 */
#define TCPH_ISN	1
#define TCPH_COOKIE	2
#define TCPH_SYNCACHE	3
#define TCPH_TSOFF	4
/* (key, TCPH_xxx, address length, local and foreign address and port, extra) */
u_int32_t tcp_conn_hash(const u_int32_t *, int, int, const void *,
	    const void *, u_int16_t, u_int16_t, u_int32_t);

/*
 * SYN cookie support (RFC 4987).
 */
extern int tcp_syncookies;
extern u_int32_t tcp_secret_key[];
tcp_seq	 tcp_syncookie_generate(int, const void *, const void *,
	    u_int16_t, u_int16_t, u_int16_t);
u_int16_t tcp_syncookie_validate(int, const void *, const void *,
	    u_int16_t, u_int16_t, tcp_seq);

/*
 * TCP and the address families (netinet/inetfamily.h). TCP does not know
 * which family a connection is of: it asks the family the PCB points to
 * (inp_family), or the one a segment came in by.
 */
struct mbuf;
struct ifnet;
struct inetfamily;

/*
 * The largest network header a family puts in front of a segment, options
 * apart (IPv6's): only sizes the room kept free in front of what is built.
 */
#define TCP_NETHDR_MAX	40

/*
 * A segment waiting in the reassembly queue keeps the length of its data
 * where its checksum was (verified long before it got there).
 */
#define th_qlen		th_sum

void	 tcp_mtudisc(struct tcpcb *, u_int32_t);

/* TCP's variables, registered by name (netinet/tcp_usrreq.c) */
void	 tcp_vars_register(void);
/* ... and taken back when TCP stops; its variables are then as the file has them */
void	 tcp_vars_unregister(void);

/*
 * TCP stops (netinet/inet_transport.c transport_stop()): each file puts
 * its own variables back to what the file gives them, and frees what it
 * allocated. A module library that stays in memory between a stop and the
 * next start has only this to go by: its data is not loaded again.
 */
void	 tcp_deinit(void);		/* tcp_subr.c: undoes tcp_init() */
void	 tcp_input_defaults(void);	/* tcp_input.c */
void	 tcp_timer_defaults(void);	/* tcp_timer.c */
void	 tcp_cc_defaults(void);		/* tcp_cc_cubic.c */
void	 tcp_rack_defaults(void);	/* tcp_rack.c */
void	 tcp_ss_defaults(void);		/* tcp_ss.c */
void	 tcp_debug_free(void);		/* tcp_debug.c */

#endif
