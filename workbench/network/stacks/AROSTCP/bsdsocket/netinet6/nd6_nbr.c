/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME nd6_nbr.c:
 * Copyright (C) 1995-1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * nd6_nbr.c - IPv6 Neighbor Solicitation / Advertisement + DAD.
 *
 * Implements:
 *   - Neighbor Solicitation input/output (RFC 4861 §7.2)
 *   - Neighbor Advertisement input/output (RFC 4861 §7.2)
 *   - Duplicate Address Detection (RFC 4862 §5.4)
 *   - Link-layer address caching with proper state machine
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

/* Forward declarations */
struct rtentry *rtalloc1(struct sockaddr *, int);
int ip6_output(void *, ...);

/* DAD queue — singly-linked list */
static struct dadq *dadq_head = NULL;

/* ------------------------------------------------------------------
 * ICMPv6 pseudo-header checksum for NDP messages.
 *
 * RFC 4861 requires hop-limit=255 and proper ICMPv6 checksums.
 * ------------------------------------------------------------------ */
static u_int16_t
nd6_cksum(struct ip6_hdr *ip6, void *icmp6, int icmp6len)
{
	u_int32_t sum = 0;
	u_int16_t *p;
	int i;

	/* pseudo-header: src + dst + length + next-header */
	{
		struct in6_addr addrs[2];
		memcpy(&addrs[0], &ip6->ip6_src, sizeof(struct in6_addr));
		memcpy(&addrs[1], &ip6->ip6_dst, sizeof(struct in6_addr));
		p = (u_int16_t *)addrs;
		for (i = 0; i < 16; i++)	/* 32 bytes of addresses */
			sum += ntohs(p[i]);
	}

	sum += (u_int32_t)icmp6len;
	sum += IPPROTO_ICMPV6;

	/* ICMPv6 payload */
	p = (u_int16_t *)icmp6;
	for (i = 0; i < icmp6len / 2; i++)
		sum += ntohs(p[i]);
	if (icmp6len & 1)
		sum += ((u_int8_t *)icmp6)[icmp6len - 1] << 8;

	/* fold */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return htons((u_int16_t)~sum);
}

/* ------------------------------------------------------------------
 * nd6_ns_output - send a Neighbor Solicitation.
 *
 * src  - source address (NULL = use link-local of ifp)
 * tgt  - target address being resolved
 * ln   - neighbor cache entry (may be NULL for DAD)
 * dad  - non-zero if this is a DAD probe (src = ::)
 * ------------------------------------------------------------------ */
void
nd6_ns_output(struct ifnet *ifp, struct in6_addr *src,
              struct in6_addr *tgt, struct llinfo_nd6 *ln, int dad)
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct nd_neighbor_solicit *nd_ns;
	struct nd_opt_hdr *nd_opt;
	struct sockaddr_in6 dst_sa;
	int hlen = sizeof(struct ip6_hdr);
	int icmp6len = sizeof(struct nd_neighbor_solicit);
	int optlen = 0;
	caddr_t mac;
	int pktlen;

	/* include Source Link-Layer Address option unless DAD */
	mac = nd6_ifptomac(ifp);
	if (!dad && mac && ifp->if_addrlen == 6)
		optlen = 8;		/* type(1) + len(1) + MAC(6) */

	pktlen = hlen + icmp6len + optlen;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;

	if (pktlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return;
		}
	}

	m->m_pkthdr.len = m->m_len = pktlen;
	m->m_pkthdr.rcvif = NULL;

	ip6   = mtod(m, struct ip6_hdr *);
	nd_ns = (struct nd_neighbor_solicit *)(ip6 + 1);

	/* IPv6 header */
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_nxt  = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 255;			/* required by RFC 4861 */
	ip6->ip6_plen = htons(icmp6len + optlen);

	/* destination: solicited-node multicast ff02::1:ffXX:XXXX */
	bzero(&ip6->ip6_dst, sizeof(ip6->ip6_dst));
	ip6->ip6_dst.s6_addr[0]  = 0xff;
	ip6->ip6_dst.s6_addr[1]  = 0x02;
	ip6->ip6_dst.s6_addr[11] = 0x01;
	ip6->ip6_dst.s6_addr[12] = 0xff;
	ip6->ip6_dst.s6_addr[13] = tgt->s6_addr[13];
	ip6->ip6_dst.s6_addr[14] = tgt->s6_addr[14];
	ip6->ip6_dst.s6_addr[15] = tgt->s6_addr[15];

	/* source address */
	if (dad) {
		bzero(&ip6->ip6_src, sizeof(ip6->ip6_src));
	} else if (src) {
		ip6->ip6_src = *src;
	} else {
		struct in6_ifaddr *ia = in6_ifaof_ifpforlinklocal(ifp);
		if (ia)
			ip6->ip6_src = ia->ia_addr.sin6_addr;
	}

	/* ICMPv6 Neighbor Solicitation */
	bzero(nd_ns, sizeof(*nd_ns));
	nd_ns->nd_ns_type   = ND_NEIGHBOR_SOLICIT;
	nd_ns->nd_ns_target = *tgt;

	/* Source Link-Layer Address option */
	if (optlen) {
		nd_opt = (struct nd_opt_hdr *)(nd_ns + 1);
		nd_opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd_opt->nd_opt_len  = 1;	/* units of 8 bytes */
		bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
	}

	/* ICMPv6 checksum */
	nd_ns->nd_ns_cksum = 0;
	nd_ns->nd_ns_cksum = nd6_cksum(ip6, nd_ns, icmp6len + optlen);

	/* build destination sockaddr for ip6_output */
	bzero(&dst_sa, sizeof(dst_sa));
	dst_sa.sin6_family = AF_INET6;
	dst_sa.sin6_len    = sizeof(dst_sa);
	dst_sa.sin6_addr   = ip6->ip6_dst;

	nd6stat.nd6s_snd_ns++;
	ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * nd6_ns_input - process an inbound Neighbor Solicitation.
 *
 * Validates the NS per RFC 4861 §7.1.1, updates neighbor cache
 * with source link-layer address, and sends NA in response.
 * ------------------------------------------------------------------ */
void
nd6_ns_input(struct mbuf *m, int off, int icmp6len)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_neighbor_solicit *nd_ns;
	struct in6_addr tgt;
	struct in6_ifaddr *ia;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	union nd_opts ndopts;
	int is_dad;

	nd6stat.nd6s_rcv_ns++;

	/* validate hop limit */
	if (ip6->ip6_hlim != 255) {
		nd6stat.nd6s_rcv_badns++;
		goto bad;
	}

	/* pull up NS header */
	if (m->m_len < off + (int)sizeof(*nd_ns)) {
		if ((m = m_pullup(m, off + sizeof(*nd_ns))) == NULL) {
			nd6stat.nd6s_rcv_badns++;
			return;
		}
		ip6 = mtod(m, struct ip6_hdr *);
	}

	nd_ns = (struct nd_neighbor_solicit *)(mtod(m, u_int8_t *) + off);
	tgt = nd_ns->nd_ns_target;

	/* target must not be multicast (RFC 4861 §7.1.1) */
	if (IN6_IS_ADDR_MULTICAST(&tgt)) {
		nd6stat.nd6s_rcv_badns++;
		goto bad;
	}

	/* DAD detection: source is unspecified */
	is_dad = IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src);

	/* parse options */
	nd6_option_init(nd_ns + 1,
	    icmp6len - (int)sizeof(*nd_ns), &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6stat.nd6s_rcv_badns++;
		goto bad;
	}

	/* DAD NS must not contain SLLA option (RFC 4861 §7.1.1) */
	if (is_dad && ndopts.nd_opts_src_lladdr) {
		nd6stat.nd6s_rcv_badns++;
		goto bad;
	}

	/* is the target one of our addresses? */
	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (ia->ia6_ifp == ifp &&
		    IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, &tgt))
			goto found;
	}
	/* target not ours */
	goto bad;

found:
	/* if target address is tentative, this is a DAD conflict */
	if (ia->ia6_ifaflags & IN6_IFF_TENTATIVE) {
		nd6_dad_ns_input(ia);
		goto bad;
	}

	/* update source's neighbor cache if SLLA option present */
	if (ndopts.nd_opts_src_lladdr && !is_dad) {
		char *lladdr = (char *)(ndopts.nd_opts_src_lladdr + 1);
		int lladdrlen = ifp->if_addrlen;
		struct in6_addr src;
		memcpy(&src, &ip6->ip6_src, sizeof(src));
		nd6_cache_lladdr(ifp, &src, lladdr, lladdrlen,
		    ND_NEIGHBOR_SOLICIT, 0);
	}

	/* send Neighbor Advertisement in response */
	if (is_dad) {
		/* reply to all-nodes multicast for DAD */
		struct in6_addr allnodes;
		bzero(&allnodes, sizeof(allnodes));
		allnodes.s6_addr[0]  = 0xff;
		allnodes.s6_addr[1]  = 0x02;
		allnodes.s6_addr[15] = 0x01;
		nd6_na_output(ifp, &allnodes, &tgt,
		    ND_NA_FLAG_OVERRIDE, 1, NULL);
	} else {
		struct in6_addr src;
		memcpy(&src, &ip6->ip6_src, sizeof(src));
		nd6_na_output(ifp, &src, &tgt,
		    ND_NA_FLAG_SOLICITED | ND_NA_FLAG_OVERRIDE, 1, NULL);
	}

	m_freem(m);
	return;

bad:
	m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_na_output - send a Neighbor Advertisement.
 *
 * dst    - destination (unicast reply or all-nodes multicast)
 * tgt    - target address
 * flags  - ND_NA_FLAG_* (SOLICITED, ROUTER, OVERRIDE)
 * tlladdr - include Target Link-Layer Address option
 * ------------------------------------------------------------------ */
void
nd6_na_output(struct ifnet *ifp, struct in6_addr *dst,
              struct in6_addr *tgt, u_long flags, int tlladdr,
              struct sockaddr *sdl0)
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct nd_neighbor_advert *nd_na;
	struct nd_opt_hdr *nd_opt;
	caddr_t mac;
	int icmp6len = sizeof(*nd_na);
	int optlen   = 0;
	int hlen     = sizeof(struct ip6_hdr);
	int pktlen;

	mac = nd6_ifptomac(ifp);
	if (tlladdr && mac && ifp->if_addrlen == 6)
		optlen = 8;

	pktlen = hlen + icmp6len + optlen;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;

	if (pktlen > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return;
		}
	}

	m->m_pkthdr.len = m->m_len = pktlen;
	m->m_pkthdr.rcvif = NULL;

	ip6   = mtod(m, struct ip6_hdr *);
	nd_na = (struct nd_neighbor_advert *)(ip6 + 1);

	/* IPv6 header */
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_nxt  = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 255;
	ip6->ip6_plen = htons(icmp6len + optlen);
	ip6->ip6_dst  = *dst;

	/* source: our link-local */
	{
		struct in6_ifaddr *ia = in6_ifaof_ifpforlinklocal(ifp);
		if (ia)
			ip6->ip6_src = ia->ia_addr.sin6_addr;
	}

	/* ICMPv6 NA */
	bzero(nd_na, sizeof(*nd_na));
	nd_na->nd_na_type = ND_NEIGHBOR_ADVERT;
	nd_na->nd_na_flags_reserved = htonl(flags);
	nd_na->nd_na_target = *tgt;

	/* Target Link-Layer Address option */
	if (optlen) {
		nd_opt = (struct nd_opt_hdr *)(nd_na + 1);
		nd_opt->nd_opt_type = ND_OPT_TARGET_LINKADDR;
		nd_opt->nd_opt_len  = 1;
		bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
	}

	/* ICMPv6 checksum */
	nd_na->nd_na_cksum = 0;
	nd_na->nd_na_cksum = nd6_cksum(ip6, nd_na, icmp6len + optlen);

	nd6stat.nd6s_snd_na++;
	ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------
 * nd6_na_input - process an inbound Neighbor Advertisement.
 *
 * Implements RFC 4861 §7.2.5 state machine updates.
 * ------------------------------------------------------------------ */
void
nd6_na_input(struct mbuf *m, int off, int icmp6len)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_neighbor_advert *nd_na;
	struct in6_addr tgt;
	struct rtentry *rt;
	struct llinfo_nd6 *ln;
	struct sockaddr_dl *sdl;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct mbuf *held;
	union nd_opts ndopts;
	char *lladdr = NULL;
	int lladdrlen = 0;
	u_int32_t flags;
	int is_override, is_solicited, is_router;

	nd6stat.nd6s_rcv_na++;

	/* validate hop limit */
	if (ip6->ip6_hlim != 255) {
		nd6stat.nd6s_rcv_badna++;
		goto bad;
	}

	/* pull up NA header */
	if (m->m_len < off + (int)sizeof(*nd_na)) {
		if ((m = m_pullup(m, off + sizeof(*nd_na))) == NULL) {
			nd6stat.nd6s_rcv_badna++;
			return;
		}
		ip6 = mtod(m, struct ip6_hdr *);
	}

	nd_na = (struct nd_neighbor_advert *)(mtod(m, u_int8_t *) + off);
	tgt = nd_na->nd_na_target;
	flags = ntohl(nd_na->nd_na_flags_reserved);

	is_solicited = (flags & ND_NA_FLAG_SOLICITED) != 0;
	is_override  = (flags & ND_NA_FLAG_OVERRIDE) != 0;
	is_router    = (flags & ND_NA_FLAG_ROUTER) != 0;

	/* target must not be multicast */
	if (IN6_IS_ADDR_MULTICAST(&tgt)) {
		nd6stat.nd6s_rcv_badna++;
		goto bad;
	}

	/* if destination is multicast, solicited flag must be zero */
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) && is_solicited) {
		nd6stat.nd6s_rcv_badna++;
		goto bad;
	}

	/* check if target is a tentative address (DAD) */
	{
		struct in6_ifaddr *ia;
		for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
			if (ia->ia6_ifp == ifp &&
			    (ia->ia6_ifaflags & IN6_IFF_TENTATIVE) &&
			    IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr, &tgt)) {
				/* DAD failure: someone else has this address */
				ia->ia6_ifaflags |= IN6_IFF_DUPLICATED;
				ia->ia6_ifaflags &= ~IN6_IFF_TENTATIVE;
				nd6_dad_stop(ia);
				nd6stat.nd6s_dad_dup++;
				__log(LOG_ERR,
				    "nd6_na_input: DAD duplicate for %s on %s%d\n",
				    "IPv6 address", ifp->if_name, ifp->if_unit);
				goto bad;
			}
		}
	}

	/* parse options */
	nd6_option_init(nd_na + 1,
	    icmp6len - (int)sizeof(*nd_na), &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6stat.nd6s_rcv_badna++;
		goto bad;
	}

	if (ndopts.nd_opts_tgt_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_tgt_lladdr + 1);
		lladdrlen = ifp->if_addrlen;
	}

	/* look up neighbor cache entry */
	rt = nd6_lookup(&tgt, 0, ifp);
	if (rt == NULL || (ln = (struct llinfo_nd6 *)rt->rt_llinfo) == NULL) {
		/* no existing entry — if this is a router, create one */
		if (is_router) {
			rt = nd6_lookup(&tgt, 1, ifp);
			if (rt && rt->rt_llinfo == NULL) {
				MALLOC(ln, struct llinfo_nd6 *, sizeof(*ln),
				    M_PCB, M_NOWAIT);
				if (ln) {
					bzero(ln, sizeof(*ln));
					rt->rt_llinfo = (caddr_t)ln;
					ln->ln_rt = rt;
					ln->ln_state = ND6_LLINFO_STALE;
					ln->ln_next = llinfo_nd6.ln_next;
					ln->ln_prev = &llinfo_nd6;
					llinfo_nd6.ln_next->ln_prev = ln;
					llinfo_nd6.ln_next = ln;
				}
			}
		}
		goto bad;
	}

	/*
	 * RFC 4861 §7.2.5 — update neighbor cache based on NA.
	 */
	sdl = (rt->rt_gateway && rt->rt_gateway->sa_family == AF_LINK)
	    ? (struct sockaddr_dl *)rt->rt_gateway : NULL;

	if (ln->ln_state == ND6_LLINFO_INCOMPLETE) {
		/* entry was waiting for address resolution */
		if (lladdr == NULL)
			goto bad;	/* need TLLA for INCOMPLETE */

		if (sdl && lladdrlen > 0) {
			bcopy(lladdr, LLADDR(sdl), lladdrlen);
			sdl->sdl_alen = lladdrlen;
		}
		if (is_solicited)
			ln->ln_state = ND6_LLINFO_REACHABLE;
		else
			ln->ln_state = ND6_LLINFO_STALE;

		{
			struct timeval _tv;
			GetSysTime(&_tv);
			ln->ln_expire = _tv.tv_sec + ND6_REACHABLE_TIME / 1000;
		}
		ln->ln_router = is_router;

		/* send queued packet */
		held = ln->ln_hold;
		ln->ln_hold = NULL;
		if (held && sdl && sdl->sdl_alen > 0) {
			struct sockaddr_in6 dst_sa;
			bzero(&dst_sa, sizeof(dst_sa));
			dst_sa.sin6_family = AF_INET6;
			dst_sa.sin6_len    = sizeof(dst_sa);
			dst_sa.sin6_addr   = tgt;
			(*ifp->if_output)(ifp, held,
			    (struct sockaddr *)&dst_sa, rt);
		} else if (held) {
			m_freem(held);
		}
	} else {
		/* existing entry in REACHABLE/STALE/DELAY/PROBE */
		int llchange = 0;

		if (lladdr && sdl && sdl->sdl_alen > 0)
			llchange = bcmp(lladdr, LLADDR(sdl), lladdrlen);

		if (is_override || !llchange) {
			/* update link-layer address */
			if (lladdr && sdl && lladdrlen > 0) {
				bcopy(lladdr, LLADDR(sdl), lladdrlen);
				sdl->sdl_alen = lladdrlen;
			}
			if (is_solicited) {
				ln->ln_state = ND6_LLINFO_REACHABLE;
				struct timeval _tv;
				GetSysTime(&_tv);
				ln->ln_expire = _tv.tv_sec +
				    ND6_REACHABLE_TIME / 1000;
			} else if (llchange) {
				ln->ln_state = ND6_LLINFO_STALE;
			}
		} else {
			/* not override and address changed */
			if (is_solicited && ln->ln_state == ND6_LLINFO_REACHABLE)
				ln->ln_state = ND6_LLINFO_STALE;
		}

		/* router flag change handling (RFC 4861 §7.2.5 last para) */
		if (ln->ln_router && !is_router) {
			/* neighbor is no longer a router */
			ln->ln_router = 0;
		} else {
			ln->ln_router = is_router;
		}
	}

	m_freem(m);
	return;

bad:
	m_freem(m);
}

/* ==================================================================
 * Duplicate Address Detection (DAD) — RFC 4862 §5.4
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_dad_start - begin DAD for a tentative address.
 * ------------------------------------------------------------------ */
void
nd6_dad_start(struct in6_ifaddr *ia)
{
	struct dadq *dp;
	struct ifnet *ifp;
	struct timeval _tv;

	if (ia == NULL)
		return;

	ifp = ia->ia6_ifp;

	/* mark address as tentative */
	ia->ia6_ifaflags |= IN6_IFF_TENTATIVE;
	ia->ia6_ifaflags &= ~IN6_IFF_DUPLICATED;

	/* check if already in DAD queue */
	for (dp = dadq_head; dp; dp = dp->next) {
		if (dp->ia == ia)
			return;		/* already running DAD */
	}

	/* allocate DAD queue entry */
	MALLOC(dp, struct dadq *, sizeof(*dp), M_PCB, M_NOWAIT);
	if (dp == NULL) {
		/* can't do DAD, just mark as ready */
		ia->ia6_ifaflags &= ~IN6_IFF_TENTATIVE;
		return;
	}
	bzero(dp, sizeof(*dp));
	dp->ia  = ia;
	dp->ifp = ifp;
	dp->count = 0;
	dp->ns_icount = 0;
	dp->ns_ocount = 0;

	/* schedule first probe after a random delay (0-1s) */
	GetSysTime(&_tv);
	dp->timer = _tv.tv_sec + 1;	/* 1 second delay */

	/* insert into DAD queue */
	dp->next = dadq_head;
	dadq_head = dp;
}

/* ------------------------------------------------------------------
 * nd6_dad_stop - abort DAD for an address (e.g., address removed).
 * ------------------------------------------------------------------ */
void
nd6_dad_stop(struct in6_ifaddr *ia)
{
	struct dadq **pp, *dp;

	for (pp = &dadq_head; (dp = *pp) != NULL; pp = &dp->next) {
		if (dp->ia == ia) {
			*pp = dp->next;
			FREE(dp, M_PCB);
			return;
		}
	}
}

/* ------------------------------------------------------------------
 * nd6_dad_timer - called from nd6_timer() to drive DAD state machine.
 *
 * For each entry: send NS probe, check for completion.
 * ------------------------------------------------------------------ */
void
nd6_dad_timer(void)
{
	struct dadq *dp, **pp;
	struct timeval _tv;
	long now;

	GetSysTime(&_tv);
	now = _tv.tv_sec;

	pp = &dadq_head;
	while ((dp = *pp) != NULL) {
		if (dp->timer > now) {
			pp = &dp->next;
			continue;
		}

		if (dp->ns_icount > 0) {
			/* received NS for our tentative addr = duplicate */
			dp->ia->ia6_ifaflags |= IN6_IFF_DUPLICATED;
			dp->ia->ia6_ifaflags &= ~IN6_IFF_TENTATIVE;
			nd6stat.nd6s_dad_dup++;
			__log(LOG_ERR,
			    "nd6_dad_timer: DAD duplicate detected on %s%d\n",
			    dp->ifp->if_name, dp->ifp->if_unit);
			/* remove from queue */
			*pp = dp->next;
			FREE(dp, M_PCB);
			continue;
		}

		if (dp->count >= ND6_MAX_DAD_NS) {
			/* DAD complete — no duplicates detected */
			dp->ia->ia6_ifaflags &= ~IN6_IFF_TENTATIVE;
			/* send initial Router Solicitation now that addr is valid */
			nd6_rs_output(dp->ifp);
			/* remove from queue */
			*pp = dp->next;
			FREE(dp, M_PCB);
			continue;
		}

		/* send DAD NS probe (src = ::, dad = 1) */
		nd6_ns_output(dp->ifp, NULL,
		    &dp->ia->ia_addr.sin6_addr, NULL, 1);
		dp->count++;
		dp->ns_ocount++;
		dp->timer = now + ND6_RETRANS_TIMER / 1000;
		if (dp->timer <= now)
			dp->timer = now + 1;

		pp = &dp->next;
	}
}

/* ------------------------------------------------------------------
 * nd6_dad_ns_input - called when we receive NS for our tentative addr.
 *
 * This means someone else is also doing DAD for the same address.
 * ------------------------------------------------------------------ */
void
nd6_dad_ns_input(struct in6_ifaddr *ia)
{
	struct dadq *dp;

	for (dp = dadq_head; dp; dp = dp->next) {
		if (dp->ia == ia) {
			dp->ns_icount++;
			return;
		}
	}
}

#endif /* INET6 */
