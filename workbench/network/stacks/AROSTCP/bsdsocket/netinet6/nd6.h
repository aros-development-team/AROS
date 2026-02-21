/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on KAME IPv6 implementation:
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
#define ND6_MAX_RTR_SOLICITATION_DELAY	1
#define ND6_MAX_MULTICAST_SOLICIT	3
#define ND6_MAX_UNICAST_SOLICIT		3
#define ND6_MAX_ANYCAST_DELAY_TIME	1
#define ND6_MAX_NEIGHBOR_ADVERTISEMENT	3
#define ND6_REACHABLE_TIME		30000	/* ms */
#define ND6_RETRANS_TIMER		1000	/* ms */
#define ND6_DELAY_FIRST_PROBE_TIME	5	/* seconds */
#define ND6_MIN_RANDOM_FACTOR		500	/* ms */
#define ND6_MAX_RANDOM_FACTOR		1500	/* ms */

/* Neighbor Cache Entry states */
#define ND6_LLINFO_INCOMPLETE	0	/* address resolution in progress */
#define ND6_LLINFO_REACHABLE	1	/* neighbor known reachable */
#define ND6_LLINFO_STALE	2	/* reachability uncertain */
#define ND6_LLINFO_DELAY	3	/* probe delay */
#define ND6_LLINFO_PROBE	4	/* sending NS probes */
#define ND6_LLINFO_NOSTATE	-1

/* ND option types (RFC 4861 sec 4.6) - see netinet/icmp6.h */

/*
 * Neighbor cache entry (llinfo_nd6).
 * One per neighbor, linked through the routing table.
 */
struct llinfo_nd6 {
	struct llinfo_nd6 *ln_next;
	struct llinfo_nd6 *ln_prev;
	struct rtentry *ln_rt;
	struct mbuf *ln_hold;		/* queued packet while resolving */
	long ln_asked;			/* #NS sent */
	short ln_state;
	short ln_router;		/* neighbor is a router */
	long ln_expire;			/* expiry time (hz ticks) */
	long ln_ntick;			/* ticks until state transitions */
};

/*
 * Per-interface ND information.
 */
struct nd_ifinfo {
	u_int32_t linkmtu;		/* link MTU, 0 = use ifp->if_mtu */
	u_int32_t maxmtu;		/* max MTU for this interface */
	u_int32_t basereachable;	/* BaseReachableTime (ms) */
	u_int32_t reachable;		/* Reachable Time (computed, ms) */
	u_int32_t retrans;		/* Retrans Timer (ms) */
	int flags;			/* interface flags */
	int recalctm;			/* RecalcReachable timer */
	u_int8_t chlim;			/* CurHopLimit */
	int initialized;
	/* autoconfiguration */
	int rtr_soliciting;		/* # RS sent */
};
#define ND6_IFF_PERFORMNUD	0x1	/* perform NUD */
#define ND6_IFF_ACCEPT_RTADV	0x2	/* accept RA */
#define ND6_IFF_AUTO_LINKLOCAL	0x20	/* auto-configure link-local addr */

#ifdef KERNEL
extern struct llinfo_nd6 llinfo_nd6;
extern struct nd_ifinfo *nd_ifinfo;
extern int nd6_maxndopt;
extern int nd6_maxqueuelen;

void nd6_init(void);
void nd6_ifattach(struct ifnet *);
int  nd6_is_addr_neighbor(struct sockaddr_in6 *, struct ifnet *);
void nd6_option_init(void *, int, union nd_opts *);
int  nd6_options(union nd_opts *);
struct rtentry *nd6_lookup(struct in6_addr *, int, struct ifnet *);
void nd6_setmtu(struct ifnet *);
void nd6_timer(void *);
int  nd6_resolve(struct ifnet *, struct rtentry *, struct mbuf *,
	struct sockaddr *, u_char *);
void nd6_rtrequest(int, struct rtentry *, struct sockaddr *);
int  nd6_ioctl(u_long, caddr_t, struct ifnet *);
int  nd6_cache_lladdr(struct ifnet *, struct in6_addr *,
	char *, int, int, int);
void nd6_input(struct mbuf *, int, int);
void nd6_na_input(struct mbuf *, int, int);
void nd6_na_output(struct ifnet *, struct in6_addr *, struct in6_addr *,
	u_long, int, struct sockaddr *);
void nd6_ns_input(struct mbuf *, int, int);
void nd6_ns_output(struct ifnet *, struct in6_addr *, struct in6_addr *,
	struct llinfo_nd6 *, int);
caddr_t nd6_ifptomac(struct ifnet *);
void nd6_ra_input(struct mbuf *, int, int);
void nd6_rs_input(struct mbuf *, int, int);
#endif /* KERNEL */

/* nd_opts union for option parsing */
union nd_opts {
	struct nd_opt_hdr *nd_opt_array[8];
	struct {
		struct nd_opt_hdr *zero;
		struct nd_opt_hdr *src_lladdr;
		struct nd_opt_hdr *tgt_lladdr;
		struct nd_opt_prefix_info *pi_beg;
		struct nd_opt_redirected_header *rh;
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

#endif /* NETINET6_ND6_H */
