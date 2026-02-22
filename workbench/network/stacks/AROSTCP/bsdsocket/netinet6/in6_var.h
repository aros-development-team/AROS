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

#ifndef NETINET6_IN6_VAR_H
#define NETINET6_IN6_VAR_H

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/icmp6.h>
#include <net/if.h>

/*
 * IPv6 multicast membership record (kernel-internal).
 */
struct in6_multi {
	struct in6_addr in6m_addr;	/* IPv6 multicast address */
	struct ifnet   *in6m_ifp;	/* back pointer to ifnet */
	u_int		in6m_refcount;	/* reference count */
	struct in6_multi *in6m_next;	/* linked list */
};

/*
 * Macros for IPv6 address classification.
 */
#define IN6_IS_ADDR_UNSPECIFIED(a) \
	((a)->s6_addr32[0] == 0 && (a)->s6_addr32[1] == 0 && \
	 (a)->s6_addr32[2] == 0 && (a)->s6_addr32[3] == 0)

#define IN6_IS_ADDR_LOOPBACK(a) \
	((a)->s6_addr32[0] == 0 && (a)->s6_addr32[1] == 0 && \
	 (a)->s6_addr32[2] == 0 && ntohl((a)->s6_addr32[3]) == 1)

#define IN6_IS_ADDR_MULTICAST(a)	((a)->s6_addr[0] == 0xff)

#define IN6_IS_ADDR_LINKLOCAL(a) \
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))

#define IN6_IS_ADDR_SITELOCAL(a) \
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))

#define IN6_IS_ADDR_V4MAPPED(a) \
	((a)->s6_addr32[0] == 0 && (a)->s6_addr32[1] == 0 && \
	 ntohl((a)->s6_addr32[2]) == 0x0000ffff)

#define IN6_IS_ADDR_V4COMPAT(a) \
	((a)->s6_addr32[0] == 0 && (a)->s6_addr32[1] == 0 && \
	 (a)->s6_addr32[2] == 0 && \
	 ntohl((a)->s6_addr32[3]) > 1)

#define IN6_IS_ADDR_MC_NODELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a) && (((a)->s6_addr[1] & 0x0f) == 0x01))

#define IN6_IS_ADDR_MC_LINKLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a) && (((a)->s6_addr[1] & 0x0f) == 0x02))

#define IN6_IS_ADDR_MC_SITELOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a) && (((a)->s6_addr[1] & 0x0f) == 0x05))

#define IN6_IS_ADDR_MC_ORGLOCAL(a) \
	(IN6_IS_ADDR_MULTICAST(a) && (((a)->s6_addr[1] & 0x0f) == 0x08))

#define IN6_IS_ADDR_MC_GLOBAL(a) \
	(IN6_IS_ADDR_MULTICAST(a) && (((a)->s6_addr[1] & 0x0f) == 0x0e))

/* Convenience: access s6_addr32 portably without assuming struct layout */
#ifndef s6_addr32
#define s6_addr32 un.u32_addr
#endif

/* IPv6 address equality comparison */
#define IN6_ARE_ADDR_EQUAL(a, b) \
	(bcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof(struct in6_addr)) == 0)

/* IPv6 statistics */
struct ip6stat {
	u_long ip6s_total;		/* total packets received */
	u_long ip6s_tooshort;		/* packet too short */
	u_long ip6s_toosmall;		/* not enough data */
	u_long ip6s_fragments;		/* fragments received */
	u_long ip6s_fragdropped;	/* frags dropped (out of space) */
	u_long ip6s_fragtimeout;	/* fragments timed out */
	u_long ip6s_fragoverflow;	/* fragments that exceeded limit */
	u_long ip6s_forward;		/* packets forwarded */
	u_long ip6s_cantforward;	/* packets rcvd for unreachable dest */
	u_long ip6s_redirectsent;	/* packets forwarded on same net */
	u_long ip6s_delivered;		/* datagrams delivered to upper level */
	u_long ip6s_localout;		/* total ip packets generated here */
	u_long ip6s_odropped;		/* lost packets due to nobufs, etc. */
	u_long ip6s_reassembled;	/* total packets reassembled ok */
	u_long ip6s_fragmented;		/* datagrams successfully fragmented */
	u_long ip6s_ofragments;		/* output fragments created */
	u_long ip6s_cantfrag;		/* don't fragment flag was set, etc. */
	u_long ip6s_badoptions;		/* error in option processing */
	u_long ip6s_noroute;		/* packets discarded due to no route */
	u_long ip6s_badvers;		/* IPv6 version != 6 */
	u_long ip6s_rawout;		/* total raw ip packets generated */
	u_long ip6s_badscope;		/* scope error */
	u_long ip6s_notmember;		/* don't join this multicast group */
	u_long ip6s_nxthist[256];	/* next header history */
	u_long ip6s_m1;			/* one mbuf */
	u_long ip6s_m2m[32];		/* two or more mbufs */
	u_long ip6s_mext1;		/* one ext mbuf */
	u_long ip6s_mext2m;		/* two or more ext mbufs */
	u_long ip6s_exthdrtoolong;	/* ext hdr too long; could be dropped */
	u_long ip6s_nogif;		/* no match gif found */
	u_long ip6s_toomanyhdr;		/* discarded due to too many headers */
	u_long ip6s_sources_none;
	u_long ip6s_sources_sameif[16];
	u_long ip6s_sources_otherif[16];
	u_long ip6s_sources_samescope[16];
	u_long ip6s_sources_otherscope[16];
	u_long ip6s_sources_deprecated[16];
	u_long ip6s_forward_cachehit;
	u_long ip6s_forward_cachemiss;
};

/* icmp6stat defined in <netinet/icmp6.h> */

/*
 * route_in6 - like struct route but with a properly-sized IPv6 destination.
 * Use this instead of struct route when doing IPv6 route lookups.
 */
struct route_in6 {
	struct  rtentry      *ro_rt;
	struct  sockaddr_in6  ro_dst;
};

#ifdef KERNEL
extern struct ip6stat ip6stat;
extern struct icmp6stat icmp6stat;
extern struct in6_ifaddr *in6_ifaddr;
extern struct ifqueue ip6intrq;		/* IPv6 packet input queue */
extern int ip6_defhlim;
extern int ip6_forwarding;
extern int ip6_v6only;

int  in6_control(struct socket *, int, caddr_t, struct ifnet *);
void in6_ifscrub(struct ifnet *, struct in6_ifaddr *);
int  in6_ifinit(struct ifnet *, struct in6_ifaddr *, struct sockaddr_in6 *, int);
void in6_purgeaddr(struct ifaddr *);
struct in6_ifaddr *in6_ifawithifp(struct ifnet *, struct in6_addr *);
struct in6_ifaddr *in6_ifaof_ifpforlinklocal(struct ifnet *);
int  in6_localaddr(struct in6_addr *);
void in6_if_up(struct ifnet *);
#endif /* KERNEL */

#endif /* NETINET6_IN6_VAR_H */
