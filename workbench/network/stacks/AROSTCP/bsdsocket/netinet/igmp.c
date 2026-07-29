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
 * igmp.c - Internet Group Management Protocol, version 2 (RFC 2236).
 *
 * IGMP lets an IPv4 multicast router discover which multicast groups have
 * listeners on a directly attached link.  A host sends an unsolicited
 * Membership Report when it joins a group, answers Membership Queries with a
 * (randomly delayed) Report, and sends a Leave Group message when it drops a
 * group it last reported.  Reports are suppressed when another host on the
 * link reports the same group first.
 *
 * This is the host half of IGMPv2 only; there is no querier/router role.
 * The structure, timer model and report-suppression logic mirror the IPv6
 * MLDv1 implementation in netinet6/mld6.c.
 *
 * State machine per in_multi (in_multi.inm_state):
 *   IGMP_OTHERMEMBER    - some other host is the last reporter; no Leave sent
 *   IGMP_IREPORTEDLAST  - we sent the most recent Report; Leave sent on drop
 *
 * inm_timer holds the pending report delay in fast-timer ticks (0 = none).
 */

#include <conf.h>

#ifdef ENABLE_MULTICAST

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/synch.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include "in_var.h"
#include <netinet/igmp.h>

#include <protos/netinet/in_cksum_protos.h>
#include <protos/netinet/ip_output_protos.h>

/*
 * Robustness / timing constants (RFC 2236 §8).  inm_timer counts fast-timer
 * ticks; there are PR_FASTHZ ticks per second.
 */
#define	IGMP_UNSOLICITED_REPORT_TICKS	(IGMP_MAX_HOST_REPORT_DELAY * PR_FASTHZ)

static int igmp_timers_running = 0;
static u_int32_t igmp_random_seed = 1;

/* Simple pseudo-random for report jitter (no arc4random in this kernel). */
static u_int32_t
igmp_random(void)
{
	igmp_random_seed = igmp_random_seed * 1103515245 + 12345;
	return (igmp_random_seed >> 16) & 0x7fff;
}

/* Forward declarations */
static void igmp_sendpkt(struct in_multi *, int, struct in_addr);

/* ------------------------------------------------------------------
 * igmp_init - initialize the IGMP subsystem.
 *
 * Called from the protosw pr_init hook when the IP domain is brought up.
 * ------------------------------------------------------------------ */
void
igmp_init(void)
{
	igmp_timers_running = 0;
	igmp_random_seed = 1;
}

/* ------------------------------------------------------------------
 * igmp_joingroup - called when the host joins a multicast group.
 *
 * Sends an unsolicited IGMPv2 Membership Report immediately and arms the
 * report timer so a second, robustness Report is sent (RFC 2236 §3).  The
 * all-hosts group (224.0.0.1) is never reported.
 * ------------------------------------------------------------------ */
void
igmp_joingroup(struct in_multi *inm)
{
	if(ntohl(inm->inm_addr.s_addr) == INADDR_ALLHOSTS_GROUP) {
		inm->inm_state = IGMP_OTHERMEMBER;
		inm->inm_timer = 0;
		return;
	}

	/* Send the initial unsolicited Report immediately. */
	igmp_sendpkt(inm, IGMP_V2_MEMBERSHIP_REPORT, inm->inm_addr);

	/*
	 * Arm the timer for the second (robustness) Report.  We were the last
	 * host to report, so a Leave must be sent if we drop the group.
	 */
	inm->inm_timer = 1 + (igmp_random() % IGMP_UNSOLICITED_REPORT_TICKS);
	inm->inm_state = IGMP_IREPORTEDLAST;
	igmp_timers_running = 1;
}

/* ------------------------------------------------------------------
 * igmp_leavegroup - called when the host drops a multicast group.
 *
 * Sends an IGMPv2 Leave Group message to the all-routers group (224.0.0.2)
 * if this host was the last to report the group (RFC 2236 §6).  Nothing is
 * sent for the all-hosts group.
 * ------------------------------------------------------------------ */
void
igmp_leavegroup(struct in_multi *inm)
{
	struct in_addr allrtrs;

	if(inm->inm_state != IGMP_IREPORTEDLAST)
		return;
	if(ntohl(inm->inm_addr.s_addr) == INADDR_ALLHOSTS_GROUP)
		return;

	allrtrs.s_addr = htonl(INADDR_ALLRTRS_GROUP);
	igmp_sendpkt(inm, IGMP_V2_LEAVE_GROUP, allrtrs);
}

/* ------------------------------------------------------------------
 * igmp_input - process an inbound IGMP message.
 *
 * Called via the IPv4 protosw pr_input: the first argument is the mbuf and a
 * single int va_arg gives the IP header length.  On entry ip_len has already
 * been converted to host order and had the header length subtracted, so it is
 * the length of the IGMP payload.
 * ------------------------------------------------------------------ */
void
igmp_input(void *arg, ...)
{
	struct mbuf   *m = arg;
	struct ip     *ip = mtod(m, struct ip *);
	struct igmp   *igmp;
	struct ifnet  *ifp = m->m_pkthdr.rcvif;
	struct in_multi *inm;
	int hlen, igmplen, timer, maxresp;
	va_list va;

	va_start(va, arg);
	hlen = va_arg(va, int);
	va_end(va);

	igmplen = ip->ip_len;

	/*
	 * Validate length and pull up the IGMP header.
	 */
	if(igmplen < IGMP_MINLEN) {
		m_freem(m);
		return;
	}
	if(m->m_len < hlen + IGMP_MINLEN) {
		m = m_pullup(m, hlen + IGMP_MINLEN);
		if(m == NULL)
			return;
		ip = mtod(m, struct ip *);
	}
	igmp = (struct igmp *)((u_char *)ip + hlen);

	/*
	 * Verify the IGMP checksum over the message.
	 */
	m->m_data += hlen;
	m->m_len  -= hlen;
	if(in_cksum(m, igmplen)) {
		m->m_data -= hlen;
		m->m_len  += hlen;
		m_freem(m);
		return;
	}
	m->m_data -= hlen;
	m->m_len  += hlen;

	switch(igmp->igmp_type) {
	case IGMP_MEMBERSHIP_QUERY:
		/*
		 * Membership Query (RFC 2236 §2.2, §3).  Schedule a delayed
		 * Report per joined group on the receiving interface.  A
		 * general query (group == 0) covers every group; a
		 * group-specific query covers a single group.
		 *
		 * The Max Response Time (igmp_code) is in units of 1/10 s; an
		 * IGMPv1 query carries 0, which means the fixed 10-second
		 * maximum.
		 */
		maxresp = igmp->igmp_code;
		if(maxresp == 0)
			maxresp = IGMP_MAX_HOST_REPORT_DELAY * IGMP_TIMER_SCALE;

		/* Convert 1/10-second units to fast-timer ticks. */
		timer = (maxresp * PR_FASTHZ) / IGMP_TIMER_SCALE;
		if(timer == 0)
			timer = 1;

		for(inm = in_multihead; inm != NULL; inm = inm->inm_next) {
			if(inm->inm_ifp != ifp)
				continue;
			if(ntohl(inm->inm_addr.s_addr) == INADDR_ALLHOSTS_GROUP)
				continue;
			/* group-specific query: only the named group */
			if(igmp->igmp_group.s_addr != INADDR_ANY &&
			   igmp->igmp_group.s_addr != inm->inm_addr.s_addr)
				continue;
			/*
			 * Start (or shorten) the report timer.  If a timer is
			 * already running and no longer than the new delay,
			 * leave it alone (RFC 2236 §3).
			 */
			if(inm->inm_timer == 0 || inm->inm_timer > (u_int)timer) {
				inm->inm_timer = 1 + (igmp_random() % timer);
				igmp_timers_running = 1;
			}
		}
		break;

	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
		/*
		 * Membership Report from another host (RFC 2236 §3).  If we
		 * have a pending Report for the same group, cancel it: another
		 * member has already told the router, so we stay silent and
		 * are no longer the last reporter.
		 *
		 * (AROS does not loop multicast back by default, so a Report
		 * seen here originated on another host.)
		 */
		for(inm = in_multihead; inm != NULL; inm = inm->inm_next) {
			if(inm->inm_ifp == ifp &&
			   inm->inm_addr.s_addr == igmp->igmp_group.s_addr) {
				inm->inm_timer = 0;
				inm->inm_state = IGMP_OTHERMEMBER;
				break;
			}
		}
		break;

	case IGMP_V2_LEAVE_GROUP:
		/* Hosts ignore Leave messages; only the querier acts on them. */
		break;
	}

	m_freem(m);
}

/* ------------------------------------------------------------------
 * igmp_fasttimo - driven from the protosw fast timer (PR_FASTHZ/second).
 *
 * Counts down each pending report timer and, on expiry, emits a Membership
 * Report for that group.  Mirrors mld6_fasttimeo().
 * ------------------------------------------------------------------ */
void
igmp_fasttimo(void)
{
	struct in_multi *inm;
	int any_running = 0;

	if(!igmp_timers_running)
		return;

	for(inm = in_multihead; inm != NULL; inm = inm->inm_next) {
		if(inm->inm_timer == 0)
			continue;

		if(--inm->inm_timer == 0) {
			igmp_sendpkt(inm, IGMP_V2_MEMBERSHIP_REPORT,
			             inm->inm_addr);
			inm->inm_state = IGMP_IREPORTEDLAST;
		} else {
			any_running = 1;
		}
	}

	igmp_timers_running = any_running;
}

/* ------------------------------------------------------------------
 * igmp_slowtimo - driven from the protosw slow timer (PR_SLOWHZ/second).
 *
 * The IGMPv2 host role has no slow-timer duties (queriers use it for the
 * v1-router present timer); provided for protosw symmetry.
 * ------------------------------------------------------------------ */
void
igmp_slowtimo(void)
{
}

/* ------------------------------------------------------------------
 * igmp_sendpkt - build and transmit an IGMP message.
 *
 * type - IGMP_V2_MEMBERSHIP_REPORT or IGMP_V2_LEAVE_GROUP
 * dst  - IP destination (the group for a Report, 224.0.0.2 for a Leave)
 *
 * The datagram carries a Router Alert option (RFC 2113), TTL 1, and is forced
 * out of the group's interface with loopback disabled.
 * ------------------------------------------------------------------ */
static void
igmp_sendpkt(struct in_multi *inm, int type, struct in_addr dst)
{
	struct mbuf   *m;
	struct mbuf   *optm;
	struct igmp   *igmp;
	struct ip     *ip;
	struct ipoption *ipopt;
	struct ifnet  *ifp = inm->inm_ifp;
	struct ip_moptions imo;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;

	/*
	 * Reserve room at the front for the IP header, then build the IGMP
	 * message.  Building it at the mbuf front lets in_cksum() checksum
	 * exactly the IGMP payload before the IP header is exposed.
	 */
	m->m_data += sizeof(struct ip);
	m->m_len   = IGMP_MINLEN;

	igmp = mtod(m, struct igmp *);
	igmp->igmp_type  = type;
	igmp->igmp_code  = 0;
	igmp->igmp_group = inm->inm_addr;
	igmp->igmp_cksum = 0;
	igmp->igmp_cksum = in_cksum(m, IGMP_MINLEN);

	/* Expose the IP header ahead of the IGMP message. */
	m->m_data -= sizeof(struct ip);
	m->m_len  += sizeof(struct ip);
	m->m_pkthdr.len = m->m_len;

	ip = mtod(m, struct ip *);
	ip->ip_tos = 0;
	ip->ip_len = sizeof(struct ip) + IGMP_MINLEN;	/* host order */
	ip->ip_off = 0;
	ip->ip_p   = IPPROTO_IGMP;
	ip->ip_src.s_addr = INADDR_ANY;			/* filled by ip_output */
	ip->ip_dst = dst;
	ip->ip_ttl = 1;					/* IGMP requires TTL 1 */

	/*
	 * Build a Router Alert option (RFC 2113) in a separate mbuf and hand
	 * it to ip_output(), which inserts it and recomputes the header
	 * length.  The leading in_addr is the ipoption first-hop field (unused
	 * here); the four option bytes are {IPOPT_RA, len=4, value=0}.
	 */
	optm = NULL;
	MGET(optm, M_DONTWAIT, MT_DATA);
	if(optm != NULL) {
		ipopt = mtod(optm, struct ipoption *);
		ipopt->ipopt_dst.s_addr = INADDR_ANY;
		ipopt->ipopt_list[0] = (char)IPOPT_RA;
		ipopt->ipopt_list[1] = 4;
		ipopt->ipopt_list[2] = 0;
		ipopt->ipopt_list[3] = 0;
		optm->m_len = sizeof(struct in_addr) + 4;
	}

	D(bug("[AROSTCP:IGMP] %s: type=0x%02x on %s%d group=%08lx dst=%08lx\n",
	      __func__, type, ifp->if_name, ifp->if_unit,
	      (unsigned long)ntohl(inm->inm_addr.s_addr),
	      (unsigned long)ntohl(dst.s_addr)));

	/* Force the message out of the group's interface, TTL 1, no loopback. */
	bzero(&imo, sizeof(imo));
	imo.imo_multicast_ifp  = ifp;
	imo.imo_multicast_ttl  = 1;
	imo.imo_multicast_loop = 0;

	ip_output(m, optm, (struct route *)NULL, 0, &imo);

	/* ip_output() does not free the options mbuf. */
	if(optm != NULL)
		m_freem(optm);
}

#endif /* ENABLE_MULTICAST */
