/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip route - display routing tables (IPv4 and IPv6)
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
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ip.h"

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

static void
route_usage(void)
{
	fprintf(stderr,
	    "Usage: ip route show [ table all ]\n"
	    "       ip -6 route show\n"
	    "       ip route get ADDRESS\n"
	    "       ip route help\n");
}

/*
 * Extract sockaddrs from a routing message per rtm_addrs bitmask.
 */
static void
rt_xaddrs(const struct rt_msghdr *rtm, struct sockaddr **addrs)
{
	char *cp = (char *)(rtm + 1);
	int i;

	for (i = 0; i < RTAX_MAX; i++)
		addrs[i] = NULL;

	for (i = 0; i < RTAX_MAX; i++) {
		if (rtm->rtm_addrs & (1 << i)) {
			struct sockaddr *sa = (struct sockaddr *)cp;
			addrs[i] = sa;
			ADVANCE(cp, sa);
		}
	}
}

/*
 * Format destination + prefix from route entry.
 */
static void
format_route_dst(struct sockaddr *dst, struct sockaddr *mask,
    int rtflags, char *buf, size_t buflen)
{
	char addr[128];
	int prefix;

	if (dst == NULL) {
		snprintf(buf, buflen, "???");
		return;
	}

	/* Default route */
	if (dst->sa_family == AF_INET) {
		struct sockaddr_in *sin = (struct sockaddr_in *)dst;
		if (sin->sin_addr.s_addr == INADDR_ANY) {
			snprintf(buf, buflen, "default");
			return;
		}
	}
#ifdef INET6
	if (dst->sa_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)dst;
		if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr)) {
			snprintf(buf, buflen, "default");
			return;
		}
	}
#endif

	format_sockaddr(dst, addr, sizeof(addr));

	if (rtflags & RTF_HOST) {
		snprintf(buf, buflen, "%s", addr);
	} else {
		prefix = mask2prefix(mask);
		if (prefix >= 0)
			snprintf(buf, buflen, "%s/%d", addr, prefix);
		else
			snprintf(buf, buflen, "%s", addr);
	}
}

/*
 * Find the interface name associated with a gateway sockaddr_dl.
 */
static const char *
ifname_from_sdl(struct sockaddr *sa, char *buf, size_t len)
{
	if (sa && sa->sa_family == AF_LINK) {
		struct sockaddr_dl *sdl = (struct sockaddr_dl *)sa;
		if (sdl->sdl_nlen > 0 && sdl->sdl_nlen < len) {
			memcpy(buf, sdl->sdl_data, sdl->sdl_nlen);
			buf[sdl->sdl_nlen] = '\0';
			return buf;
		}
	}
	return NULL;
}

/*
 * Print one routing table entry in iproute2-style format:
 *   192.168.1.0/24 dev eth0 proto kernel scope link src 192.168.1.100
 *   default via 192.168.1.1 dev eth0
 */
static void
print_route(struct rt_msghdr *rtm)
{
	struct sockaddr *addrs[RTAX_MAX];
	char dstbuf[128], gwbuf[128], ifbuf[IFNAMSIZ];
	const char *ifname;

	if (!(rtm->rtm_flags & RTF_UP))
		return;

	rt_xaddrs(rtm, addrs);

	format_route_dst(addrs[RTAX_DST], addrs[RTAX_NETMASK],
	    rtm->rtm_flags, dstbuf, sizeof(dstbuf));

	printf("%s", dstbuf);

	/* Gateway */
	if (addrs[RTAX_GATEWAY]) {
		if (addrs[RTAX_GATEWAY]->sa_family == AF_LINK) {
			ifname = ifname_from_sdl(addrs[RTAX_GATEWAY],
			    ifbuf, sizeof(ifbuf));
			if (ifname)
				printf(" dev %s", ifname);
			else {
				format_sockaddr(addrs[RTAX_GATEWAY],
				    gwbuf, sizeof(gwbuf));
				printf(" dev %s", gwbuf);
			}
		} else {
			format_sockaddr(addrs[RTAX_GATEWAY],
			    gwbuf, sizeof(gwbuf));
			printf(" via %s", gwbuf);
		}
	}

	/* Interface from IFA */
	if (addrs[RTAX_IFP]) {
		ifname = ifname_from_sdl(addrs[RTAX_IFP],
		    ifbuf, sizeof(ifbuf));
		if (ifname && !addrs[RTAX_GATEWAY])
			printf(" dev %s", ifname);
		else if (ifname && addrs[RTAX_GATEWAY] &&
		    addrs[RTAX_GATEWAY]->sa_family != AF_LINK)
			printf(" dev %s", ifname);
	}

	/* Protocol hint */
	if (rtm->rtm_flags & RTF_DYNAMIC)
		printf(" proto redirect");
	else if (rtm->rtm_flags & RTF_STATIC)
		printf(" proto static");
	else
		printf(" proto kernel");

	/* Scope */
	if (rtm->rtm_flags & RTF_HOST)
		printf(" scope host");
	else if (rtm->rtm_flags & RTF_GATEWAY)
		printf(" scope global");
	else
		printf(" scope link");

	/* Source address (IFA) */
	if (addrs[RTAX_IFA]) {
		char srcbuf[128];
		format_sockaddr(addrs[RTAX_IFA], srcbuf, sizeof(srcbuf));
		printf(" src %s", srcbuf);
	}

	/* Metrics */
	if (opts.detail) {
		if (rtm->rtm_rmx.rmx_mtu)
			printf(" mtu %lu", (unsigned long)rtm->rtm_rmx.rmx_mtu);
		if (rtm->rtm_rmx.rmx_expire)
			printf(" expires %lds",
			    (long)rtm->rtm_rmx.rmx_expire);
	}

	/* Flags in detail mode */
	if (opts.detail) {
		printf(" flags [");
		if (rtm->rtm_flags & RTF_UP) printf("U");
		if (rtm->rtm_flags & RTF_GATEWAY) printf("G");
		if (rtm->rtm_flags & RTF_HOST) printf("H");
		if (rtm->rtm_flags & RTF_REJECT) printf("!");
		if (rtm->rtm_flags & RTF_DYNAMIC) printf("D");
		if (rtm->rtm_flags & RTF_MODIFIED) printf("M");
		if (rtm->rtm_flags & RTF_CLONING) printf("C");
		if (rtm->rtm_flags & RTF_LLINFO) printf("L");
		printf("]");
	}

	printf("\n");
}

static int
show_routes_family(int family)
{
	struct rt_msghdr *buf, *rtm;
	char *next;

	buf = GetRouteInfo(family, 0);
	if (buf == NULL)
		return 0;

	for (next = (char *)buf; ; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *)next;
		if (rtm->rtm_msglen == 0)
			break;
		print_route(rtm);
	}

	FreeRouteInfo(buf);
	return 0;
}

static int
show_routes(int show_all)
{
	int ret = 0;

	if (opts.family == AF_INET || opts.family == AF_UNSPEC)
		ret |= show_routes_family(AF_INET);

	if (opts.family == AF_INET6 || opts.family == AF_UNSPEC)
		ret |= show_routes_family(AF_INET6);

	return ret;
}

/*
 * ip route get ADDRESS - lookup what route would be used.
 * Uses PF_ROUTE socket with RTM_GET message.
 */
static int
route_get(char *dest)
{
	int s, n;
	struct {
		struct rt_msghdr rtm;
		struct sockaddr_in6 dst;	/* large enough for both */
	} msg;
	struct rt_msghdr *rtm = &msg.rtm;
	int family;

	memset(&msg, 0, sizeof(msg));

	/* Determine address family */
	if (opts.family == AF_INET6 || strchr(dest, ':')) {
		struct sockaddr_in6 *sin6 = &msg.dst;
		family = AF_INET6;
		sin6->sin6_family = AF_INET6;
		sin6->sin6_len = sizeof(*sin6);
		if (inet_pton(AF_INET6, dest, &sin6->sin6_addr) != 1) {
			fprintf(stderr, "ip: invalid IPv6 address \"%s\"\n",
			    dest);
			return 1;
		}
		rtm->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(*sin6);
	} else {
		struct sockaddr_in *sin = (struct sockaddr_in *)&msg.dst;
		family = AF_INET;
		sin->sin_family = AF_INET;
		sin->sin_len = sizeof(*sin);
		if (inet_pton(AF_INET, dest, &sin->sin_addr) != 1) {
			fprintf(stderr, "ip: invalid address \"%s\"\n", dest);
			return 1;
		}
		rtm->rtm_msglen = sizeof(struct rt_msghdr) +
		    sizeof(struct sockaddr_in);
	}

	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = RTM_GET;
	rtm->rtm_addrs = RTA_DST;
	rtm->rtm_pid = (int)(long)FindTask(NULL);
	rtm->rtm_seq = 1;

	s = socket(PF_ROUTE, SOCK_RAW, 0);
	if (s < 0) {
		perror("ip: PF_ROUTE socket");
		return 1;
	}

	if (send(s, &msg, rtm->rtm_msglen, 0) < 0) {
		perror("ip: route get send");
		CloseSocket(s);
		return 1;
	}

	n = recv(s, &msg, sizeof(msg), 0);
	CloseSocket(s);

	if (n < (int)sizeof(struct rt_msghdr)) {
		fprintf(stderr, "ip: route get: short reply\n");
		return 1;
	}

	/* Print the result */
	print_route(rtm);

	return 0;
}

int
do_iproute(int argc, char **argv)
{
	int show_all = 0;

	/* Skip "route" */
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "show") == 0) ||
	    (argc >= 1 && strcmp(argv[0], "list") == 0)) {
		if (argc > 0) {
			argc--;
			argv++;
		}
		while (argc > 0) {
			if (strcmp(argv[0], "table") == 0) {
				if (argc >= 2 && strcmp(argv[1], "all") == 0) {
					show_all = 1;
					argc -= 2;
					argv += 2;
				} else {
					argc--;
					argv++;
				}
			} else {
				argc--;
				argv++;
			}
		}
		return show_routes(show_all);
	}

	if (strcmp(argv[0], "get") == 0) {
		if (argc < 2) {
			fprintf(stderr, "ip route get: missing address\n");
			return 1;
		}
		return route_get(argv[1]);
	}

	if (strcmp(argv[0], "help") == 0) {
		route_usage();
		return 0;
	}

	fprintf(stderr, "Command \"%s\" is unknown, try \"ip route help\".\n",
	    argv[0]);
	return 1;
}
