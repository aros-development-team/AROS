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

/* Maximum rules: of both families together */
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

/*
 * IPv6 filter rule - AROSTCP. Added and removed with SIOCADAFR6,
 * SIOCINAFR6 (insert at the position given in `hits') and SIOCRMAFR6
 * (sys/ioctl.h); SIOCIPFFL, SIOCFRENB and the statistics act on the rules
 * of both families. A packet of either family is judged by the rules of
 * its own family only: a rule set that is to cover IPv6 needs IPv6 rules
 * (ipf writes a rule that names no address and no family as one rule of
 * each).
 *
 * Fields as in struct ipf_rule, but:
 *  size	sizeof(struct ipf_rule6) as the program knows it; a rule of
 *		another size is refused (EINVAL)
 *  proto	the upper-layer protocol, after the extension headers;
 *		IPPROTO_ICMPV6 for an icmp_type rule
 *  *_addr	an IPv6 address, and *_plen its prefix length (0 to 128; 0:
 *		any address, and the match is skipped as for a zero
 *		address and mask in struct ipf_rule)
 *  icmp_type	an ICMPv6 type, -1 any; icmp_code likewise
 *  rflags	IPF_IPOPTS: the packet has extension headers; IPF_SHORT: the
 *		TCP or UDP header is shorter than 8 bytes
 *
 * A fragment that is not the first carries no upper-layer header: a rule
 * that names ports, TCP flags or an ICMP type does not match it (in both
 * families). Neighbour discovery is ICMPv6 (types 133 to 137): a rule set
 * that blocks all incoming ICMPv6 stops IPv6 on the link.
 */
struct ipf_rule6 {
	struct ipf_rule6 *next;

	u_int16_t	size;
	u_int8_t	action;
	u_int8_t	dir;
	u_int16_t	rflags;

	char		ifname[IPF_MAXIFNAME];

	u_int16_t	proto;

	struct in6_addr	src_addr;
	struct in6_addr	dst_addr;
	u_int8_t	src_plen;
	u_int8_t	src_not;
	u_int8_t	dst_plen;
	u_int8_t	dst_not;

	u_int8_t	sport_op;
	u_int8_t	dport_op;
	u_int16_t	sport_lo;
	u_int16_t	sport_hi;
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
