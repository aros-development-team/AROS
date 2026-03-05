/*
 * Copyright (C) 2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME MLDv1 implementation (RFC 2710).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * mld6.c - Multicast Listener Discovery for IPv6 (RFC 2710).
 *
 * MLDv1 allows an IPv6 multicast router to discover the presence of
 * multicast listeners on its directly-attached links.  Hosts send
 * MLD Report messages when joining a group and MLD Done messages when
 * leaving.  Routers periodically send Query messages and hosts respond
 * with Reports for groups they belong to.
 *
 * State machine per in6_multi:
 *   MLD6_IDLE_MEMBER        - no report pending
 *   MLD6_LAZY_MEMBER        - timer pending for initial report
 *   MLD6_SLEEPING_MEMBER    - report sent, timer pending for query response
 *   MLD6_AWAKENING_MEMBER   - heard another's report, suppressed ours
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>
#include <stdarg.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6.h>

#include <protos/netinet/in_cksum_protos.h>

/* ip6_output uses varargs; forward declare here */
int ip6_output(void *, ...);

/*
 * Protocol constants (RFC 2710 §7).
 */
#define MLD_RV			2	/* Robustness Variable */
#define MLD_TIMER_SCALE		1000	/* ms → timer-tick divisor */
#define MLD_UNSOLICITED_REPORT_INTERVAL	10	/* seconds */

static int mld6_timer_running = 0;
static u_int32_t mld6_random_seed = 1;

/* Simple pseudo-random for timer jitter (no arc4random in kernel) */
static u_int32_t
mld6_random(void)
{
	mld6_random_seed = mld6_random_seed * 1103515245 + 12345;
	return (mld6_random_seed >> 16) & 0x7fff;
}

/* Forward declarations */
static void mld6_sendpkt(struct in6_multi *, int, const struct in6_addr *);
static int  mld6_timer_active(void);

/* ------------------------------------------------------------------
 * mld6_init - initialize MLD subsystem.
 * ------------------------------------------------------------------ */
void
mld6_init(void)
{
	mld6_timer_running = 0;
}

/* ------------------------------------------------------------------
 * mld6_start_listening - called when a host joins a multicast group.
 *
 * Sends an initial unsolicited MLD Report and starts the report timer
 * (RFC 2710 §5).  The all-nodes group (ff02::1) never reports.
 * ------------------------------------------------------------------ */
void
mld6_start_listening(struct in6_multi *in6m)
{
	struct in6_addr allnodes;

	/* ff02::1 — all-nodes: don't report */
	bzero(&allnodes, sizeof(allnodes));
	allnodes.s6_addr[0]  = 0xff;
	allnodes.s6_addr[1]  = 0x02;
	allnodes.s6_addr[15] = 0x01;

	if(IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &allnodes)) {
		in6m->in6m_state = MLD6_IDLE_MEMBER;
		in6m->in6m_timer = 0;
		return;
	}

	/* Send initial report immediately */
	mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);

	/* Start timer for the second unsolicited report */
	in6m->in6m_timer = MLD_UNSOLICITED_REPORT_INTERVAL;
	in6m->in6m_state = MLD6_LAZY_MEMBER;
	mld6_timer_running = 1;
}

/* ------------------------------------------------------------------
 * mld6_stop_listening - called when a host leaves a multicast group.
 *
 * Sends an MLD Done message (RFC 2710 §5).
 * The all-nodes group (ff02::1) never sends Done.
 * ------------------------------------------------------------------ */
void
mld6_stop_listening(struct in6_multi *in6m)
{
	struct in6_addr allnodes;

	/* ff02::1 — all-nodes: don't send Done */
	bzero(&allnodes, sizeof(allnodes));
	allnodes.s6_addr[0]  = 0xff;
	allnodes.s6_addr[1]  = 0x02;
	allnodes.s6_addr[15] = 0x01;

	if(IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &allnodes))
		return;

	/* Only send Done if we were the last reporter (RFC 2710 §6) */
	if(in6m->in6m_state == MLD6_IDLE_MEMBER ||
	   in6m->in6m_state == MLD6_LAZY_MEMBER) {
		mld6_sendpkt(in6m, MLD_LISTENER_DONE, NULL);
	}
}

/* ------------------------------------------------------------------
 * mld6_input - process an incoming MLD message.
 *
 * Called from icmp6_input() for MLD_LISTENER_QUERY, MLD_LISTENER_REPORT,
 * and MLD_LISTENER_DONE messages.
 *
 * m    - the inbound mbuf chain (icmp6 header at offset off)
 * off  - byte offset of the ICMPv6/MLD header
 * len  - length of the ICMPv6 payload
 * ------------------------------------------------------------------ */
void
mld6_input(struct mbuf *m, int off, int len)
{
	struct ip6_hdr *ip6;
	struct mld_hdr *mld;
	struct ifnet   *ifp;
	struct in6_multi *in6m;
	struct in6_addr maddr;

	if(len < (int)sizeof(struct mld_hdr)) {
		m_freem(m);
		return;
	}

	/* pull up MLD header */
	if(m->m_len < off + (int)sizeof(struct mld_hdr)) {
		m = m_pullup(m, off + sizeof(struct mld_hdr));
		if(m == NULL)
			return;
	}

	ip6 = mtod(m, struct ip6_hdr *);
	mld = (struct mld_hdr *)((u_int8_t *)ip6 + off);
	ifp = m->m_pkthdr.rcvif;
	maddr = mld->mld_addr;

	/* MLD messages must have hop limit = 1 (RFC 2710 §3) */
	if(ip6->ip6_hlim != 1) {
		D(bug("[AROSTCP:MLD6] %s: bad hop limit %d, dropping\n",
		      __func__, ip6->ip6_hlim));
		m_freem(m);
		return;
	}

	switch(mld->mld_type) {
	case MLD_LISTENER_QUERY:
	{
		/*
		 * Multicast Listener Query (RFC 2710 §5.1).
		 * If the query is for a specific group we belong to,
		 * start a report timer.  If it's a general query (all-zeros),
		 * start timers for all groups on this interface.
		 */
		int maxdelay = ntohs(mld->mld_maxdelay);
		if(maxdelay == 0)
			maxdelay = 1;	/* RFC 2710: treat 0 as 1 */

		if(IN6_IS_ADDR_UNSPECIFIED(&maddr)) {
			/* General query: set timer for every group on this interface */
			for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
				if(in6m->in6m_ifp != ifp)
					continue;
				if(in6m->in6m_timer == 0 ||
				   in6m->in6m_timer > maxdelay) {
					in6m->in6m_timer = 1 + (mld6_random() % maxdelay);
					in6m->in6m_state = MLD6_SLEEPING_MEMBER;
					mld6_timer_running = 1;
				}
			}
		} else {
			/* Group-specific query */
			for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
				if(in6m->in6m_ifp == ifp &&
				   IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &maddr)) {
					if(in6m->in6m_timer == 0 ||
					   in6m->in6m_timer > maxdelay) {
						in6m->in6m_timer = 1 + (mld6_random() % maxdelay);
						in6m->in6m_state = MLD6_SLEEPING_MEMBER;
						mld6_timer_running = 1;
					}
					break;
				}
			}
		}
		break;
	}

	case MLD_LISTENER_REPORT:
		/*
		 * Multicast Listener Report (RFC 2710 §5.2).
		 * If another host on the same link reported a group we
		 * also belong to, suppress our own pending report (timer
		 * suppression).
		 *
		 * Note: on AROS we don't loopback multicast by default,
		 * so we won't see our own reports here.  If we did,
		 * checking ip6->ip6_src against our own addresses would
		 * be needed.
		 */

		for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
			if(in6m->in6m_ifp == ifp &&
			   IN6_ARE_ADDR_EQUAL(&in6m->in6m_addr, &maddr)) {
				in6m->in6m_timer = 0;
				in6m->in6m_state = MLD6_IDLE_MEMBER;
				break;
			}
		}
		break;

	case MLD_LISTENER_DONE:
		/* Hosts ignore Done messages; only routers process them */
		break;
	}

	m_freem(m);
}

/* ------------------------------------------------------------------
 * mld6_fasttimeo - called from the fast timer (200 ms tick).
 *
 * Scans all multicast memberships looking for expired report timers
 * and sends MLD Report messages as required.
 * ------------------------------------------------------------------ */
void
mld6_fasttimeo(void)
{
	struct in6_multi *in6m;
	int any_running = 0;

	if(!mld6_timer_running)
		return;

	for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
		if(in6m->in6m_timer == 0)
			continue;

		if(--in6m->in6m_timer == 0) {
			/* Timer expired: send a report */
			mld6_sendpkt(in6m, MLD_LISTENER_REPORT, NULL);
			in6m->in6m_state = MLD6_IDLE_MEMBER;
		} else {
			any_running = 1;
		}
	}

	mld6_timer_running = any_running;
}

/* ------------------------------------------------------------------
 * mld6_sendpkt - send an MLD message (Report or Done).
 *
 * type - MLD_LISTENER_REPORT or MLD_LISTENER_DONE
 * dst  - destination override (NULL = use standard destination)
 *
 * For Reports the destination is the multicast group address itself.
 * For Done the destination is the all-routers multicast address (ff02::2).
 * ------------------------------------------------------------------ */
static void
mld6_sendpkt(struct in6_multi *in6m, int type, const struct in6_addr *dst)
{
	struct mbuf    *m;
	struct mld_hdr *mld;
	struct ip6_hdr *ip6;
	struct ifnet   *ifp = in6m->in6m_ifp;
	struct in6_ifaddr *ia;
	int hdrlen;

	hdrlen = sizeof(struct ip6_hdr) + sizeof(struct mld_hdr);

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;
	m->m_pkthdr.len = m->m_len = hdrlen;

	/* Build IPv6 header */
	ip6 = mtod(m, struct ip6_hdr *);
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_plen = htons(sizeof(struct mld_hdr));
	ip6->ip6_nxt  = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 1;		/* MLD requires hop limit = 1 */

	/* Source: link-local address of the interface */
	{
		struct in6_addr dst_tmp = in6m->in6m_addr;
		ia = in6_ifawithifp(ifp, &dst_tmp);
	}
	if(ia)
		ip6->ip6_src = ia->ia_addr.sin6_addr;
	/* else: unspecified (valid per RFC 2710 §4 for initial report) */

	/* Destination address */
	if(dst != NULL) {
		ip6->ip6_dst = *dst;
	} else if(type == MLD_LISTENER_REPORT) {
		/* Report: destination is the multicast group */
		ip6->ip6_dst = in6m->in6m_addr;
	} else {
		/* Done: destination is all-routers (ff02::2) */
		bzero(&ip6->ip6_dst, sizeof(ip6->ip6_dst));
		ip6->ip6_dst.s6_addr[0]  = 0xff;
		ip6->ip6_dst.s6_addr[1]  = 0x02;
		ip6->ip6_dst.s6_addr[15] = 0x02;
	}

	/* Build MLD header */
	mld = (struct mld_hdr *)(ip6 + 1);
	bzero(mld, sizeof(*mld));
	mld->mld_type = type;
	mld->mld_addr = in6m->in6m_addr;

	/* Compute ICMPv6 checksum */
	mld->mld_cksum = 0;
	mld->mld_cksum = in6_cksum(m, IPPROTO_ICMPV6,
	                           sizeof(struct ip6_hdr),
	                           sizeof(struct mld_hdr));

	D(bug("[AROSTCP:MLD6] %s: sending type=%d on %s%d for %02x%02x:...:%02x%02x\n",
	      __func__, type, ifp->if_name, ifp->if_unit,
	      in6m->in6m_addr.s6_addr[0], in6m->in6m_addr.s6_addr[1],
	      in6m->in6m_addr.s6_addr[14], in6m->in6m_addr.s6_addr[15]));

	ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * mld6_timer_active - return nonzero if any MLD timer is running.
 * ------------------------------------------------------------------ */
static int
mld6_timer_active(void)
{
	return mld6_timer_running;
}

#endif /* INET6 */
