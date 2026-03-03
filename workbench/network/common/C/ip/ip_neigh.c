/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip neigh - display neighbor cache (ARP for IPv4, NDP for IPv6)
 */

#define INET6 1

#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/socket.h>
#include <proto/miami.h>
#include <libraries/bsdsocket.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void
neigh_usage(void)
{
	fprintf(stderr,
	    "Usage: ip neigh show [ dev DEVICE ]\n"
	    "       ip -6 neigh show\n"
	    "       ip neigh help\n");
}

/*
 * Show IPv4 ARP neighbors via SIOCGARPT ioctl.
 * Format: 192.168.1.1 dev eth0 lladdr aa:bb:cc:dd:ee:ff REACHABLE
 */
static int
show_arp_neighbors(const char *dev)
{
	struct arptabreq atr;
	struct arpreq *ar;
	int s, i;
	long n;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("ip: socket");
		return 1;
	}

	memset(&atr, 0, sizeof(atr));

	/* Set up address for the ioctl — use INADDR_ANY for "all" */
	{
		struct sockaddr_in *sin =
		    (struct sockaddr_in *)&atr.atr_arpreq.arp_pa;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_addr.s_addr = INADDR_ANY;
	}

	/* First call: get count */
	if (IoctlSocket(s, SIOCGARPT, (caddr_t)&atr) < 0) {
		/* Not supported — skip ARP display silently */
		CloseSocket(s);
		return 0;
	}

	n = atr.atr_inuse;
	if (n == 0) {
		CloseSocket(s);
		return 0;
	}

	atr.atr_size = n;
	ar = (struct arpreq *)malloc(sizeof(*ar) * n);
	if (ar == NULL) {
		perror("ip: malloc");
		CloseSocket(s);
		return 1;
	}
	atr.atr_table = ar;

	if (IoctlSocket(s, SIOCGARPT, (caddr_t)&atr) < 0) {
		perror("ip: SIOCGARPT");
		free(ar);
		CloseSocket(s);
		return 1;
	}

	n = atr.atr_inuse;

	for (i = 0; i < n; i++) {
		struct sockaddr_in *sin =
		    (struct sockaddr_in *)&ar[i].arp_pa;
		char addrbuf[INET_ADDRSTRLEN];
		char llbuf[64];
		const char *state;

		/* Filter by dev if requested */
		if (dev && ar[i].arp_ha.sa_data[0] != '\0') {
			/* arp_dev is not reliably available on AROS */
		}

		inet_ntop(AF_INET, &sin->sin_addr, addrbuf, sizeof(addrbuf));

		format_lladdr(
		    (const unsigned char *)ar[i].arp_ha.sa_data,
		    6, llbuf, sizeof(llbuf));

		/* Determine state from flags */
		if (ar[i].arp_flags & ATF_COM)
			state = "REACHABLE";
		else if (ar[i].arp_flags & ATF_PERM)
			state = "PERMANENT";
		else
			state = "INCOMPLETE";

		printf("%s lladdr %s", addrbuf, llbuf);

		if (ar[i].arp_flags & ATF_PUBL)
			printf(" PUBLISHED");

		printf(" %s\n", state);
	}

	free(ar);
	CloseSocket(s);
	return 0;
}

/*
 * Show IPv6 NDP neighbors from the routing table.
 * NDP neighbor entries appear as RTF_HOST | RTF_LLINFO routes with
 * AF_LINK gateways in the AF_INET6 routing table.
 */
static int
show_ndp_neighbors(const char *dev)
{
	struct rt_msghdr *buf, *rtm;
	char *next;

	buf = GetRouteInfo(AF_INET6, RTF_LLINFO);
	if (buf == NULL) {
		/* Also try without flag filter */
		buf = GetRouteInfo(AF_INET6, 0);
		if (buf == NULL)
			return 0;
	}

	for (next = (char *)buf; ; next += rtm->rtm_msglen) {
		struct sockaddr *addrs[8];
		char *cp;
		int i;

		rtm = (struct rt_msghdr *)next;
		if (rtm->rtm_msglen == 0)
			break;

		/* Only interested in host routes with link-layer info */
		if (!(rtm->rtm_flags & RTF_HOST))
			continue;
		if (!(rtm->rtm_flags & RTF_LLINFO) &&
		    !(rtm->rtm_flags & RTF_UP))
			continue;

		/* Parse sockaddrs */
		for (i = 0; i < 8; i++)
			addrs[i] = NULL;
		cp = (char *)(rtm + 1);
		for (i = 0; i < 8; i++) {
			if (rtm->rtm_addrs & (1 << i)) {
				struct sockaddr *sa = (struct sockaddr *)cp;
				addrs[i] = sa;
				ADVANCE(cp, sa);
			}
		}

		/* Need DST (IPv6 addr) and GATEWAY (link-layer addr) */
		if (addrs[RTAX_DST] == NULL)
			continue;
		if (addrs[RTAX_DST]->sa_family != AF_INET6)
			continue;

		{
			struct sockaddr_in6 *sin6 =
			    (struct sockaddr_in6 *)addrs[RTAX_DST];
			char addr6[INET6_ADDRSTRLEN];
			char llbuf[64];
			const char *state;
			const char *ifname = NULL;
			char ifbuf[IFNAMSIZ];

			inet_ntop(AF_INET6, &sin6->sin6_addr,
			    addr6, sizeof(addr6));

			/* Get link-layer address from gateway */
			llbuf[0] = '\0';
			if (addrs[RTAX_GATEWAY] &&
			    addrs[RTAX_GATEWAY]->sa_family == AF_LINK) {
				struct sockaddr_dl *sdl =
				    (struct sockaddr_dl *)addrs[RTAX_GATEWAY];
				if (sdl->sdl_alen > 0) {
					format_lladdr(
					    (const unsigned char *)LLADDR(sdl),
					    sdl->sdl_alen, llbuf,
					    sizeof(llbuf));
				}
				if (sdl->sdl_nlen > 0 &&
				    sdl->sdl_nlen < IFNAMSIZ) {
					memcpy(ifbuf, sdl->sdl_data,
					    sdl->sdl_nlen);
					ifbuf[sdl->sdl_nlen] = '\0';
					ifname = ifbuf;
				}
			}

			/* Filter by dev */
			if (dev && ifname && strcmp(dev, ifname) != 0)
				continue;

			/* Determine NDP state from route flags/metrics */
			if (rtm->rtm_flags & RTF_REJECT)
				state = "FAILED";
			else if (llbuf[0] == '\0')
				state = "INCOMPLETE";
			else if (rtm->rtm_rmx.rmx_expire == 0)
				state = "PERMANENT";
			else
				state = "REACHABLE";

			printf("%s", addr6);
			if (ifname)
				printf(" dev %s", ifname);
			if (llbuf[0] != '\0')
				printf(" lladdr %s", llbuf);
			printf(" %s\n", state);
		}
	}

	FreeRouteInfo(buf);
	return 0;
}

int
do_ipneigh(int argc, char **argv)
{
	const char *dev = NULL;
	int ret = 0;

	/* Skip "neigh" */
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
					neigh_usage();
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

		if (opts.family == AF_INET || opts.family == AF_UNSPEC)
			ret |= show_arp_neighbors(dev);

		if (opts.family == AF_INET6 || opts.family == AF_UNSPEC)
			ret |= show_ndp_neighbors(dev);

		return ret;
	}

	if (strcmp(argv[0], "help") == 0) {
		neigh_usage();
		return 0;
	}

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip neigh help\".\n",
	    argv[0]);
	return 1;
}
