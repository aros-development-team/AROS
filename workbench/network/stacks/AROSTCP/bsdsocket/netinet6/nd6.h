/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 / FreeBSD Neighbor Discovery implementation.
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NETINET6_ND6_H
#define NETINET6_ND6_H

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>

/* Neighbor Discovery default constants (RFC 4861) */
#define ND6_MAX_RTR_SOLICITATIONS	3
#define ND6_RTR_SOLICITATION_INTERVAL	4	/* seconds */
#define ND6_MAX_RTR_SOLICITATION_DELAY	1	/* seconds */
#define ND6_MAX_MULTICAST_SOLICIT	3
#define ND6_MAX_UNICAST_SOLICIT		3
#define ND6_MAX_ANYCAST_DELAY_TIME	1
#define ND6_MAX_NEIGHBOR_ADVERTISEMENT	3
#define ND6_REACHABLE_TIME		30000	/* ms */
#define ND6_RETRANS_TIMER		1000	/* ms */
#define ND6_DELAY_FIRST_PROBE_TIME	5	/* seconds */
#define ND6_MIN_RANDOM_FACTOR		500	/* ms */
#define ND6_MAX_RANDOM_FACTOR		1500	/* ms */

/* Limits */
#define ND6_MAX_DEFROUTERS	8	/* max tracked default routers */
#define ND6_MAX_PREFIXES	16	/* max tracked prefixes */
#define ND6_MAX_DAD_NS		1	/* DAD NS transmissions */

/* Neighbor Cache Entry states (RFC 4861 §7.3.2) */
#define ND6_LLINFO_NOSTATE	-1
#define ND6_LLINFO_INCOMPLETE	0	/* address resolution in progress */
#define ND6_LLINFO_REACHABLE	1	/* neighbor known reachable */
#define ND6_LLINFO_STALE	2	/* reachability uncertain */
#define ND6_LLINFO_DELAY	3	/* waiting before probing */
#define ND6_LLINFO_PROBE	4	/* sending unicast NS probes */

/*
 * Neighbor cache entry (llinfo_nd6).
 * One per neighbor, linked in a doubly-linked list with sentinel.
 */
struct llinfo_nd6 {
	struct llinfo_nd6 *ln_next;
	struct llinfo_nd6 *ln_prev;
	struct rtentry *ln_rt;		/* back-pointer to routing entry */
	struct mbuf *ln_hold;		/* queued packet while resolving */
	long ln_asked;			/* # of NS sent */
	short ln_state;			/* ND6_LLINFO_* */
	short ln_router;		/* neighbor is a router */
	long ln_expire;			/* expiry time (seconds) */
	long ln_ntick;			/* ticks for state transitions */
};

/*
 * Per-interface ND information (RFC 4861 §6.3.2).
 */
struct nd_ifinfo {
	u_int32_t linkmtu;		/* LinkMTU from RA, 0 = if_mtu */
	u_int32_t maxmtu;		/* max MTU for this interface */
	u_int32_t basereachable;	/* BaseReachableTime (ms) */
	u_int32_t reachable;		/* Reachable Time (computed, ms) */
	u_int32_t retrans;		/* Retrans Timer (ms) */
	int flags;			/* ND6_IFF_* */
	int recalctm;			/* RecalcReachable countdown */
	u_int8_t chlim;			/* CurHopLimit */
	int initialized;
	int rtr_soliciting;		/* # RS sent so far */
	long rtr_solicit_timer;		/* next RS time (seconds) */
};
#define ND6_IFF_PERFORMNUD	0x01	/* perform NUD */
#define ND6_IFF_ACCEPT_RTADV	0x02	/* accept Router Advertisements */
#define ND6_IFF_AUTO_LINKLOCAL	0x20	/* auto-configure link-local */
#define ND6_IFF_IFDISABLED	0x80	/* interface disabled for IPv6 */

/*
 * Default router list entry (RFC 4861 §6.3.4).
 */
struct nd_defrouter {
	struct nd_defrouter *next;
	struct in6_addr rtaddr;		/* router link-local address */
	struct ifnet *ifp;		/* receiving interface */
	u_int16_t rtlifetime;		/* router lifetime (seconds) */
	long expire;			/* expiry time (seconds) */
	u_int8_t flags;			/* ND_RA_FLAG_* */
	u_int8_t installed;		/* route installed in FIB */
};

/*
 * Prefix list entry (RFC 4861 §6.3.4).
 */
struct nd_prefix {
	struct nd_prefix *next;
	struct ifnet *ndpr_ifp;		/* interface */
	struct sockaddr_in6 ndpr_prefix; /* prefix address */
	u_int8_t ndpr_plen;		/* prefix length */
	u_int8_t ndpr_vltime_set;	/* valid lifetime was set */
	u_int32_t ndpr_vltime;		/* valid lifetime (seconds) */
	u_int32_t ndpr_pltime;		/* preferred lifetime (seconds) */
	long ndpr_expire;		/* expiry time (seconds) */
	long ndpr_preferred;		/* preferred expiry (seconds) */
	u_int8_t ndpr_flags;		/* ND_OPT_PI_FLAG_* */
	int ndpr_refcount;		/* addresses using this prefix */
	int ndpr_onlink;		/* on-link route installed */
	int ndpr_autoconf;		/* SLAAC address created */
};

/*
 * DAD (Duplicate Address Detection) state (RFC 4862 §5.4).
 */
struct dadq {
	struct dadq *next;
	struct in6_ifaddr *ia;		/* address being verified */
	struct ifnet *ifp;
	int count;			/* NS probes sent */
	int ns_icount;			/* NS received (duplicate!) */
	int ns_ocount;			/* NS sent */
	long timer;			/* next probe time (seconds) */
};

/* ND statistics */
struct nd6stat {
	u_long nd6s_rcv_ns;		/* NS received */
	u_long nd6s_rcv_na;		/* NA received */
	u_long nd6s_rcv_rs;		/* RS received */
	u_long nd6s_rcv_ra;		/* RA received */
	u_long nd6s_rcv_redirect;	/* Redirect received */
	u_long nd6s_snd_ns;		/* NS sent */
	u_long nd6s_snd_na;		/* NA sent */
	u_long nd6s_snd_rs;		/* RS sent */
	u_long nd6s_rcv_badns;		/* bad NS */
	u_long nd6s_rcv_badna;		/* bad NA */
	u_long nd6s_rcv_badrs;		/* bad RS */
	u_long nd6s_rcv_badra;		/* bad RA */
	u_long nd6s_rcv_badredirect;	/* bad Redirect */
	u_long nd6s_nud_incomplete;	/* NUD: entries timed out incomplete */
	u_long nd6s_nud_probe;		/* NUD: probe failures */
	u_long nd6s_dad_dup;		/* DAD: duplicate detected */
	u_long nd6s_res_holdq_full;	/* queued packet replaced */
};

/* nd_opts union for option parsing (RFC 4861 §4.6) */
union nd_opts {
	struct nd_opt_hdr *nd_opt_array[10];
	struct {
		struct nd_opt_hdr *zero;
		struct nd_opt_hdr *src_lladdr;
		struct nd_opt_hdr *tgt_lladdr;
		struct nd_opt_prefix_info *pi_beg;
		struct nd_opt_rd_hdr *rh;
		struct nd_opt_mtu *mtu;
		struct nd_opt_hdr *search;
		struct nd_opt_hdr *last;
		int done;
		struct nd_opt_prefix_info *pi_end;
	} nd_opt_each;
};
#define nd_opts_src_lladdr	nd_opt_each.src_lladdr
#define nd_opts_tgt_lladdr	nd_opt_each.tgt_lladdr
#define nd_opts_pi		nd_opt_each.pi_beg
#define nd_opts_pi_end		nd_opt_each.pi_end
#define nd_opts_rh		nd_opt_each.rh
#define nd_opts_mtu		nd_opt_each.mtu
#define nd_opts_search		nd_opt_each.search
#define nd_opts_last		nd_opt_each.last
#define nd_opts_done		nd_opt_each.done

#ifdef KERNEL

/* --- Globals (nd6.c) --- */
extern struct llinfo_nd6 llinfo_nd6;
extern int nd6_maxndopt;
extern int nd6_maxqueuelen;
extern struct nd6stat nd6stat;

/* --- nd6.c: core neighbor cache and NUD --- */
void nd6_init(void);
void nd6_ifattach(struct ifnet *);
struct nd_ifinfo *nd6_ifinfo(struct ifnet *);
int  nd6_is_addr_neighbor(struct sockaddr_in6 *, struct ifnet *);
void nd6_option_init(void *, int, union nd_opts *);
int  nd6_options(union nd_opts *);
struct rtentry *nd6_lookup(struct in6_addr *, int, struct ifnet *);
void nd6_setmtu(struct ifnet *);
void nd6_timer(void *);
void nd6_nud_hint(struct rtentry *);
int  nd6_resolve(struct ifnet *, struct rtentry *, struct mbuf *,
	struct sockaddr *, u_char *);
void nd6_rtrequest(int, struct rtentry *, struct sockaddr *);
int  nd6_ioctl(u_long, caddr_t, struct ifnet *);
int  nd6_cache_lladdr(struct ifnet *, struct in6_addr *,
	char *, int, int, int);
caddr_t nd6_ifptomac(struct ifnet *);

/* --- nd6_nbr.c: Neighbor Solicitation / Advertisement + DAD --- */
void nd6_ns_input(struct mbuf *, int, int);
void nd6_ns_output(struct ifnet *, struct in6_addr *, struct in6_addr *,
	struct llinfo_nd6 *, int);
void nd6_na_input(struct mbuf *, int, int);
void nd6_na_output(struct ifnet *, struct in6_addr *, struct in6_addr *,
	u_long, int, struct sockaddr *);
void nd6_dad_start(struct in6_ifaddr *);
void nd6_dad_stop(struct in6_ifaddr *);
void nd6_dad_timer(void);
void nd6_dad_ns_input(struct in6_ifaddr *);

/* --- nd6_rtr.c: Router Solicitation / Advertisement --- */
void nd6_rs_input(struct mbuf *, int, int);
void nd6_rs_output(struct ifnet *);
void nd6_ra_input(struct mbuf *, int, int);
void nd6_redirect_input(struct mbuf *, int, int);
struct nd_defrouter *defrouter_lookup(struct in6_addr *, struct ifnet *);
void defrouter_select(void);
struct nd_prefix *nd6_prefix_lookup(struct sockaddr_in6 *, int);
void nd6_prefix_timer(void);
void nd6_defrouter_timer(void);

#endif /* KERNEL */

#endif /* NETINET6_ND6_H */
