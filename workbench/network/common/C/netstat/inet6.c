/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 *
 * IPv6 protocol statistics and connection display for netstat.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/icmp6.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include "netstat.h"

/*
 * Local copy of ip6stat — must match the kernel's struct ip6stat
 * in netinet6/in6_var.h.  Read via KVM.
 */
struct ip6stat {
	u_long ip6s_total;
	u_long ip6s_tooshort;
	u_long ip6s_toosmall;
	u_long ip6s_fragments;
	u_long ip6s_fragdropped;
	u_long ip6s_fragtimeout;
	u_long ip6s_fragoverflow;
	u_long ip6s_forward;
	u_long ip6s_cantforward;
	u_long ip6s_redirectsent;
	u_long ip6s_delivered;
	u_long ip6s_localout;
	u_long ip6s_odropped;
	u_long ip6s_reassembled;
	u_long ip6s_fragmented;
	u_long ip6s_ofragments;
	u_long ip6s_cantfrag;
	u_long ip6s_badoptions;
	u_long ip6s_noroute;
	u_long ip6s_badvers;
	u_long ip6s_rawout;
	u_long ip6s_badscope;
	u_long ip6s_notmember;
	u_long ip6s_nxthist[256];
	u_long ip6s_m1;
	u_long ip6s_m2m[32];
	u_long ip6s_mext1;
	u_long ip6s_mext2m;
	u_long ip6s_exthdrtoolong;
	u_long ip6s_nogif;
	u_long ip6s_toomanyhdr;
};

/*
 * ICMPv6 type names for histogram display.
 */
static const char *icmp6names[] = {
	"#0",
	"destination unreachable",	/* 1 */
	"packet too big",		/* 2 */
	"time exceeded",		/* 3 */
	"parameter problem",		/* 4 */
};
#define NICMP6NAMES (sizeof(icmp6names) / sizeof(icmp6names[0]))

static const char *icmp6_echonames[] = {
	"echo request",			/* 128 */
	"echo reply",			/* 129 */
};

static const char *icmp6_ndnames[] = {
	"router solicitation",		/* 133 */
	"router advertisement",		/* 134 */
	"neighbor solicitation",	/* 135 */
	"neighbor advertisement",	/* 136 */
	"redirect",			/* 137 */
};

static const char *
icmp6_typename(int type)
{
	static char buf[32];

	if (type < (int)NICMP6NAMES)
		return icmp6names[type];
	if (type >= 128 && type <= 129)
		return icmp6_echonames[type - 128];
	if (type >= 133 && type <= 137)
		return icmp6_ndnames[type - 133];
	snprintf(buf, sizeof(buf), "#%d", type);
	return buf;
}

/*
 * Dump IPv6 statistics.
 */
void
ip6_stats(off, name)
	u_long off;
	char *name;
{
	struct ip6stat ip6stat;

	if (off == 0)
		return;
	kread(off, (char *)&ip6stat, sizeof(ip6stat));
	printf("%s:\n", name);

#define	p(f, m) if (ip6stat.f || sflag <= 1) \
    printf(m, ip6stat.f, plural(ip6stat.f))

	p(ip6s_total, "\t%u total packet%s received\n");
	p(ip6s_tooshort, "\t%u with data size < data length\n");
	p(ip6s_toosmall, "\t%u with size smaller than minimum\n");
	p(ip6s_badvers, "\t%u with incorrect version number\n");
	p(ip6s_badscope, "\t%u with bad scope\n");
	p(ip6s_badoptions, "\t%u with bad options\n");
	p(ip6s_fragments, "\t%u fragment%s received\n");
	p(ip6s_fragdropped, "\t%u fragment%s dropped (dup or out of space)\n");
	p(ip6s_fragtimeout, "\t%u fragment%s dropped after timeout\n");
	p(ip6s_fragoverflow, "\t%u fragment%s that exceeded limit\n");
	p(ip6s_reassembled, "\t%u packet%s reassembled ok\n");
	p(ip6s_delivered, "\t%u packet%s for this host\n");
	p(ip6s_forward, "\t%u packet%s forwarded\n");
	p(ip6s_cantforward, "\t%u packet%s not forwardable\n");
	p(ip6s_redirectsent, "\t%u redirect%s sent\n");
	p(ip6s_localout, "\t%u packet%s sent from this host\n");
	p(ip6s_rawout, "\t%u packet%s sent with fabricated ip header\n");
	p(ip6s_odropped, "\t%u output packet%s dropped due to no bufs, etc.\n");
	p(ip6s_noroute, "\t%u output packet%s discarded due to no route\n");
	p(ip6s_fragmented, "\t%u output datagram%s fragmented\n");
	p(ip6s_ofragments, "\t%u fragment%s created\n");
	p(ip6s_cantfrag, "\t%u datagram%s that can't be fragmented\n");
	p(ip6s_notmember, "\t%u multicast packet%s dropped (not a member)\n");
	p(ip6s_exthdrtoolong, "\t%u packet%s with ext headers too long\n");
	p(ip6s_toomanyhdr, "\t%u packet%s discarded due to too many headers\n");
#undef p
}

/*
 * Dump ICMPv6 statistics.
 */
void
icmp6_stats(off, name)
	u_long off;
	char *name;
{
	struct icmp6stat icmp6stat;
	int i, first;

	if (off == 0)
		return;
	kread(off, (char *)&icmp6stat, sizeof(icmp6stat));
	printf("%s:\n", name);

#define	p(f, m) if (icmp6stat.f || sflag <= 1) \
    printf(m, (u_long)icmp6stat.f, plural((int)icmp6stat.f))

	p(icp6s_error, "\t%lu call%s to icmp6_error\n");
	p(icp6s_canterror, "\t%lu error%s not generated because old message was icmp6\n");
	p(icp6s_toofreq, "\t%lu error%s not generated due to rate limitation\n");

	for (first = 1, i = 0; i < 256; i++) {
		if (icmp6stat.icp6s_outhist[i] != 0) {
			if (first) {
				printf("\tOutput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmp6_typename(i),
			    (u_long)icmp6stat.icp6s_outhist[i]);
		}
	}

	p(icp6s_badcode, "\t%lu message%s with bad code fields\n");
	p(icp6s_tooshort, "\t%lu message%s < minimum length\n");
	p(icp6s_checksum, "\t%lu bad checksum%s\n");
	p(icp6s_badlen, "\t%lu message%s with bad length\n");
	p(icp6s_dropped, "\t%lu message%s dropped waiting for resolution\n");

	for (first = 1, i = 0; i < 256; i++) {
		if (icmp6stat.icp6s_inhist[i] != 0) {
			if (first) {
				printf("\tInput histogram:\n");
				first = 0;
			}
			printf("\t\t%s: %lu\n", icmp6_typename(i),
			    (u_long)icmp6stat.icp6s_inhist[i]);
		}
	}

	p(icp6s_reflect, "\t%lu message response%s generated\n");
	p(icp6s_nd_toomanyopt, "\t%lu message%s with too many ND options\n");
	p(icp6s_nd_badopt, "\t%lu message%s with bad ND options\n");
	p(icp6s_badns, "\t%lu bad neighbor solicitation%s\n");
	p(icp6s_badna, "\t%lu bad neighbor advertisement%s\n");
	p(icp6s_badrs, "\t%lu bad router solicitation%s\n");
	p(icp6s_badra, "\t%lu bad router advertisement%s\n");
	p(icp6s_badredirect, "\t%lu bad redirect message%s\n");
#undef p
}

/*
 * Pretty print an IPv6 address (numeric only for now).
 */
char *
inet6name(in6)
	struct in6_addr *in6;
{
	static char line[NI_MAXHOST];
	static const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;

	if (memcmp(in6, &in6addr_any, sizeof(*in6)) == 0) {
		strcpy(line, "*");
	} else {
		if (inet_ntop(AF_INET6, in6, line, sizeof(line)) == NULL)
			strcpy(line, "?");
	}
	return line;
}

/*
 * Print an IPv6 address + port.
 */
void
inet6print(in6, port, proto)
	struct in6_addr *in6;
	int port;
	char *proto;
{
	struct servent *sp = 0;
	char line[80], *cp;
	int width;

	snprintf(line, sizeof(line), "%s.", inet6name(in6));
	cp = line + strlen(line);
	if (!nflag && port)
		sp = getservbyport((int)port, proto);
	if (sp || port == 0)
		snprintf(cp, sizeof(line) - (cp - line), "%.8s",
		    sp ? sp->s_name : "*");
	else
		snprintf(cp, sizeof(line) - (cp - line), "%d",
		    ntohs((u_short)port));
	width = Aflag ? 18 : 22;
	printf(" %-*.*s", width, width, line);
}
