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

/*
 * in6.c - IPv6 interface address management.
 *
 * Manages IPv6 addresses assigned to network interfaces, including
 * address addition/removal, ioctl handling, and link-local address
 * auto-generation.
 */

#include <conf.h>

#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/synch.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

struct in6_ifaddr *in6_ifaddr = NULL;
struct ifqueue    ip6intrq    = {0};

int ip6_defhlim    = 64;
int ip6_forwarding = 0;
int ip6_v6only     = 1;   /* sockets default to IPv6-only */

extern struct ifnet loif;

/* ------------------------------------------------------------------
 * Utility: compare two IPv6 addresses.
 * ------------------------------------------------------------------ */
static int
in6_addrscope(struct in6_addr *addr)
{
	if (IN6_IS_ADDR_MULTICAST(addr)) {
		return addr->s6_addr[1] & 0x0f;
	}
	if (IN6_IS_ADDR_LINKLOCAL(addr))
		return 2;
	if (IN6_IS_ADDR_SITELOCAL(addr))
		return 5;
	return 14;  /* global */
}

/* ------------------------------------------------------------------
 * in6_control - ioctl handler for IPv6 interface addresses.
 * ------------------------------------------------------------------ */
int
in6_control(struct socket *so, int cmd, caddr_t data, struct ifnet *ifp)
{
	struct in6_ifreq  *ifr  = (struct in6_ifreq *)data;
	struct in6_aliasreq *ifra = (struct in6_aliasreq *)data;
	struct in6_ifaddr *ia   = NULL, *oia;
	struct ifaddr     *ifa;
	struct mbuf       *m;
	int error = 0;
	spl_t s;

	/* locate existing IPv6 address for this interface */
	if (ifp) {
		for (ia = in6_ifaddr; ia; ia = ia->ia_next)
			if (ia->ia6_ifp == ifp)
				break;
	}

	switch (cmd) {
	case SIOCAIFADDR_IN6:
	case SIOCDIFADDR_IN6:
		/* find the exact address being requested */
		if (ifra->ifra_addr.sin6_family == AF_INET6) {
			for (oia = ia; ia; ia = ia->ia_next) {
				if (ia->ia6_ifp == ifp &&
				    IN6_ARE_ADDR_EQUAL(
				      &ia->ia_addr.sin6_addr,
				      &ifra->ifra_addr.sin6_addr))
					break;
			}
		}
		if (cmd == SIOCDIFADDR_IN6 && ia == NULL)
			return EADDRNOTAVAIL;
		/* FALLTHROUGH */
	case SIOCSIFADDR:
		if (ifp == NULL)
			panic("in6_control");
		if (ia == NULL) {
			/* allocate new address record */
			m = m_getclr(M_WAIT, MT_IFADDR);
			if (m == NULL)
				return ENOBUFS;
			if ((oia = in6_ifaddr) != NULL) {
				for (; oia->ia_next; oia = oia->ia_next)
					;
				oia->ia_next = mtod(m, struct in6_ifaddr *);
			} else {
				in6_ifaddr = mtod(m, struct in6_ifaddr *);
			}
			ia = mtod(m, struct in6_ifaddr *);
			/* link into ifp address list */
			if ((ifa = ifp->if_addrlist) != NULL) {
				for (; ifa->ifa_next; ifa = ifa->ifa_next)
					;
				ifa->ifa_next = (struct ifaddr *)ia;
			} else {
				ifp->if_addrlist = (struct ifaddr *)ia;
			}
			ia->ia_ifa.ifa_addr =
				(struct sockaddr *)&ia->ia_addr;
			ia->ia_ifa.ifa_dstaddr =
				(struct sockaddr *)&ia->ia_dstaddr;
			ia->ia_ifa.ifa_netmask =
				(struct sockaddr *)&ia->ia_prefixmask;
			ia->ia6_ifp = ifp;
		}
		break;

	case SIOCGIFADDR_IN6:
		if (ia == NULL)
			return EADDRNOTAVAIL;
		break;

	default:
		if (ifp == NULL || ifp->if_ioctl == NULL)
			return EOPNOTSUPP;
		return (*ifp->if_ioctl)(ifp, cmd, data);
	}

	switch (cmd) {
	case SIOCGIFADDR_IN6:
		ifr->ifr_addr = ia->ia_addr;
		break;

	case SIOCSIFADDR:
		/* set IPv6 address (from in6_aliasreq) */
		return in6_ifinit(ifp, ia, &ifra->ifra_addr, 1);

	case SIOCAIFADDR_IN6:
		/* add/change alias */
		if (ifra->ifra_prefixmask.sin6_len)
			ia->ia_prefixmask = ifra->ifra_prefixmask;
		if (ifra->ifra_addr.sin6_family == AF_INET6) {
			error = in6_ifinit(ifp, ia, &ifra->ifra_addr, 0);
			if (error)
				return error;
		}
		ia->ia6_flags    = ifra->ifra_flags;
		ia->ia6_lifetime_vltime = ifra->ifra_lifetime.ia6t_vltime;
		ia->ia6_lifetime_pltime = ifra->ifra_lifetime.ia6t_pltime;
		return 0;

	case SIOCDIFADDR_IN6:
		in6_purgeaddr((struct ifaddr *)ia);
		break;

	default:
		if (ifp == NULL || ifp->if_ioctl == NULL)
			return EOPNOTSUPP;
		return (*ifp->if_ioctl)(ifp, cmd, data);
	}
	return 0;
}

/* ------------------------------------------------------------------
 * in6_ifscrub - remove route for an IPv6 interface address.
 * ------------------------------------------------------------------ */
void
in6_ifscrub(struct ifnet *ifp, struct in6_ifaddr *ia)
{
	if ((ia->ia6_flags & IFA_ROUTE) == 0)
		return;
	if (ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT))
		rtinit(&ia->ia_ifa, RTM_DELETE, RTF_HOST);
	else
		rtinit(&ia->ia_ifa, RTM_DELETE, 0);
	ia->ia6_flags &= ~IFA_ROUTE;
}

/* ------------------------------------------------------------------
 * in6_ifinit - initialise an IPv6 interface address and add its route.
 * ------------------------------------------------------------------ */
int
in6_ifinit(struct ifnet *ifp, struct in6_ifaddr *ia,
	   struct sockaddr_in6 *sin6, int scrub)
{
	struct sockaddr_in6 oldaddr;
	int flags = RTF_UP;
	int error;
	spl_t s;

	s = splimp();
	oldaddr = ia->ia_addr;
	ia->ia_addr = *sin6;

	if (ifp->if_ioctl &&
	    (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)ia))) {
		splx(s);
		ia->ia_addr = oldaddr;
		return error;
	}
	splx(s);

	if (scrub) {
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&oldaddr;
		in6_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = (struct sockaddr *)&ia->ia_addr;
	}

	/* build prefix mask from prefix length */
	if (ia->ia6_plen > 0) {
		int i, plen = ia->ia6_plen;
		u_int8_t *mask = ia->ia_prefixmask.sin6_addr.s6_addr;
		bzero(mask, 16);
		for (i = 0; i < 16 && plen >= 8; i++, plen -= 8)
			mask[i] = 0xff;
		if (plen > 0)
			mask[i] = (0xff << (8 - plen)) & 0xff;
		ia->ia_prefixmask.sin6_family = AF_INET6;
		ia->ia_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
	}

	if (ifp->if_flags & IFF_LOOPBACK) {
		ia->ia_ifa.ifa_dstaddr = ia->ia_ifa.ifa_addr;
		flags |= RTF_HOST;
	} else if (ifp->if_flags & IFF_POINTOPOINT) {
		if (ia->ia_dstaddr.sin6_family != AF_INET6)
			return 0;
		flags |= RTF_HOST;
	}

	if ((error = rtinit(&ia->ia_ifa, RTM_ADD, flags)) == 0)
		ia->ia6_flags |= IFA_ROUTE;

	return error;
}

/* ------------------------------------------------------------------
 * in6_purgeaddr - remove an IPv6 address from an interface.
 * ------------------------------------------------------------------ */
void
in6_purgeaddr(struct ifaddr *ifa)
{
	struct in6_ifaddr *ia  = (struct in6_ifaddr *)ifa;
	struct in6_ifaddr *oia;
	struct ifnet *ifp = ia->ia6_ifp;
	struct ifaddr *ifa2;

	in6_ifscrub(ifp, ia);

	/* unlink from ifp address list */
	if ((ifa2 = ifp->if_addrlist) == (struct ifaddr *)ia) {
		ifp->if_addrlist = ifa2->ifa_next;
	} else {
		while (ifa2->ifa_next && ifa2->ifa_next != (struct ifaddr *)ia)
			ifa2 = ifa2->ifa_next;
		if (ifa2->ifa_next)
			ifa2->ifa_next = ((struct ifaddr *)ia)->ifa_next;
	}

	/* unlink from global list */
	oia = ia;
	if (oia == in6_ifaddr) {
		in6_ifaddr = ia->ia_next;
	} else {
		for (ia = in6_ifaddr; ia && ia->ia_next != oia; ia = ia->ia_next)
			;
		if (ia)
			ia->ia_next = oia->ia_next;
	}

	(void)m_free(dtom(oia));
}

/* ------------------------------------------------------------------
 * in6_localaddr - return 1 if addr is a local IPv6 address.
 * ------------------------------------------------------------------ */
int
in6_localaddr(struct in6_addr *in6)
{
	struct in6_ifaddr *ia;

	if (IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_ADDR_LINKLOCAL(in6))
		return 1;

	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (IN6_ARE_ADDR_EQUAL(in6, &ia->ia_addr.sin6_addr))
			return 1;
	}
	return 0;
}

/* ------------------------------------------------------------------
 * in6_ifawithifp - find best IPv6 source address for given ifp/dest.
 * ------------------------------------------------------------------ */
struct in6_ifaddr *
in6_ifawithifp(struct ifnet *ifp, struct in6_addr *dst)
{
	struct in6_ifaddr *ia, *besta = NULL;
	int bestscope = -1;
	int dstscope  = in6_addrscope(dst);

	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (ia->ia6_ifp != ifp)
			continue;
		if (ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED))
			continue;
		int sc = in6_addrscope(&ia->ia_addr.sin6_addr);
		if (sc == dstscope) {
			besta = ia;
			break;
		}
		if (sc > bestscope) {
			bestscope = sc;
			besta = ia;
		}
	}
	return besta;
}

/* ------------------------------------------------------------------
 * in6_ifaof_ifpforlinklocal - find the link-local address of an ifp.
 * ------------------------------------------------------------------ */
struct in6_ifaddr *
in6_ifaof_ifpforlinklocal(struct ifnet *ifp)
{
	struct in6_ifaddr *ia;

	for (ia = in6_ifaddr; ia; ia = ia->ia_next) {
		if (ia->ia6_ifp == ifp &&
		    IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr) &&
		    !(ia->ia6_ifaflags & (IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED)))
			return ia;
	}
	return NULL;
}

/* ------------------------------------------------------------------
 * in6_if_up - called when interface is brought up; configure link-local.
 * ------------------------------------------------------------------ */
void
in6_if_up(struct ifnet *ifp)
{
	struct in6_ifaddr *ia;
	struct sockaddr_in6 sin6;
	struct mbuf *m;

	/* skip loopback - it already has ::1 */
	if (ifp->if_flags & IFF_LOOPBACK)
		return;

	/* check if link-local already configured */
	if (in6_ifaof_ifpforlinklocal(ifp))
		return;

	/* only for interfaces with a link-layer address */
	if (ifp->if_addrlen == 0)
		return;

	/* allocate address record */
	m = m_getclr(M_WAIT, MT_IFADDR);
	if (m == NULL)
		return;

	ia = mtod(m, struct in6_ifaddr *);

	/* build EUI-64 link-local from MAC address */
	bzero(&sin6, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len    = sizeof(sin6);
	sin6.sin6_addr.s6_addr[0] = 0xfe;
	sin6.sin6_addr.s6_addr[1] = 0x80;
	/* EUI-64: copy MAC, flip U/L bit */
	if (ifp->if_addrlen == 6) {
		u_int8_t *mac = (u_int8_t *)LLADDR(
			(struct sockaddr_dl *)ifp->if_addrlist->ifa_addr);
		sin6.sin6_addr.s6_addr[8]  = mac[0] ^ 0x02;
		sin6.sin6_addr.s6_addr[9]  = mac[1];
		sin6.sin6_addr.s6_addr[10] = mac[2];
		sin6.sin6_addr.s6_addr[11] = 0xff;
		sin6.sin6_addr.s6_addr[12] = 0xfe;
		sin6.sin6_addr.s6_addr[13] = mac[3];
		sin6.sin6_addr.s6_addr[14] = mac[4];
		sin6.sin6_addr.s6_addr[15] = mac[5];
	} else {
		/* can't form EUI-64 */
		m_free(m);
		return;
	}

	/* link into global and ifp lists */
	if (in6_ifaddr) {
		struct in6_ifaddr *tail;
		for (tail = in6_ifaddr; tail->ia_next; tail = tail->ia_next)
			;
		tail->ia_next = ia;
	} else {
		in6_ifaddr = ia;
	}
	if (ifp->if_addrlist) {
		struct ifaddr *tail;
		for (tail = ifp->if_addrlist; tail->ifa_next; tail = tail->ifa_next)
			;
		tail->ifa_next = (struct ifaddr *)ia;
	} else {
		ifp->if_addrlist = (struct ifaddr *)ia;
	}

	ia->ia_ifa.ifa_addr    = (struct sockaddr *)&ia->ia_addr;
	ia->ia_ifa.ifa_netmask = (struct sockaddr *)&ia->ia_prefixmask;
	ia->ia6_ifp = ifp;
	ia->ia6_plen = 64;
	ia->ia6_ifaflags = IN6_IFF_AUTOCONF;

	(void)in6_ifinit(ifp, ia, &sin6, 0);
}

#endif /* INET6 */
