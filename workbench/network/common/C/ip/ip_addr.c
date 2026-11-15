/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip addr - display and manage interface addresses (IPv4 + IPv6)
 */

#define INET6 1

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/miami.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

static void
addr_usage(void)
{
	fprintf(stderr,
	    "Usage: ip addr show [ dev DEVICE ]\n"
	    "       ip addr help\n");
}

/*
 * Print IPv4 addresses for an interface.
 */
/* An ioctl on name with addr in ifr_addr: that address's answer. */
static int
inet_ask(int s, const char *name, struct in_addr addr, int cmd, struct ifreq *ifr)
{
	struct sockaddr_in *sin = (struct sockaddr_in *)&ifr->ifr_addr;

	memset(ifr, 0, sizeof(*ifr));
	snprintf(ifr->ifr_name, sizeof(ifr->ifr_name), "%s", name);
	sin->sin_len = sizeof(*sin);
	sin->sin_family = AF_INET;
	sin->sin_addr = addr;
	return IoctlSocket(s, cmd, (caddr_t)ifr);
}

/* One IPv4 address of name, its prefix and broadcast asked for it. */
static void
show_inet_one(const char *name, int s, struct in_addr addr)
{
	struct ifreq ifr;
	char buf[INET_ADDRSTRLEN];
	int prefix = 32;

	inet_ntop(AF_INET, &addr, buf, sizeof(buf));
	if (inet_ask(s, name, addr, SIOCGIFNETMASK, &ifr) >= 0) {
		int p = mask2prefix(&ifr.ifr_addr, AF_INET);
		if (p >= 0)
			prefix = p;
	}
	printf("    inet %s/%d", buf, prefix);
	if (inet_ask(s, name, addr, SIOCGIFBRDADDR, &ifr) >= 0) {
		struct sockaddr_in *bsin =
		    (struct sockaddr_in *)&ifr.ifr_broadaddr;
		if (bsin->sin_family == AF_INET &&
		    bsin->sin_addr.s_addr != 0) {
			char bbuf[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &bsin->sin_addr,
			    bbuf, sizeof(bbuf));
			printf(" brd %s", bbuf);
		}
	}
	printf(" scope global %s\n", name);

	/* Show valid/preferred lifetime if detailed */
	if (opts.detail) {
		printf("       valid_lft forever preferred_lft forever\n");
	}
}

/*
 * The interface's IPv4 addresses: the first (SIOCGIFADDR), then its
 * aliases, the other AF_INET entries SIOCGIFCONF lists for it.
 */
static void
show_inet_addr(const char *name, int s)
{
	struct ifreq ifr;
	const struct sockaddr_in *sin;
	struct in_addr first;
	struct ifconf ifc;
	char *cp;
	const char *cplim;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

	if (IoctlSocket(s, SIOCGIFADDR, (caddr_t)&ifr) < 0)
		return;
	sin = (const struct sockaddr_in *)&ifr.ifr_addr;
	if (sin->sin_family != AF_INET)
		return;
	first = sin->sin_addr;
	show_inet_one(name, s, first);

	if (ip_getifconf(s, &ifc) < 0)
		return;
	for (cp = ifc.ifc_buf, cplim = cp + ifc.ifc_len; cp < cplim; ) {
		struct ifreq *e = (struct ifreq *)cp;
		const struct sockaddr_in *esin =
		    (const struct sockaddr_in *)&e->ifr_addr;
		int sz = sizeof(e->ifr_name) + e->ifr_addr.sa_len;

		cp += sz < (int)sizeof(*e) ? (int)sizeof(*e) : sz;
		if (e->ifr_addr.sa_family == AF_INET &&
		    strcmp(e->ifr_name, name) == 0 &&
		    esin->sin_addr.s_addr != first.s_addr)
			show_inet_one(name, s, esin->sin_addr);
	}
	free(ifc.ifc_buf);
}

/*
 * Print IPv6 addresses for an interface.
 * Uses the SIOCGIFADDR_IN6 ioctl, iterating over all IPv6 addresses
 * by walking the ifconf list entries with AF_INET6 sockaddrs.
 */
static void
show_inet6_addr(const char *name, int s4)
{
	int s6;
	struct ifconf ifc;
	struct ifreq *ifr;
	char *cp, *cplim;

	s6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (s6 < 0)
		return;

	/*
	 * AROSTCP returns IPv6 addresses in the SIOCGIFCONF response
	 * as AF_INET6 sockaddrs when queried on an AF_INET6 socket.
	 * Walk them to find addresses for our interface.
	 */
	if (ip_getifconf(s6, &ifc) < 0) {
		/* Fallback: try SIOCGIFADDR_IN6 for single address */
		struct in6_ifreq ifr6;
		char addr[INET6_ADDRSTRLEN];

		memset(&ifr6, 0, sizeof(ifr6));
		strncpy(ifr6.ifr_name, name, IFNAMSIZ - 1);
		if (IoctlSocket(s6, SIOCGIFADDR_IN6, (caddr_t)&ifr6) >= 0) {
			struct sockaddr_in6 *sin6 =
			    (struct sockaddr_in6 *)&ifr6.ifr_ifru;
			inet_ntop(AF_INET6, &sin6->sin6_addr,
			    addr, sizeof(addr));
			printf("    inet6 %s/64 scope ", addr);
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				printf("link");
			else if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))
				printf("host");
			else
				printf("global");
			printf("\n");
		}
		CloseSocket(s6);
		return;
	}

	cp = ifc.ifc_buf;
	cplim = cp + ifc.ifc_len;
	for (; cp < cplim; ) {
		ifr = (struct ifreq *)cp;

		/* Advance pointer */
		{
			int ifr_sz = sizeof(ifr->ifr_name) +
			    ifr->ifr_addr.sa_len;
			if (ifr_sz < (int)sizeof(*ifr))
				ifr_sz = sizeof(*ifr);
			cp += ifr_sz;
		}

		/* Only AF_INET6 entries for this interface */
		if (ifr->ifr_addr.sa_family != AF_INET6)
			continue;
		if (strcmp(ifr->ifr_name, name) != 0)
			continue;

		{
			struct sockaddr_in6 *sin6 =
			    (struct sockaddr_in6 *)&ifr->ifr_addr;
			char addr[INET6_ADDRSTRLEN];
			int prefix = 64; /* default */
			const char *scope;

			inet_ntop(AF_INET6, &sin6->sin6_addr,
			    addr, sizeof(addr));

			/* Try to get actual prefix via SIOCGIFNETMASK_IN6 */
			{
				struct in6_ifreq nmifr6;
				memset(&nmifr6, 0, sizeof(nmifr6));
				strncpy(nmifr6.ifr_name, name, IFNAMSIZ - 1);
				memcpy(&nmifr6.ifr_ifru, sin6, sizeof(*sin6));
				if (IoctlSocket(s6, SIOCGIFNETMASK_IN6,
				    (caddr_t)&nmifr6) >= 0) {
					int p = mask2prefix(
					    (struct sockaddr *)&nmifr6.ifr_ifru,
					    AF_INET6);
					if (p >= 0)
						prefix = p;
				}
			}

			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				scope = "link";
			else if (IN6_IS_ADDR_LOOPBACK(&sin6->sin6_addr))
				scope = "host";
			else if (IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr))
				scope = "site";
			else
				scope = "global";

			printf("    inet6 %s/%d scope %s", addr, prefix,
			    scope);

			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				printf(" noprefixroute");

			printf("\n");

			if (opts.detail) {
				printf("       valid_lft forever "
				    "preferred_lft forever\n");
			}
		}
	}

	free(ifc.ifc_buf);
	CloseSocket(s6);
}

/*
 * Print link-layer address for an interface.
 */
static void
show_link_addr(const char *name, int s)
{
	struct ifreq ifr;
	unsigned int flags;
	const char *type;
	char llbuf[64];

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

	if (IoctlSocket(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		return;
	flags = (unsigned short)ifr.ifr_flags;

	if (flags & IFF_LOOPBACK)
		type = "loopback";
	else if (flags & IFF_POINTOPOINT)
		type = "ppp";
	else
		type = "ether";

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
	if (IoctlSocket(s, SIOCGIFHWADDR, (caddr_t)&ifr) >= 0) {
		format_lladdr(
		    (const unsigned char *)ifr.ifr_hwaddr.sa_data,
		    6, llbuf, sizeof(llbuf));
		printf("    link/%s %s brd ff:ff:ff:ff:ff:ff\n",
		    type, llbuf);
	} else {
		printf("    link/%s\n", type);
	}
}

static int
show_addrs(const char *dev)
{
	struct ifconf ifc;
	struct ifreq *ifr;
	int s, idx;
	char *cp, *cplim;
	char prevname[IFNAMSIZ];
	char flagbuf[256];
	unsigned int flags;
	int show_inet = (opts.family == AF_UNSPEC || opts.family == AF_INET);
	int show_inet6 = (opts.family == AF_UNSPEC || opts.family == AF_INET6);
	int show_link = (opts.family == AF_UNSPEC || opts.family == AF_LINK);

	s = ip_socket(AF_INET);
	if (s < 0)
		return 1;

	if (ip_getifconf(s, &ifc) < 0) {
		perror("ip: SIOCGIFCONF");
		CloseSocket(s);
		return 1;
	}

	prevname[0] = '\0';
	idx = 0;

	cp = ifc.ifc_buf;
	cplim = cp + ifc.ifc_len;
	for (; cp < cplim; ) {
		struct ifreq tmpifr;

		ifr = (struct ifreq *)cp;

		/* Advance pointer */
		{
			int ifr_sz = sizeof(ifr->ifr_name) +
			    ifr->ifr_addr.sa_len;
			if (ifr_sz < (int)sizeof(*ifr))
				ifr_sz = sizeof(*ifr);
			cp += ifr_sz;
		}

		/* Skip duplicate interface names */
		if (strcmp(ifr->ifr_name, prevname) == 0)
			continue;
		strncpy(prevname, ifr->ifr_name, IFNAMSIZ);

		/* Filter by device name */
		if (dev && strcmp(ifr->ifr_name, dev) != 0)
			continue;

		idx++;

		/* Print interface header */
		memset(&tmpifr, 0, sizeof(tmpifr));
		strncpy(tmpifr.ifr_name, ifr->ifr_name, IFNAMSIZ - 1);
		flags = 0;
		if (IoctlSocket(s, SIOCGIFFLAGS, (caddr_t)&tmpifr) >= 0)
			flags = (unsigned short)tmpifr.ifr_flags;

		format_flags(flags, flagbuf, sizeof(flagbuf));

		{
			int mtu = 0;
			memset(&tmpifr, 0, sizeof(tmpifr));
			strncpy(tmpifr.ifr_name, ifr->ifr_name, IFNAMSIZ - 1);
			if (IoctlSocket(s, SIOCGIFMTU, (caddr_t)&tmpifr) >= 0)
				mtu = tmpifr.ifr_mtu;

			const char *state = "UNKNOWN";
			if (!(flags & IFF_UP))
				state = "DOWN";
			else if (flags & IFF_DRV_RUNNING)
				state = "UP";
			else
				state = "NO-CARRIER";

			printf("%d: %s: <%s> mtu %d state %s\n",
			    idx, ifr->ifr_name, flagbuf, mtu, state);
		}

		/* Link-layer address */
		if (show_link)
			show_link_addr(ifr->ifr_name, s);

		/* IPv4 addresses */
		if (show_inet)
			show_inet_addr(ifr->ifr_name, s);

		/* IPv6 addresses */
		if (show_inet6)
			show_inet6_addr(ifr->ifr_name, s);
	}

	free(ifc.ifc_buf);
	CloseSocket(s);
	return 0;
}

int
do_ipaddr(int argc, char **argv)
{
	const char *dev = NULL;

	/* Skip "addr" */
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "show") == 0)) {
		if (argc > 0) {
			argc--;
			argv++;
		}
		while (argc > 0) {
			if (strcmp(argv[0], "dev") == 0) {
				if (argc < 2) {
					addr_usage();
					return 1;
				}
				dev = argv[1];
				argc -= 2;
				argv += 2;
			} else {
				dev = argv[0];
				argc--;
				argv++;
			}
		}
		return show_addrs(dev);
	}

	if (strcmp(argv[0], "help") == 0) {
		addr_usage();
		return 0;
	}

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip addr help\".\n",
	    argv[0]);
	return 1;
}
