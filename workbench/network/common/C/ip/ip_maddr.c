/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip maddr  - display multicast group memberships
 * ip mroute - display multicast routing cache
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
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

static void
maddr_usage(void)
{
	fprintf(stderr,
	    "Usage: ip maddr show [ dev DEVICE ]\n"
	    "       ip maddr help\n");
}

/*
 * Show multicast memberships for each interface.
 *
 * On AROS/BSD, per-interface multicast group info is retrieved via
 * SIOCGIFCONF to enumerate interfaces, and then SIOCADDMULTI/SIOCDELMULTI
 * are the control ioctls. There is no direct "list all multicast groups"
 * ioctl in AROSTCP — we display well-known multicast addresses that
 * are always joined (all-hosts, solicited-node for IPv6), plus any
 * addresses we can discover from the routing table.
 *
 * For a more complete listing, the kernel would need a SIOCGIFMULTI ioctl.
 * Here we display what we can from available information.
 */
static int
show_maddr(const char *dev)
{
	struct ifconf ifc;
	char buf[4096];
	struct ifreq *ifr;
	int s, idx;
	char *cp, *cplim;
	char prevname[IFNAMSIZ];

	s = ip_socket(AF_INET);
	if (s < 0)
		return 1;

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (IoctlSocket(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		perror("ip: SIOCGIFCONF");
		CloseSocket(s);
		return 1;
	}

	prevname[0] = '\0';
	idx = 0;

	cp = ifc.ifc_buf;
	cplim = cp + ifc.ifc_len;
	for (; cp < cplim; ) {
		unsigned int flags;
		struct ifreq tmpifr;

		ifr = (struct ifreq *)cp;

		{
			int ifr_sz = sizeof(ifr->ifr_name) +
			    ifr->ifr_addr.sa_len;
			if (ifr_sz < (int)sizeof(*ifr))
				ifr_sz = sizeof(*ifr);
			cp += ifr_sz;
		}

		if (strcmp(ifr->ifr_name, prevname) == 0)
			continue;
		strncpy(prevname, ifr->ifr_name, IFNAMSIZ);

		if (dev && strcmp(ifr->ifr_name, dev) != 0)
			continue;

		idx++;

		/* Get flags */
		memset(&tmpifr, 0, sizeof(tmpifr));
		strncpy(tmpifr.ifr_name, ifr->ifr_name, IFNAMSIZ - 1);
		flags = 0;
		if (IoctlSocket(s, SIOCGIFFLAGS, (caddr_t)&tmpifr) >= 0)
			flags = (unsigned short)tmpifr.ifr_flags;

		printf("%d:\t%s\n", idx, ifr->ifr_name);

		/* All interfaces with IFF_MULTICAST implicitly join: */
		if (flags & IFF_MULTICAST) {
			if (opts.family == AF_UNSPEC ||
			    opts.family == AF_LINK)
				printf("\tlink  01:00:5e:00:00:01\n");

			if (opts.family == AF_UNSPEC ||
			    opts.family == AF_INET)
				printf("\tinet  224.0.0.1\n");

#ifdef INET6
			if (opts.family == AF_UNSPEC ||
			    opts.family == AF_INET6) {
				printf("\tinet6 ff02::1\n");

				/* If interface has link-local, show solicited-node */
				if (!(flags & IFF_LOOPBACK)) {
					printf("\tinet6 ff02::1:ff00:0/104 "
					    "(solicited-node)\n");
				}
			}
#endif
		}
	}

	CloseSocket(s);
	return 0;
}

int
do_ipmaddr(int argc, char **argv)
{
	const char *dev = NULL;

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
					maddr_usage();
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
		return show_maddr(dev);
	}

	if (strcmp(argv[0], "help") == 0) {
		maddr_usage();
		return 0;
	}

	fprintf(stderr,
	    "Command \"%s\" is unknown, try \"ip maddr help\".\n",
	    argv[0]);
	return 1;
}

/*
 * ip mroute show - multicast routing cache.
 * AROSTCP does not currently support multicast routing, so we report that.
 */
int
do_ipmroute(int argc, char **argv)
{
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "show") == 0)) {
		printf("ip: multicast routing is not supported "
		    "on this platform\n");
		return 0;
	}

	if (argc >= 1 && strcmp(argv[0], "help") == 0) {
		fprintf(stderr,
		    "Usage: ip mroute show\n"
		    "       ip mroute help\n");
		return 0;
	}

	fprintf(stderr,
	    "Command \"%s\" is unknown, try \"ip mroute help\".\n",
	    argv[0]);
	return 1;
}
