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

/*
 * MLDv2 (RFC 3810) host-side support.
 *
 * MLDv2 is added incrementally alongside the existing MLDv1 code:
 * an MLDv2 Query is recognised by its message length (an MLDv1 Query is
 * exactly the 24-byte struct mld_hdr; an MLDv2 Query is at least 28
 * bytes), and is answered with an MLDv2 Membership Report (ICMPv6 type
 * 143).  MLDv1 Queries continue to be answered with MLDv1 Reports, so a
 * v1 querier still sees v1 responses (RFC 3810 §8).
 *
 * Since per-source filtering is not tracked, each joined group is
 * reported with a single MODE_IS_EXCLUDE record carrying no sources,
 * which is equivalent to a plain "listening to this group" membership.
 */

/* Multicast Address Record types (RFC 3810 §5.2.12) */
#define MLD_MODE_IS_INCLUDE	1
#define MLD_MODE_IS_EXCLUDE	2

/*
 * Extra per-group timer state used to remember that the pending response
 * should be an MLDv2 report rather than an MLDv1 report.  It extends the
 * MLD6_*_MEMBER states from <netinet6/mld6.h> (which occupy 0..3).
 */
#define MLD6_V2_PENDING		4	/* v2 query-response timer running */

/* MLDv2 Multicast Address Record (RFC 3810 §5.2), no sources / aux data */
struct mldv2_record {
	u_int8_t	mr_type;	/* record type */
	u_int8_t	mr_datalen;	/* auxiliary data length (octets) */
	u_int16_t	mr_numsrc;	/* number of sources */
	struct in6_addr	mr_addr;	/* multicast address */
	/* followed by source addresses and auxiliary data */
} __packed;

static int mld6_timer_running = 0;
static u_int32_t mld6_random_seed = 1;

/* Simple pseudo-random for timer jitter (no arc4random in kernel) */
static u_int32_t
mld6_random(void)
{
	mld6_random_seed = mld6_random_seed * 1103515245 + 12345;
	return (mld6_random_seed >> 16) & 0x7fff;
}

/*
 * Every MLD message is sent with an IPv6 Hop-by-Hop Options header carrying
 * the Router Alert option (RFC 2711), so that multicast routers intercept it
 * even though it is addressed to a group they are not a member of.  The header
 * is a fixed 8 octets: the 2-byte Hop-by-Hop header, the 4-byte Router Alert
 * option, and a 2-byte PadN(0) to round up to the required 8-octet multiple.
 */
#define MLD6_RA_HBHLEN	8

/* Forward declarations */
static void mld6_sendpkt(struct in6_multi *, int, const struct in6_addr *);
static void mld6_sendpkt_v2(struct in6_multi *);
static int  mld6_timer_active(void);

/* ------------------------------------------------------------------
 * mld6_prepend_ra - write the Router Alert Hop-by-Hop Options header
 * immediately after the IPv6 header, set ip6_nxt to IPPROTO_HOPOPTS, and
 * return a pointer to the first byte after it (where the ICMPv6/MLD message
 * begins).  MLD6_RA_HBHLEN bytes must already be reserved between the IPv6
 * header and the message.
 * ------------------------------------------------------------------ */
static u_int8_t *
mld6_prepend_ra(struct ip6_hdr *ip6)
{
	struct ip6_hbh        *hbh = (struct ip6_hbh *)(ip6 + 1);
	struct ip6_opt_router *ra  = (struct ip6_opt_router *)(hbh + 1);
	u_int8_t              *pad = (u_int8_t *)(ra + 1);
	u_int16_t              rav = IP6_ALERT_MLD;	/* network byte order */

	hbh->ip6h_nxt = IPPROTO_ICMPV6;
	hbh->ip6h_len = 0;			/* (0 + 1) * 8 = 8 octets */

	ra->ip6or_type = IP6OPT_ROUTER_ALERT;
	ra->ip6or_len  = sizeof(ra->ip6or_value);	/* option data length = 2 */
	bcopy(&rav, ra->ip6or_value, sizeof(rav));

	/* PadN(0) fills the remaining 2 octets of the 8-octet header */
	pad[0] = IP6OPT_PADN;
	pad[1] = 0;

	ip6->ip6_nxt = IPPROTO_HOPOPTS;
	return pad + 2;
}

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

	/* Start timer for the second unsolicited report.  The interval is in
	 * seconds but in6m_timer counts fast-timer ticks (PR_FASTHZ per second). */
	in6m->in6m_timer = MLD_UNSOLICITED_REPORT_INTERVAL * PR_FASTHZ;
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
		 * Multicast Listener Query (RFC 2710 §5.1 / RFC 3810 §5.1).
		 * If the query is for a specific group we belong to,
		 * start a report timer.  If it's a general query (all-zeros),
		 * start timers for all groups on this interface.
		 *
		 * An MLDv2 Query is distinguished from an MLDv1 Query by its
		 * length: the MLDv1 form is exactly struct mld_hdr (24 bytes),
		 * whereas an MLDv2 Query carries additional fields.  When the
		 * querier speaks MLDv2 the pending response is flagged so that
		 * an MLDv2 Report is generated at timer expiry; otherwise an
		 * MLDv1 Report is generated for backward compatibility.
		 */
		int mldv2 = (len > (int)sizeof(struct mld_hdr));
		int maxdelay = ntohs(mld->mld_maxdelay);
		u_int newstate;

		/*
		 * MLDv2 encodes a large Maximum Response Code as a
		 * floating-point value (RFC 3810 §5.1.3).
		 */
		if(mldv2 && maxdelay >= 32768) {
			int mant = maxdelay & 0x0fff;
			int exp  = (maxdelay >> 12) & 0x7;
			maxdelay = (mant | 0x1000) << (exp + 3);
		}
		if(maxdelay == 0)
			maxdelay = 1;	/* RFC 2710: treat 0 as 1 */
		/*
		 * The Max Response value is in milliseconds, but in6m_timer
		 * counts fast-timer ticks (PR_FASTHZ ticks per second).
		 * Convert from ms to ticks, keeping a minimum of one tick.
		 */
		maxdelay = (maxdelay * PR_FASTHZ) / 1000;
		if(maxdelay == 0)
			maxdelay = 1;

		newstate = mldv2 ? MLD6_V2_PENDING : MLD6_SLEEPING_MEMBER;

		if(IN6_IS_ADDR_UNSPECIFIED(&maddr)) {
			/* General query: set timer for every group on this interface */
			for(in6m = in6_multihead; in6m; in6m = in6m->in6m_next) {
				if(in6m->in6m_ifp != ifp)
					continue;
				if(in6m->in6m_timer == 0 ||
				   in6m->in6m_timer > maxdelay) {
					in6m->in6m_timer = 1 + (mld6_random() % maxdelay);
					in6m->in6m_state = newstate;
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
						in6m->in6m_state = newstate;
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
			/* Timer expired: send a report using the querier's version */
			if(in6m->in6m_state == MLD6_V2_PENDING)
				mld6_sendpkt_v2(in6m);
			else
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
	struct ip6_moptions im6o;
	int hdrlen;

	hdrlen = sizeof(struct ip6_hdr) + MLD6_RA_HBHLEN + sizeof(struct mld_hdr);

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;
	m->m_pkthdr.len = m->m_len = hdrlen;

	/* Build IPv6 header */
	ip6 = mtod(m, struct ip6_hdr *);
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_plen = htons(MLD6_RA_HBHLEN + sizeof(struct mld_hdr));
	ip6->ip6_hlim = 1;		/* MLD requires hop limit = 1 */
	/* ip6_nxt is set to IPPROTO_HOPOPTS by mld6_prepend_ra() below */

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

	/* Router Alert Hop-by-Hop header, then the MLD message after it */
	mld = (struct mld_hdr *)mld6_prepend_ra(ip6);
	bzero(mld, sizeof(*mld));
	mld->mld_type = type;
	mld->mld_addr = in6m->in6m_addr;

	/* Compute ICMPv6 checksum (over the MLD message, past the HBH header) */
	mld->mld_cksum = 0;
	mld->mld_cksum = in6_cksum(m, IPPROTO_ICMPV6,
	                           sizeof(struct ip6_hdr) + MLD6_RA_HBHLEN,
	                           sizeof(struct mld_hdr));

	D(bug("[AROSTCP:MLD6] %s: sending type=%d on %s%d for %02x%02x:...:%02x%02x\n",
	      __func__, type, ifp->if_name, ifp->if_unit,
	      in6m->in6m_addr.s6_addr[0], in6m->in6m_addr.s6_addr[1],
	      in6m->in6m_addr.s6_addr[14], in6m->in6m_addr.s6_addr[15]));

	/* force the MLD message out of the group's interface */
	bzero(&im6o, sizeof(im6o));
	im6o.im6o_multicast_ifp  = ifp;
	im6o.im6o_multicast_hlim = 1;		/* MLD requires hop limit = 1 */

	ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0, &im6o,
	           (struct ifnet **)NULL, (struct inpcb *)NULL);
}

/* ------------------------------------------------------------------
 * mld6_sendpkt_v2 - send an MLDv2 Membership Report (RFC 3810 §5.2).
 *
 * Emits an ICMPv6 type 143 report containing a single MODE_IS_EXCLUDE
 * Multicast Address Record (with no sources), which is equivalent to a
 * plain membership in the group.  The report is addressed to the
 * all-MLDv2-capable-routers multicast address ff02::16.  The message is sent
 * with a Router Alert Hop-by-Hop header (mld6_prepend_ra).
 * ------------------------------------------------------------------ */
static void
mld6_sendpkt_v2(struct in6_multi *in6m)
{
	struct mbuf      *m;
	struct ip6_hdr   *ip6;
	struct icmp6_hdr *icmp6;
	struct mldv2_record *rec;
	struct ifnet     *ifp = in6m->in6m_ifp;
	struct in6_ifaddr *ia;
	struct ip6_moptions im6o;
	int reportlen, hdrlen;

	/* header + one record (no sources, no auxiliary data) */
	reportlen = sizeof(struct icmp6_hdr) + sizeof(struct mldv2_record);
	hdrlen    = sizeof(struct ip6_hdr) + MLD6_RA_HBHLEN + reportlen;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if(m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;
	m->m_pkthdr.len = m->m_len = hdrlen;

	/* Build IPv6 header */
	ip6 = mtod(m, struct ip6_hdr *);
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_plen = htons(MLD6_RA_HBHLEN + reportlen);
	ip6->ip6_hlim = 1;		/* MLD requires hop limit = 1 */
	/* ip6_nxt is set to IPPROTO_HOPOPTS by mld6_prepend_ra() below */

	/* Source: link-local address of the interface */
	{
		struct in6_addr dst_tmp = in6m->in6m_addr;
		ia = in6_ifawithifp(ifp, &dst_tmp);
	}
	if(ia)
		ip6->ip6_src = ia->ia_addr.sin6_addr;

	/* Destination: all MLDv2-capable routers (ff02::16) */
	bzero(&ip6->ip6_dst, sizeof(ip6->ip6_dst));
	ip6->ip6_dst.s6_addr[0]  = 0xff;
	ip6->ip6_dst.s6_addr[1]  = 0x02;
	ip6->ip6_dst.s6_addr[15] = 0x16;

	/* Router Alert Hop-by-Hop header, then the MLDv2 report after it */
	icmp6 = (struct icmp6_hdr *)mld6_prepend_ra(ip6);
	bzero(icmp6, sizeof(*icmp6));
	icmp6->icmp6_type = MLDV2_LISTENER_REPORT;
	icmp6->icmp6_data16[0] = 0;		/* reserved */
	icmp6->icmp6_data16[1] = htons(1);	/* one multicast address record */

	/* Single MODE_IS_EXCLUDE record: no sources = plain membership */
	rec = (struct mldv2_record *)(icmp6 + 1);
	bzero(rec, sizeof(*rec));
	rec->mr_type    = MLD_MODE_IS_EXCLUDE;
	rec->mr_datalen = 0;
	rec->mr_numsrc  = 0;
	rec->mr_addr    = in6m->in6m_addr;

	/* Compute ICMPv6 checksum (over the report, past the HBH header) */
	icmp6->icmp6_cksum = 0;
	icmp6->icmp6_cksum = in6_cksum(m, IPPROTO_ICMPV6,
	                               sizeof(struct ip6_hdr) + MLD6_RA_HBHLEN,
	                               reportlen);

	D(bug("[AROSTCP:MLD6] %s: sending v2 report on %s%d for %02x%02x:...:%02x%02x\n",
	      __func__, ifp->if_name, ifp->if_unit,
	      in6m->in6m_addr.s6_addr[0], in6m->in6m_addr.s6_addr[1],
	      in6m->in6m_addr.s6_addr[14], in6m->in6m_addr.s6_addr[15]));

	/* force the report out of the group's interface */
	bzero(&im6o, sizeof(im6o));
	im6o.im6o_multicast_ifp  = ifp;
	im6o.im6o_multicast_hlim = 1;		/* MLD requires hop limit = 1 */

	ip6_output(m, (struct mbuf *)NULL, (struct route *)NULL, 0, &im6o,
	           (struct ifnet **)NULL, (struct inpcb *)NULL);
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
