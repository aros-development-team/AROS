/*
 * net/ipfilter.h - IP Filter (ipf) kernel structures and API
 *
 * Provides packet filtering for AROSTCP, compatible with Roadshow ipf syntax.
 * Rules are evaluated in the pfil hook path (pfil_run_hooks).
 *
 * Copyright (C) 2026 The AROS Dev Team
 */

#ifndef _NET_IPFILTER_H_
#define _NET_IPFILTER_H_

#include <sys/types.h>
#include <netinet/in.h>

/* Rule actions */
#define IPF_PASS	0
#define IPF_BLOCK	1
#define IPF_LOG		2
#define IPF_COUNT	3

/* Rule directions */
#define IPF_IN		0x01
#define IPF_OUT		0x02
#define IPF_INOUT	(IPF_IN|IPF_OUT)

/* Rule flags */
#define IPF_QUICK	0x0001	/* stop processing on match */
#define IPF_LOG_BODY	0x0002	/* log packet body */
#define IPF_LOG_FIRST	0x0004	/* log first packet only */
#define IPF_LOG_ORBLOCK	0x0008	/* block if log fails */
#define IPF_KEEP_STATE	0x0010	/* keep connection state */
#define IPF_KEEP_FRAGS	0x0020	/* keep fragment state */
#define IPF_RETURN_RST	0x0040	/* return TCP RST on block */
#define IPF_RETURN_ICMP	0x0080	/* return ICMP error on block */
#define IPF_SHORT	0x0100	/* match short packets */
#define IPF_FRAG	0x0200	/* match fragments */
#define IPF_IPOPTS	0x0400	/* match IP options */

/* Protocol match */
#define IPF_PROTO_ANY	0	/* match any protocol */
#define IPF_PROTO_TCPUDP 256	/* match both TCP and UDP */

/* Port comparison operators */
#define IPF_PORT_NONE	0
#define IPF_PORT_EQ	1	/* = */
#define IPF_PORT_NE	2	/* != */
#define IPF_PORT_LT	3	/* < */
#define IPF_PORT_GT	4	/* > */
#define IPF_PORT_LE	5	/* <= */
#define IPF_PORT_GE	6	/* >= */
#define IPF_PORT_RANGE	7	/* port1 <> port2 (exclusive) */
#define IPF_PORT_RANGEINCL 8	/* port1 >< port2 (inclusive) */

/* Maximum rules */
#define IPF_MAXRULES	256
#define IPF_MAXIFNAME	16

/* Flush flags for ipf_flush() */
#define IPF_FLUSH_IN	0x01	/* flush input rules */
#define IPF_FLUSH_OUT	0x02	/* flush output rules */
#define IPF_FLUSH_ALL	(IPF_FLUSH_IN|IPF_FLUSH_OUT)

/*
 * IP filter rule structure
 */
struct ipf_rule {
	struct ipf_rule	*next;

	/* Action and direction */
	u_int8_t	action;		/* IPF_PASS, IPF_BLOCK, etc. */
	u_int8_t	dir;		/* IPF_IN, IPF_OUT */
	u_int16_t	rflags;		/* IPF_QUICK, IPF_KEEP_STATE, etc. */

	/* Interface match (empty = any) */
	char		ifname[IPF_MAXIFNAME];

	/* Protocol (0 = any, IPPROTO_TCP, etc., IPF_PROTO_TCPUDP) */
	u_int16_t	proto;

	/* Source address/mask */
	struct in_addr	src_addr;
	struct in_addr	src_mask;
	u_int8_t	src_not;	/* negate source match */

	/* Destination address/mask */
	struct in_addr	dst_addr;
	struct in_addr	dst_mask;
	u_int8_t	dst_not;	/* negate destination match */

	/* Source port match */
	u_int8_t	sport_op;	/* comparison operator */
	u_int16_t	sport_lo;
	u_int16_t	sport_hi;	/* for range */

	/* Destination port match */
	u_int8_t	dport_op;
	u_int16_t	dport_lo;
	u_int16_t	dport_hi;

	/* TCP flags match */
	u_int8_t	tcp_flags;	/* flags to check */
	u_int8_t	tcp_flagmask;	/* mask for flags */

	/* ICMP type match (-1 = any) */
	int16_t		icmp_type;
	int16_t		icmp_code;	/* -1 = any */

	/* Statistics */
	u_int64_t	hits;
	u_int64_t	bytes;
};

/*
 * NAT rule types
 */
#define IPNAT_MAP	0	/* source NAT (outgoing) */
#define IPNAT_BIMAP	1	/* bidirectional NAT */
#define IPNAT_RDR	2	/* redirect (DNAT, incoming) */
#define IPNAT_MAPBLOCK	3	/* static block mapping */

/*
 * NAT rule structure
 */
struct ipnat_rule {
	struct ipnat_rule *next;

	u_int8_t	type;		/* IPNAT_MAP, IPNAT_RDR, etc. */
	char		ifname[IPF_MAXIFNAME];

	/* Match (source for map, destination for rdr) */
	struct in_addr	match_addr;
	struct in_addr	match_mask;

	/* For rdr: destination port match */
	u_int16_t	match_port_lo;
	u_int16_t	match_port_hi;

	/* Translation target */
	struct in_addr	nat_addr;
	struct in_addr	nat_mask;
	struct in_addr	nat_addr2;	/* secondary for round-robin rdr */

	/* Port mapping */
	u_int16_t	nat_port_lo;
	u_int16_t	nat_port_hi;
	u_int16_t	nat_proto;	/* tcp, udp, or tcp/udp */

	u_int8_t	round_robin;	/* round-robin flag */

	/* Statistics */
	u_int64_t	hits;
};

/*
 * Active NAT translation entry
 */
struct ipnat_entry {
	struct ipnat_entry *next;

	struct in_addr	orig_src;
	u_int16_t	orig_sport;
	struct in_addr	orig_dst;
	u_int16_t	orig_dport;

	struct in_addr	trans_addr;
	u_int16_t	trans_port;
	u_int8_t	proto;

	u_int32_t	expire;		/* expiration time */
};

/* IPF state */
struct ipf_state {
	int		enabled;	/* filter enabled */
	struct ipf_rule	*rules_in;	/* input rules list */
	struct ipf_rule	*rules_out;	/* output rules list */
	u_int32_t	rule_count;
	u_int64_t	passed;
	u_int64_t	blocked;
	u_int64_t	logged;
};

/* IPNAT state */
struct ipnat_state {
	struct ipnat_rule *rules;	/* NAT rules list */
	struct ipnat_entry *entries;	/* active translations */
	u_int32_t	rule_count;
	u_int32_t	entry_count;
};

/* Kernel functions */
void	ipf_init(void);
void	ipf_cleanup(void);
int	ipf_enabled(void);
void	ipf_enable(void);
void	ipf_disable(void);
int	ipf_add_rule(struct ipf_rule *rule);
int	ipf_insert_rule(struct ipf_rule *rule);
int	ipf_remove_rule(struct ipf_rule *rule);
int	ipf_flush(int flags);
int	ipf_check(struct mbuf *m, struct ifnet *ifp, int dir);

void	ipnat_init(void);
void	ipnat_cleanup(void);
int	ipnat_add_rule(struct ipnat_rule *rule);
int	ipnat_remove_rule(struct ipnat_rule *rule);
int	ipnat_clear_rules(void);
int	ipnat_flush_entries(void);

/* Global state */
extern struct ipf_state ipf_global;
extern struct ipnat_state ipnat_global;

#endif /* _NET_IPFILTER_H_ */
