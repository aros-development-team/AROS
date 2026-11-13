/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip link - display and manage network interfaces
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
#include <net/if_arp.h>
#include <netinet/in.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

static void
link_usage(void)
{
	fprintf(stderr,
	    "Usage: ip link show [ dev DEVICE ]\n"
	    "       ip link set [ dev ] DEVICE { up | down | mtu MTU }\n"
	    "       ip link help\n");
}

/*
 * Print a single interface in ip-link style:
 *   1: lo: <LOOPBACK,UP,RUNNING> mtu 16384 state UNKNOWN
 *       link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
 */
static void
print_link(int idx, const char *name, int s)
{
	struct ifreq ifr;
	char flagbuf[256];
	char llbuf[64];
	const char *type;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);

	/* Get flags */
	if (IoctlSocket(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0)
		return;
	unsigned int flags = (unsigned short)ifr.ifr_flags;

	format_flags(flags, flagbuf, sizeof(flagbuf));

	/* Get MTU */
	int mtu = 0;
	if (IoctlSocket(s, SIOCGIFMTU, (caddr_t)&ifr) >= 0)
		mtu = ifr.ifr_mtu;

	/* Determine state */
	const char *state = "UNKNOWN";
	if (!(flags & IFF_UP))
		state = "DOWN";
	else if (flags & IFF_DRV_RUNNING)
		state = "UP";
	else
		state = "NO-CARRIER";

	printf("%d: %s: <%s> mtu %d state %s\n",
	    idx, name, flagbuf, mtu, state);

	/* Get link-layer type and address */
	if (flags & IFF_LOOPBACK)
		type = "loopback";
	else if (flags & IFF_POINTOPOINT)
		type = "ppp";
	else
		type = "ether";

	/* Try to get hardware address */
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

	/* Show stats if requested */
	if (opts.stats) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
		if (IoctlSocket(s, SIOCGIFMETRIC, (caddr_t)&ifr) >= 0 &&
		    ifr.ifr_metric != 0) {
			printf("    metric %d\n", ifr.ifr_metric);
		}
	}
}

static int
show_links(const char *dev)
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
		ifr = (struct ifreq *)cp;

		/* Advance pointer */
		{
			int ifr_sz = sizeof(ifr->ifr_name) +
			    ifr->ifr_addr.sa_len;
			if (ifr_sz < (int)sizeof(*ifr))
				ifr_sz = sizeof(*ifr);
			cp += ifr_sz;
		}

		/* Skip duplicate interface names (multiple addrs) */
		if (strcmp(ifr->ifr_name, prevname) == 0)
			continue;
		strncpy(prevname, ifr->ifr_name, IFNAMSIZ);

		/* Filter by device name if specified */
		if (dev && strcmp(ifr->ifr_name, dev) != 0)
			continue;

		idx++;
		print_link(idx, ifr->ifr_name, s);
	}

	CloseSocket(s);
	return 0;
}

/*
 * ip link set [ dev ] DEVICE { up | down | mtu MTU }
 *
 * mtu maps to SIOCSIFMTU (bounded by the driver's configured frame size);
 * up/down toggle IFF_UP via SIOCGIFFLAGS/SIOCSIFFLAGS.
 */
static int
set_link(int argc, char **argv)
{
	const char *dev = NULL;
	int mtu = -1;
	int updown = 0;			/* +1 = up, -1 = down */
	struct ifreq ifr;
	int s, ret = 0;

	while (argc > 0) {
		if (strcmp(argv[0], "dev") == 0) {
			if (argc < 2) { link_usage(); return 1; }
			dev = argv[1]; argc -= 2; argv += 2;
		} else if (strcmp(argv[0], "mtu") == 0) {
			if (argc < 2) { link_usage(); return 1; }
			mtu = atoi(argv[1]); argc -= 2; argv += 2;
		} else if (strcmp(argv[0], "up") == 0) {
			updown = 1; argc--; argv++;
		} else if (strcmp(argv[0], "down") == 0) {
			updown = -1; argc--; argv++;
		} else if (dev == NULL) {
			dev = argv[0]; argc--; argv++;	/* bare device name */
		} else {
			fprintf(stderr, "ip: unknown \"link set\" option \"%s\"\n",
			    argv[0]);
			return 1;
		}
	}

	if (dev == NULL || (mtu < 0 && updown == 0)) {
		link_usage();
		return 1;
	}

	s = ip_socket(AF_INET);
	if (s < 0)
		return 1;

	if (mtu >= 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
		ifr.ifr_mtu = mtu;
		if (IoctlSocket(s, SIOCSIFMTU, (caddr_t)&ifr) < 0) {
			perror("ip: SIOCSIFMTU");
			ret = 1;
		}
	}

	if (updown != 0) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
		if (IoctlSocket(s, SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
			perror("ip: SIOCGIFFLAGS");
			CloseSocket(s);
			return 1;
		}
		{
			unsigned short flags = (unsigned short)ifr.ifr_flags;
			if (updown > 0)
				flags |= IFF_UP;
			else
				flags &= ~IFF_UP;
			ifr.ifr_flags = (short)flags;
		}
		if (IoctlSocket(s, SIOCSIFFLAGS, (caddr_t)&ifr) < 0) {
			perror("ip: SIOCSIFFLAGS");
			ret = 1;
		}
	}

	CloseSocket(s);
	return ret;
}

int
do_iplink(int argc, char **argv)
{
	const char *dev = NULL;

	/* Skip "link" */
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "show") == 0)) {
		/* ip link show [dev NAME] */
		argc--;
		argv++;
		while (argc > 0) {
			if (strcmp(argv[0], "dev") == 0) {
				if (argc < 2) {
					link_usage();
					return 1;
				}
				dev = argv[1];
				argc -= 2;
				argv += 2;
			} else {
				/* Bare device name */
				dev = argv[0];
				argc--;
				argv++;
			}
		}
		return show_links(dev);
	}

	if (strcmp(argv[0], "set") == 0) {
		/* ip link set [dev] DEVICE { up | down | mtu MTU } */
		return set_link(argc - 1, argv + 1);
	}

	if (strcmp(argv[0], "help") == 0) {
		link_usage();
		return 0;
	}

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip link help\".\n",
	    argv[0]);
	return 1;
}
