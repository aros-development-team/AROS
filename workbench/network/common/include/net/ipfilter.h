/*
 * net/ipfilter.h - IP Filter public header for AROSTCP
 *
 * Defines rule structures shared between kernel and userspace
 * ipf/ipnat commands. Compatible with Roadshow ipf syntax.
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
#define IPF_QUICK	0x0001
#define IPF_LOG_BODY	0x0002
#define IPF_LOG_FIRST	0x0004
#define IPF_LOG_ORBLOCK	0x0008
#define IPF_KEEP_STATE	0x0010
#define IPF_KEEP_FRAGS	0x0020
#define IPF_RETURN_RST	0x0040
#define IPF_RETURN_ICMP	0x0080
#define IPF_SHORT	0x0100
#define IPF_FRAG	0x0200
#define IPF_IPOPTS	0x0400

/* Protocol match */
#define IPF_PROTO_ANY	0
#define IPF_PROTO_TCPUDP 256

/* Port comparison operators */
#define IPF_PORT_NONE	0
#define IPF_PORT_EQ	1
#define IPF_PORT_NE	2
#define IPF_PORT_LT	3
#define IPF_PORT_GT	4
#define IPF_PORT_LE	5
#define IPF_PORT_GE	6
#define IPF_PORT_RANGE	7
#define IPF_PORT_RANGEINCL 8

/* Maximum rules */
#define IPF_MAXRULES	256
#define IPF_MAXIFNAME	16

/* Flush flags */
#define IPF_FLUSH_IN	0x01
#define IPF_FLUSH_OUT	0x02
#define IPF_FLUSH_ALL	(IPF_FLUSH_IN|IPF_FLUSH_OUT)

/*
 * IP filter rule structure — shared between kernel and userspace.
 */
struct ipf_rule {
	struct ipf_rule	*next;

	u_int8_t	action;
	u_int8_t	dir;
	u_int16_t	rflags;

	char		ifname[IPF_MAXIFNAME];

	u_int16_t	proto;

	struct in_addr	src_addr;
	struct in_addr	src_mask;
	u_int8_t	src_not;

	struct in_addr	dst_addr;
	struct in_addr	dst_mask;
	u_int8_t	dst_not;

	u_int8_t	sport_op;
	u_int16_t	sport_lo;
	u_int16_t	sport_hi;

	u_int8_t	dport_op;
	u_int16_t	dport_lo;
	u_int16_t	dport_hi;

	u_int8_t	tcp_flags;
	u_int8_t	tcp_flagmask;

	int16_t		icmp_type;
	int16_t		icmp_code;

	u_int64_t	hits;
	u_int64_t	bytes;
};

/* NAT rule types */
#define IPNAT_MAP	0
#define IPNAT_BIMAP	1
#define IPNAT_RDR	2
#define IPNAT_MAPBLOCK	3

/*
 * NAT rule structure — shared between kernel and userspace.
 */
struct ipnat_rule {
	struct ipnat_rule *next;

	u_int8_t	type;
	char		ifname[IPF_MAXIFNAME];

	struct in_addr	match_addr;
	struct in_addr	match_mask;

	u_int16_t	match_port_lo;
	u_int16_t	match_port_hi;

	struct in_addr	nat_addr;
	struct in_addr	nat_mask;
	struct in_addr	nat_addr2;

	u_int16_t	nat_port_lo;
	u_int16_t	nat_port_hi;
	u_int16_t	nat_proto;

	u_int8_t	round_robin;

	u_int64_t	hits;
};

/* IPF state — for SIOCGETFS / SIOCFRZST */
struct ipf_state {
	int		enabled;
	struct ipf_rule	*rules_in;
	struct ipf_rule	*rules_out;
	u_int32_t	rule_count;
	u_int64_t	passed;
	u_int64_t	blocked;
	u_int64_t	logged;
};

/* IPNAT state — for SIOCGNATS */
struct ipnat_state {
	struct ipnat_rule *rules;
	struct ipnat_entry *entries;
	u_int32_t	rule_count;
	u_int32_t	entry_count;
};

/* Active NAT entry — kernel internal, exposed for stats */
struct ipnat_entry {
	struct ipnat_entry *next;

	struct in_addr	orig_src;
	u_int16_t	orig_sport;
	struct in_addr	orig_dst;
	u_int16_t	orig_dport;

	struct in_addr	trans_addr;
	u_int16_t	trans_port;
	u_int8_t	proto;

	u_int32_t	expire;
};

#endif /* _NET_IPFILTER_H_ */
