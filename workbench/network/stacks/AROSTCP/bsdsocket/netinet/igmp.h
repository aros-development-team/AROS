/*
 * Copyright (C) 2026 The AROS Development Team.  All rights reserved.
 *
 * IGMPv2 host membership (RFC 2236) for IPv4.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * igmp.h - Internet Group Management Protocol, version 2 (RFC 2236).
 *
 * Defines the on-the-wire IGMP message, the message-type constants and the
 * per-membership state values used by igmp.c.  The layout is deliberately
 * kept local to the stack since the common includes do not carry an IGMP
 * definition.
 */

#ifndef NETINET_IGMP_H
#define NETINET_IGMP_H

/*
 * IP protocol number for IGMP (defined here because the common
 * netinet/in.h does not provide it).
 */
#ifndef IPPROTO_IGMP
#define	IPPROTO_IGMP		2
#endif

/*
 * IGMP message, on the wire (RFC 2236 §2).  Eight octets, carried directly
 * in an IPv4 datagram with protocol IPPROTO_IGMP and TTL 1.
 */
struct igmp {
	u_char		igmp_type;	/* message type */
	u_char		igmp_code;	/* max response time (1/10 s), queries */
	u_short		igmp_cksum;	/* IP-style ones-complement checksum */
	struct in_addr	igmp_group;	/* group address (0 for general query) */
};

#define	IGMP_MINLEN		8	/* sizeof(struct igmp) on the wire */

/*
 * IGMP message types (RFC 2236 §2.1).
 */
#define	IGMP_MEMBERSHIP_QUERY		0x11	/* membership query */
#define	IGMP_V1_MEMBERSHIP_REPORT	0x12	/* IGMPv1 membership report */
#define	IGMP_V2_MEMBERSHIP_REPORT	0x16	/* IGMPv2 membership report */
#define	IGMP_V2_LEAVE_GROUP		0x17	/* leave-group message */

/*
 * Protocol timing (RFC 2236 §8).  igmp_code and the query Max Response Time
 * are expressed in units of 1/10 second.
 */
#define	IGMP_MAX_HOST_REPORT_DELAY	10	/* seconds; v1-query default */
#define	IGMP_TIMER_SCALE		10	/* 1/10-second units per second */

/*
 * Per-membership state (in_multi.inm_state).  These mirror the classic BSD
 * IGMP host state machine and drive whether a Leave message is sent.
 */
#define	IGMP_OTHERMEMBER		0	/* another host is the last reporter */
#define	IGMP_IREPORTEDLAST		1	/* we sent the most recent report */

/*
 * Well-known IPv4 multicast groups (host order).
 */
#ifndef INADDR_ALLHOSTS_GROUP
#define	INADDR_ALLHOSTS_GROUP	0xe0000001UL	/* 224.0.0.1 */
#endif
#ifndef INADDR_ALLRTRS_GROUP
#define	INADDR_ALLRTRS_GROUP	0xe0000002UL	/* 224.0.0.2 */
#endif

#ifdef KERNEL
void igmp_init(void);
void igmp_input(void *, ...);
void igmp_joingroup(struct in_multi *);
void igmp_leavegroup(struct in_multi *);
void igmp_fasttimo(void);
void igmp_slowtimo(void);
#endif

#endif /* NETINET_IGMP_H */
