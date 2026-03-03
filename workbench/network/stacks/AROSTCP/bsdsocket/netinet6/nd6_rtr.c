/*
 * Copyright (C) 2020-2026 The AROS Development Team.  All rights reserved.
 *
 * Based on FreeBSD/KAME nd6_rtr.c:
 * Copyright (C) 1995-1998 WIDE Project. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * nd6_rtr.c - IPv6 Router Discovery.
 *
 * Implements:
 *   - Router Solicitation output (RFC 4861 §6.3.7)
 *   - Router Advertisement input (RFC 4861 §6.3.4)
 *   - Default Router List management
 *   - Prefix List management
 *   - Redirect input (RFC 4861 §8.3)
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
int ip6_output(void *, ...);

/* Default router list (singly-linked) */
static struct nd_defrouter *nd6_defrtrlist = NULL;

/* Prefix list (singly-linked) */
static struct nd_prefix *nd6_prefixlist = NULL;

/* ------------------------------------------------------------------
 * ICMPv6 checksum helper (same algorithm as nd6_nbr.c).
 * ------------------------------------------------------------------ */
static u_int16_t
nd6_rtr_cksum(struct ip6_hdr *ip6, void *icmp6, int icmp6len)
{
	u_int32_t sum = 0;
	u_int16_t *p;
	int i;

	{
		struct in6_addr addrs[2];
		memcpy(&addrs[0], &ip6->ip6_src, sizeof(struct in6_addr));
		memcpy(&addrs[1], &ip6->ip6_dst, sizeof(struct in6_addr));
		p = (u_int16_t *)addrs;
		for (i = 0; i < 16; i++)
			sum += ntohs(p[i]);
	}
	sum += (u_int32_t)icmp6len;
	sum += IPPROTO_ICMPV6;
	p = (u_int16_t *)icmp6;
	for (i = 0; i < icmp6len / 2; i++)
		sum += ntohs(p[i]);
	if (icmp6len & 1)
		sum += ((u_int8_t *)icmp6)[icmp6len - 1] << 8;
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return htons((u_int16_t)~sum);
}

/* ==================================================================
 * Default Router List Management
 * ================================================================== */

/* ------------------------------------------------------------------
 * defrouter_lookup - find a default router by address and interface.
 * ------------------------------------------------------------------ */
struct nd_defrouter *
defrouter_lookup(struct in6_addr *addr, struct ifnet *ifp)
{
	struct nd_defrouter *dr;

	for (dr = nd6_defrtrlist; dr; dr = dr->next) {
		if (dr->ifp == ifp &&
		    IN6_ARE_ADDR_EQUAL(&dr->rtaddr, addr))
			return dr;
	}
	return NULL;
}

/* ------------------------------------------------------------------
 * defrouter_insert - add or update a default router entry.
 * ------------------------------------------------------------------ */
static struct nd_defrouter *
defrouter_insert(struct in6_addr *addr, struct ifnet *ifp,
    u_int16_t lifetime, u_int8_t raflags)
{
	struct nd_defrouter *dr;
	struct timeval _tv;

	GetSysTime(&_tv);

	dr = defrouter_lookup(addr, ifp);
	if (dr) {
		/* update existing entry */
		dr->rtlifetime = lifetime;
		dr->flags = raflags;
		if (lifetime == 0) {
			dr->expire = 0;	/* will be pruned */
		} else {
			dr->expire = _tv.tv_sec + lifetime;
		}
		return dr;
	}

	/* count existing entries */
	{
		int count = 0;
		for (dr = nd6_defrtrlist; dr; dr = dr->next)
			count++;
		if (count >= ND6_MAX_DEFROUTERS)
			return NULL;	/* too many */
	}

	MALLOC(dr, struct nd_defrouter *, sizeof(*dr), M_PCB, M_NOWAIT);
	if (dr == NULL)
		return NULL;
	bzero(dr, sizeof(*dr));
	dr->rtaddr    = *addr;
	dr->ifp       = ifp;
	dr->rtlifetime = lifetime;
	dr->flags     = raflags;
	dr->expire    = (lifetime == 0) ? 0 : _tv.tv_sec + lifetime;
	dr->installed = 0;

	/* insert at head */
	dr->next = nd6_defrtrlist;
	nd6_defrtrlist = dr;

	return dr;
}

/* ------------------------------------------------------------------
 * defrouter_del - remove a default router entry.
 * ------------------------------------------------------------------ */
static void
defrouter_del(struct nd_defrouter *dr)
{
	struct nd_defrouter **pp;

	/* remove route if installed */
	if (dr->installed) {
		struct sockaddr_in6 defrt, gate;

		bzero(&defrt, sizeof(defrt));
		defrt.sin6_family = AF_INET6;
		defrt.sin6_len    = sizeof(defrt);
		/* defrt addr = :: (default route) */

		bzero(&gate, sizeof(gate));
		gate.sin6_family = AF_INET6;
		gate.sin6_len    = sizeof(gate);
		gate.sin6_addr   = dr->rtaddr;

		rtrequest(RTM_DELETE, (struct sockaddr *)&defrt,
		    (struct sockaddr *)&gate, NULL, RTF_GATEWAY, NULL);
		dr->installed = 0;
	}

	/* unlink from list */
	for (pp = &nd6_defrtrlist; *pp; pp = &(*pp)->next) {
		if (*pp == dr) {
			*pp = dr->next;
			break;
		}
	}
	FREE(dr, M_PCB);
}

/* ------------------------------------------------------------------
 * defrouter_select - install the best default router as default route.
 *
 * Simple policy: pick the first non-expired entry.
 * ------------------------------------------------------------------ */
void
defrouter_select(void)
{
	struct nd_defrouter *dr, *best = NULL;
	struct timeval _tv;
	struct sockaddr_in6 defrt, gate;

	GetSysTime(&_tv);

	for (dr = nd6_defrtrlist; dr; dr = dr->next) {
		if (dr->expire != 0 && dr->expire < _tv.tv_sec)
			continue;	/* expired */
		if (dr->rtlifetime == 0)
			continue;	/* lifetime 0 = don't use */
		best = dr;
		break;
	}

	/* remove any currently installed default routes */
	for (dr = nd6_defrtrlist; dr; dr = dr->next) {
		if (dr->installed && dr != best) {
			bzero(&defrt, sizeof(defrt));
			defrt.sin6_family = AF_INET6;
			defrt.sin6_len    = sizeof(defrt);

			bzero(&gate, sizeof(gate));
			gate.sin6_family = AF_INET6;
			gate.sin6_len    = sizeof(gate);
			gate.sin6_addr   = dr->rtaddr;

			rtrequest(RTM_DELETE, (struct sockaddr *)&defrt,
			    (struct sockaddr *)&gate, NULL,
			    RTF_GATEWAY, NULL);
			dr->installed = 0;
		}
	}

	/* install the selected router */
	if (best && !best->installed) {
		bzero(&defrt, sizeof(defrt));
		defrt.sin6_family = AF_INET6;
		defrt.sin6_len    = sizeof(defrt);

		bzero(&gate, sizeof(gate));
		gate.sin6_family = AF_INET6;
		gate.sin6_len    = sizeof(gate);
		gate.sin6_addr   = best->rtaddr;

		if (rtrequest(RTM_ADD, (struct sockaddr *)&defrt,
		    (struct sockaddr *)&gate, NULL,
		    RTF_GATEWAY | RTF_DYNAMIC, NULL) == 0) {
			best->installed = 1;
		}
	}
}

/* ------------------------------------------------------------------
 * nd6_defrouter_timer - expire default routers (called from nd6_timer).
 * ------------------------------------------------------------------ */
void
nd6_defrouter_timer(void)
{
	struct nd_defrouter *dr, *next;
	struct timeval _tv;
	int changed = 0;

	GetSysTime(&_tv);

	for (dr = nd6_defrtrlist; dr; dr = next) {
		next = dr->next;
		if (dr->expire != 0 && dr->expire < _tv.tv_sec) {
			defrouter_del(dr);
			changed = 1;
		}
	}
	if (changed)
		defrouter_select();
}

/* ==================================================================
 * Prefix List Management
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_prefix_lookup - find a prefix entry.
 * ------------------------------------------------------------------ */
struct nd_prefix *
nd6_prefix_lookup(struct sockaddr_in6 *prefix, int plen)
{
	struct nd_prefix *pr;

	for (pr = nd6_prefixlist; pr; pr = pr->next) {
		if (pr->ndpr_plen == plen &&
		    IN6_ARE_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		                       &prefix->sin6_addr))
			return pr;
	}
	return NULL;
}

/* ------------------------------------------------------------------
 * nd6_prefix_add - add or update a prefix list entry.
 * ------------------------------------------------------------------ */
static struct nd_prefix *
nd6_prefix_add(struct ifnet *ifp, struct nd_opt_prefix_info *pi)
{
	struct nd_prefix *pr;
	struct sockaddr_in6 prefix;
	struct timeval _tv;
	int plen = pi->nd_opt_pi_prefix_len;

	bzero(&prefix, sizeof(prefix));
	prefix.sin6_family = AF_INET6;
	prefix.sin6_len    = sizeof(prefix);
	prefix.sin6_addr   = pi->nd_opt_pi_prefix;

	GetSysTime(&_tv);

	pr = nd6_prefix_lookup(&prefix, plen);
	if (pr) {
		/* update lifetimes */
		pr->ndpr_vltime = ntohl(pi->nd_opt_pi_valid_time);
		pr->ndpr_pltime = ntohl(pi->nd_opt_pi_preferred_time);
		if (pr->ndpr_vltime == 0xffffffff)
			pr->ndpr_expire = 0;	/* infinite */
		else
			pr->ndpr_expire = _tv.tv_sec + pr->ndpr_vltime;
		if (pr->ndpr_pltime == 0xffffffff)
			pr->ndpr_preferred = 0;
		else
			pr->ndpr_preferred = _tv.tv_sec + pr->ndpr_pltime;
		pr->ndpr_flags = pi->nd_opt_pi_flags_reserved;
		return pr;
	}

	/* count existing */
	{
		int count = 0;
		for (pr = nd6_prefixlist; pr; pr = pr->next)
			count++;
		if (count >= ND6_MAX_PREFIXES)
			return NULL;
	}

	MALLOC(pr, struct nd_prefix *, sizeof(*pr), M_PCB, M_NOWAIT);
	if (pr == NULL)
		return NULL;
	bzero(pr, sizeof(*pr));

	pr->ndpr_ifp    = ifp;
	pr->ndpr_prefix = prefix;
	pr->ndpr_plen   = plen;
	pr->ndpr_vltime = ntohl(pi->nd_opt_pi_valid_time);
	pr->ndpr_pltime = ntohl(pi->nd_opt_pi_preferred_time);
	pr->ndpr_flags  = pi->nd_opt_pi_flags_reserved;
	pr->ndpr_refcount = 0;
	pr->ndpr_onlink = 0;
	pr->ndpr_autoconf = 0;

	if (pr->ndpr_vltime == 0xffffffff)
		pr->ndpr_expire = 0;
	else
		pr->ndpr_expire = _tv.tv_sec + pr->ndpr_vltime;
	if (pr->ndpr_pltime == 0xffffffff)
		pr->ndpr_preferred = 0;
	else
		pr->ndpr_preferred = _tv.tv_sec + pr->ndpr_pltime;

	/* insert at head */
	pr->next = nd6_prefixlist;
	nd6_prefixlist = pr;

	return pr;
}

/* ------------------------------------------------------------------
 * nd6_prefix_onlink - install an on-link route for a prefix.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_onlink(struct nd_prefix *pr)
{
	struct sockaddr_in6 prefix, mask;
	int i, plen;

	if (pr->ndpr_onlink)
		return;

	bzero(&prefix, sizeof(prefix));
	prefix.sin6_family = AF_INET6;
	prefix.sin6_len    = sizeof(prefix);
	prefix.sin6_addr   = pr->ndpr_prefix.sin6_addr;

	/* build netmask */
	bzero(&mask, sizeof(mask));
	mask.sin6_family = AF_INET6;
	mask.sin6_len    = sizeof(mask);
	plen = pr->ndpr_plen;
	for (i = 0; i < 16 && plen >= 8; i++, plen -= 8)
		mask.sin6_addr.s6_addr[i] = 0xff;
	if (plen > 0)
		mask.sin6_addr.s6_addr[i] = (0xff << (8 - plen)) & 0xff;

	/* mask the prefix */
	for (i = 0; i < 16; i++)
		prefix.sin6_addr.s6_addr[i] &= mask.sin6_addr.s6_addr[i];

	if (rtrequest(RTM_ADD, (struct sockaddr *)&prefix,
	    NULL, (struct sockaddr *)&mask,
	    RTF_UP | RTF_CLONING, NULL) == 0) {
		pr->ndpr_onlink = 1;
	}
}

/* ------------------------------------------------------------------
 * nd6_prefix_offlink - remove on-link route for a prefix.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_offlink(struct nd_prefix *pr)
{
	struct sockaddr_in6 prefix, mask;
	int i, plen;

	if (!pr->ndpr_onlink)
		return;

	bzero(&prefix, sizeof(prefix));
	prefix.sin6_family = AF_INET6;
	prefix.sin6_len    = sizeof(prefix);
	prefix.sin6_addr   = pr->ndpr_prefix.sin6_addr;

	bzero(&mask, sizeof(mask));
	mask.sin6_family = AF_INET6;
	mask.sin6_len    = sizeof(mask);
	plen = pr->ndpr_plen;
	for (i = 0; i < 16 && plen >= 8; i++, plen -= 8)
		mask.sin6_addr.s6_addr[i] = 0xff;
	if (plen > 0)
		mask.sin6_addr.s6_addr[i] = (0xff << (8 - plen)) & 0xff;

	for (i = 0; i < 16; i++)
		prefix.sin6_addr.s6_addr[i] &= mask.sin6_addr.s6_addr[i];

	rtrequest(RTM_DELETE, (struct sockaddr *)&prefix,
	    NULL, (struct sockaddr *)&mask, RTF_UP | RTF_CLONING, NULL);
	pr->ndpr_onlink = 0;
}

/* ------------------------------------------------------------------
 * nd6_prefix_timer - expire prefixes (called from nd6_timer).
 * ------------------------------------------------------------------ */
void
nd6_prefix_timer(void)
{
	struct nd_prefix *pr, **pp;
	struct timeval _tv;

	GetSysTime(&_tv);

	pp = &nd6_prefixlist;
	while ((pr = *pp) != NULL) {
		if (pr->ndpr_expire != 0 && pr->ndpr_expire < _tv.tv_sec) {
			/* prefix expired */
			nd6_prefix_offlink(pr);
			*pp = pr->next;
			FREE(pr, M_PCB);
			continue;
		}
		pp = &pr->next;
	}
}

/* ==================================================================
 * SLAAC - Stateless Address Autoconfiguration (RFC 4862)
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_prefix_slaac - create a SLAAC address from prefix + EUI-64.
 * ------------------------------------------------------------------ */
static void
nd6_prefix_slaac(struct ifnet *ifp, struct nd_prefix *pr)
{
	struct in6_ifaddr *ia;
	struct in6_aliasreq ifra;
	struct sockaddr_in6 addr;

	if (pr->ndpr_plen != 64)
		return;			/* SLAAC requires /64 */
	if (pr->ndpr_autoconf)
		return;			/* already configured */

	/* get link-local to derive EUI-64 interface identifier */
	ia = in6_ifaof_ifpforlinklocal(ifp);
	if (ia == NULL)
		return;

	/* build global address: prefix + IID from link-local */
	bzero(&addr, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_len    = sizeof(addr);
	bcopy(&pr->ndpr_prefix.sin6_addr, &addr.sin6_addr, 8);
	bcopy(&ia->ia_addr.sin6_addr.s6_addr[8],
	    &addr.sin6_addr.s6_addr[8], 8);

	/* check we don't already have this address */
	{
		struct in6_ifaddr *existing;
		for (existing = in6_ifaddr; existing; existing = existing->ia_next) {
			if (existing->ia6_ifp == ifp &&
			    IN6_ARE_ADDR_EQUAL(&existing->ia_addr.sin6_addr,
			                       &addr.sin6_addr))
				return;	/* already have it */
		}
	}

	/* add the address */
	bzero(&ifra, sizeof(ifra));
	bcopy(ifp->if_name, ifra.ifra_name, sizeof(ifra.ifra_name));
	ifra.ifra_addr = addr;
	ifra.ifra_flags = IN6_IFF_AUTOCONF;
	ifra.ifra_lifetime.ia6t_vltime = pr->ndpr_vltime;
	ifra.ifra_lifetime.ia6t_pltime = pr->ndpr_pltime;

	/* set /64 prefix mask */
	ifra.ifra_prefixmask.sin6_family = AF_INET6;
	ifra.ifra_prefixmask.sin6_len = sizeof(ifra.ifra_prefixmask);
	{
		int j;
		for (j = 0; j < 8; j++)
			ifra.ifra_prefixmask.sin6_addr.s6_addr[j] = 0xff;
	}

	in6_control(NULL, SIOCAIFADDR_IN6, (caddr_t)&ifra, ifp);

	/* join solicited-node multicast for the new address */
	{
		struct in6_addr maddr;
		bzero(&maddr, sizeof(maddr));
		maddr.s6_addr[0]  = 0xff;
		maddr.s6_addr[1]  = 0x02;
		maddr.s6_addr[11] = 0x01;
		maddr.s6_addr[12] = 0xff;
		maddr.s6_addr[13] = addr.sin6_addr.s6_addr[13];
		maddr.s6_addr[14] = addr.sin6_addr.s6_addr[14];
		maddr.s6_addr[15] = addr.sin6_addr.s6_addr[15];
		(void)in6_addmulti(&maddr, ifp);
	}

	pr->ndpr_autoconf = 1;
	pr->ndpr_refcount++;
}

/* ==================================================================
 * Router Solicitation Output
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_rs_output - send a Router Solicitation on an interface.
 *
 * Called after DAD completes to discover routers on the link.
 * ------------------------------------------------------------------ */
void
nd6_rs_output(struct ifnet *ifp)
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct nd_router_solicit *rs;
	struct nd_opt_hdr *nd_opt;
	caddr_t mac;
	int hlen = sizeof(struct ip6_hdr);
	int icmp6len = sizeof(struct nd_router_solicit);
	int optlen = 0;
	int pktlen;
	struct in6_ifaddr *ia;

	/* include SLLA if we have a valid source address */
	ia = in6_ifaof_ifpforlinklocal(ifp);
	mac = nd6_ifptomac(ifp);
	if (ia && mac && ifp->if_addrlen == 6)
		optlen = 8;

	pktlen = hlen + icmp6len + optlen;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;

	m->m_pkthdr.len = m->m_len = pktlen;
	m->m_pkthdr.rcvif = NULL;

	ip6 = mtod(m, struct ip6_hdr *);
	rs  = (struct nd_router_solicit *)(ip6 + 1);

	/* IPv6 header */
	bzero(ip6, sizeof(*ip6));
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_nxt  = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 255;
	ip6->ip6_plen = htons(icmp6len + optlen);

	/* destination: all-routers multicast ff02::2 */
	bzero(&ip6->ip6_dst, sizeof(ip6->ip6_dst));
	ip6->ip6_dst.s6_addr[0]  = 0xff;
	ip6->ip6_dst.s6_addr[1]  = 0x02;
	ip6->ip6_dst.s6_addr[15] = 0x02;

	/* source: link-local or :: if no address yet */
	if (ia)
		ip6->ip6_src = ia->ia_addr.sin6_addr;
	else
		bzero(&ip6->ip6_src, sizeof(ip6->ip6_src));

	/* Router Solicitation */
	bzero(rs, sizeof(*rs));
	rs->nd_rs_type = ND_ROUTER_SOLICIT;

	/* Source Link-Layer Address option */
	if (optlen) {
		nd_opt = (struct nd_opt_hdr *)(rs + 1);
		nd_opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd_opt->nd_opt_len  = 1;
		bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
	}

	/* checksum */
	rs->nd_rs_cksum = 0;
	rs->nd_rs_cksum = nd6_rtr_cksum(ip6, rs, icmp6len + optlen);

	nd6stat.nd6s_snd_rs++;
	ip6_output(m, NULL, NULL, 0, NULL, NULL, NULL);
}

/* ==================================================================
 * Router Solicitation / Advertisement Input
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_rs_input - process an inbound Router Solicitation.
 *
 * As a host, we ignore RS messages (only routers respond to RS).
 * ------------------------------------------------------------------ */
void
nd6_rs_input(struct mbuf *m, int off, int icmp6len)
{
	nd6stat.nd6s_rcv_rs++;
	m_freem(m);
}

/* ------------------------------------------------------------------
 * nd6_ra_input - process an inbound Router Advertisement.
 *
 * Implements RFC 4861 §6.3.4:
 *   - Update hop limit, reachable time, retrans timer
 *   - Manage default router list
 *   - Process prefix information options (on-link + SLAAC)
 *   - Process MTU option
 * ------------------------------------------------------------------ */
void
nd6_ra_input(struct mbuf *m, int off, int icmp6len)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_router_advert *nd_ra;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct nd_defrouter *dr;
	struct nd_ifinfo *ndi;
	struct in6_addr src6;
	u_int8_t *opts, *end;
	int optlen;

	nd6stat.nd6s_rcv_ra++;

	/* copy source address out of packed header */
	memcpy(&src6, &ip6->ip6_src, sizeof(src6));

	/* validate hop limit (RFC 4861 §6.1.2) */
	if (ip6->ip6_hlim != 255) {
		nd6stat.nd6s_rcv_badra++;
		goto bad;
	}

	/* source must be link-local */
	if (!IN6_IS_ADDR_LINKLOCAL(&src6)) {
		nd6stat.nd6s_rcv_badra++;
		goto bad;
	}

	if (m->m_len < off + (int)sizeof(*nd_ra)) {
		if ((m = m_pullup(m, off + sizeof(*nd_ra))) == NULL) {
			nd6stat.nd6s_rcv_badra++;
			return;
		}
		ip6 = mtod(m, struct ip6_hdr *);
	}

	nd_ra = (struct nd_router_advert *)(mtod(m, u_int8_t *) + off);

	/* get per-interface ND info */
	ndi = nd6_ifinfo(ifp);

	/* update hop limit (RFC 4861 §6.3.4 step 1) */
	if (nd_ra->nd_ra_curhoplimit) {
		if (ndi)
			ndi->chlim = nd_ra->nd_ra_curhoplimit;
		ip6_defhlim = nd_ra->nd_ra_curhoplimit;
	}

	/* update reachable time (RFC 4861 §6.3.4 step 2) */
	if (ntohl(nd_ra->nd_ra_reachable) != 0 && ndi) {
		ndi->basereachable = ntohl(nd_ra->nd_ra_reachable);
		ndi->reachable = ndi->basereachable;
	}

	/* update retrans timer (RFC 4861 §6.3.4 step 3) */
	if (ntohl(nd_ra->nd_ra_retransmit) != 0 && ndi) {
		ndi->retrans = ntohl(nd_ra->nd_ra_retransmit);
	}

	/* manage default router list */
	{
		u_int16_t rtlifetime = ntohs(nd_ra->nd_ra_router_lifetime);
		dr = defrouter_lookup(&src6, ifp);
		if (rtlifetime == 0 && dr) {
			/* router no longer default */
			defrouter_del(dr);
			defrouter_select();
		} else if (rtlifetime > 0) {
			dr = defrouter_insert(&src6, ifp,
			    rtlifetime, nd_ra->nd_ra_flags_reserved);
			defrouter_select();
		}
	}

	/* update neighbor cache for the router */
	nd6_cache_lladdr(ifp, &src6, NULL, 0,
	    ND_ROUTER_ADVERT, 1);

	/* process options */
	opts = (u_int8_t *)(nd_ra + 1);
	end  = mtod(m, u_int8_t *) + m->m_len;

	while (opts + 2 <= end) {
		u_int8_t otype = opts[0];
		u_int8_t olen  = opts[1];

		if (olen == 0) break;
		optlen = olen * 8;
		if (opts + optlen > end) break;

		switch (otype) {
		case ND_OPT_SOURCE_LINKADDR:
			/* update neighbor cache with router's link-layer addr */
			if (optlen >= 8) {
				char *lladdr = (char *)(opts + 2);
				nd6_cache_lladdr(ifp, &src6,
				    lladdr, ifp->if_addrlen,
				    ND_ROUTER_ADVERT, 1);
			}
			break;

		case ND_OPT_PREFIX_INFORMATION:
			if (optlen >= (int)sizeof(struct nd_opt_prefix_info)) {
				struct nd_opt_prefix_info *pi =
				    (struct nd_opt_prefix_info *)opts;
				struct nd_prefix *pr;

				/* skip link-local prefixes */
				if (IN6_IS_ADDR_LINKLOCAL(&pi->nd_opt_pi_prefix))
					break;

				/* add/update prefix entry */
				pr = nd6_prefix_add(ifp, pi);
				if (pr == NULL)
					break;

				/* on-link determination */
				if (pi->nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_ONLINK)
					nd6_prefix_onlink(pr);

				/* SLAAC */
				if ((pi->nd_opt_pi_flags_reserved &
				    ND_OPT_PI_FLAG_AUTO) &&
				    pi->nd_opt_pi_prefix_len == 64)
					nd6_prefix_slaac(ifp, pr);
			}
			break;

		case ND_OPT_MTU:
			if (optlen >= (int)sizeof(struct nd_opt_mtu) && ndi) {
				struct nd_opt_mtu *mtu_opt =
				    (struct nd_opt_mtu *)opts;
				u_int32_t mtu = ntohl(mtu_opt->nd_opt_mtu_mtu);
				if (mtu >= IPV6_MMTU && mtu <= ifp->if_mtu) {
					ndi->linkmtu = mtu;
				}
			}
			break;

		default:
			/* ignore unknown options */
			break;
		}

		opts += optlen;
	}

	m_freem(m);
	return;

bad:
	m_freem(m);
}

/* ==================================================================
 * Redirect Message Input (RFC 4861 §8.3)
 * ================================================================== */

/* ------------------------------------------------------------------
 * nd6_redirect_input - process an inbound Redirect message.
 *
 * Updates the routing table to use the better next-hop.
 * ------------------------------------------------------------------ */
void
nd6_redirect_input(struct mbuf *m, int off, int icmp6len)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_redirect *rd;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct in6_addr src6, tgt, reddst;
	struct rtentry *rt;

	nd6stat.nd6s_rcv_redirect++;

	/* copy source address out of packed header */
	memcpy(&src6, &ip6->ip6_src, sizeof(src6));

	/* validate hop limit */
	if (ip6->ip6_hlim != 255) {
		nd6stat.nd6s_rcv_badredirect++;
		goto bad;
	}

	/* source must be link-local */
	if (!IN6_IS_ADDR_LINKLOCAL(&src6)) {
		nd6stat.nd6s_rcv_badredirect++;
		goto bad;
	}

	if (m->m_len < off + (int)sizeof(*rd)) {
		if ((m = m_pullup(m, off + sizeof(*rd))) == NULL) {
			nd6stat.nd6s_rcv_badredirect++;
			return;
		}
		ip6 = mtod(m, struct ip6_hdr *);
	}

	rd = (struct nd_redirect *)(mtod(m, u_int8_t *) + off);
	tgt    = rd->nd_rd_target;
	reddst = rd->nd_rd_dst;

	/* validate: source must be a known default router */
	if (defrouter_lookup(&src6, ifp) == NULL) {
		nd6stat.nd6s_rcv_badredirect++;
		goto bad;
	}

	/* destination must not be multicast */
	if (IN6_IS_ADDR_MULTICAST(&reddst)) {
		nd6stat.nd6s_rcv_badredirect++;
		goto bad;
	}

	/* if target != destination, target must be link-local (router) */
	if (!IN6_ARE_ADDR_EQUAL(&tgt, &reddst) &&
	    !IN6_IS_ADDR_LINKLOCAL(&tgt)) {
		nd6stat.nd6s_rcv_badredirect++;
		goto bad;
	}

	/* update routing table */
	{
		struct sockaddr_in6 dst_sa, gw_sa;

		bzero(&dst_sa, sizeof(dst_sa));
		dst_sa.sin6_family = AF_INET6;
		dst_sa.sin6_len    = sizeof(dst_sa);
		dst_sa.sin6_addr   = reddst;

		bzero(&gw_sa, sizeof(gw_sa));
		gw_sa.sin6_family = AF_INET6;
		gw_sa.sin6_len    = sizeof(gw_sa);
		gw_sa.sin6_addr   = tgt;

		/* try to update existing route, or add new one */
		rt = nd6_lookup(&reddst, 0, ifp);
		if (rt) {
			/* change gateway */
			rtrequest(RTM_DELETE, (struct sockaddr *)&dst_sa,
			    NULL, NULL, RTF_HOST, NULL);
		}
		rtrequest(RTM_ADD, (struct sockaddr *)&dst_sa,
		    (struct sockaddr *)&gw_sa, NULL,
		    RTF_HOST | RTF_GATEWAY | RTF_DYNAMIC, NULL);
	}

	/* update neighbor cache for the new target */
	{
		union nd_opts ndopts;
		u_int8_t *optstart = (u_int8_t *)(rd + 1);
		int optbytes = icmp6len - (int)sizeof(*rd);

		if (optbytes > 0) {
			nd6_option_init(optstart, optbytes, &ndopts);
			if (nd6_options(&ndopts) >= 0 &&
			    ndopts.nd_opts_tgt_lladdr) {
				char *lladdr = (char *)
				    (ndopts.nd_opts_tgt_lladdr + 1);
				nd6_cache_lladdr(ifp, &tgt, lladdr,
				    ifp->if_addrlen, ND_REDIRECT, 0);
			}
		}
	}

	m_freem(m);
	return;

bad:
	m_freem(m);
}

#endif /* INET6 */
