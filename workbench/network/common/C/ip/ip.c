/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ip - unified network configuration tool for AROS
 *
 * Inspired by iproute2's "ip" command on Linux, adapted to AROS/BSD
 * networking APIs. Provides a single command to query interfaces,
 * addresses, routes, neighbors (ARP/NDP), and multicast memberships.
 *
 * Usage:  ip [OPTIONS] OBJECT [COMMAND]
 * Objects: link, addr, route, neigh, maddr, mroute, rule, netns
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
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ip.h"

const TEXT version[] = "$VER: ip 1.0 (03.03.2026)";

/* Global options */
struct ipcmd_opts opts;

static void
usage(void)
{
	fprintf(stderr,
	    "Usage: ip [ OPTIONS ] OBJECT { COMMAND | help }\n"
	    "       ip [ -force ] -batch filename\n"
	    "where  OBJECT := { link | addr | route | neigh | maddr |\n"
	    "                    mroute | rule | netns }\n"
	    "       OPTIONS := { -V[ersion] | -s[tatistics] | -d[etails] |\n"
	    "                    -r[esolve] | -f[amily] { inet | inet6 | link } |\n"
	    "                    -4 | -6 | -0 | -n[umeric] }\n");
	exit(1);
}

static int
matches(const char *arg, const char *pattern)
{
	size_t len = strlen(arg);
	return strncmp(arg, pattern, len) == 0;
}

static int
parse_family(const char *arg)
{
	if (strcmp(arg, "inet") == 0 || strcmp(arg, "ipv4") == 0)
		return AF_INET;
	if (strcmp(arg, "inet6") == 0 || strcmp(arg, "ipv6") == 0)
		return AF_INET6;
	if (strcmp(arg, "link") == 0)
		return AF_LINK;
	return AF_UNSPEC;
}

int
main(int argc, char *argv[])
{
	int i;

	SetErrnoPtr(&errno, sizeof(errno));
	memset(&opts, 0, sizeof(opts));
	opts.family = AF_UNSPEC;

	/* Parse global options */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-')
			break;

		if (matches(argv[i]+1, "Version")) {
			printf("ip utility, v1.0 (AROS)\n");
			return 0;
		} else if (matches(argv[i]+1, "statistics") ||
		           strcmp(argv[i], "-s") == 0) {
			opts.stats++;
		} else if (matches(argv[i]+1, "details") ||
		           strcmp(argv[i], "-d") == 0) {
			opts.detail++;
		} else if (matches(argv[i]+1, "resolve") ||
		           strcmp(argv[i], "-r") == 0) {
			opts.resolve++;
		} else if (matches(argv[i]+1, "numeric") ||
		           strcmp(argv[i], "-n") == 0) {
			opts.numeric++;
		} else if (matches(argv[i]+1, "family") ||
		           strcmp(argv[i], "-f") == 0) {
			if (++i >= argc)
				usage();
			opts.family = parse_family(argv[i]);
			if (opts.family == AF_UNSPEC) {
				fprintf(stderr, "ip: unknown family \"%s\"\n",
				    argv[i]);
				exit(1);
			}
		} else if (strcmp(argv[i], "-4") == 0) {
			opts.family = AF_INET;
		} else if (strcmp(argv[i], "-6") == 0) {
			opts.family = AF_INET6;
		} else if (strcmp(argv[i], "-0") == 0) {
			opts.family = AF_LINK;
		} else if (strcmp(argv[i], "-h") == 0 ||
		           strcmp(argv[i], "-help") == 0 ||
		           strcmp(argv[i], "--help") == 0) {
			usage();
		} else {
			fprintf(stderr, "ip: unknown option \"%s\"\n",
			    argv[i]);
			usage();
		}
	}

	argc -= i;
	argv += i;

	if (argc < 1)
		usage();

	/* Dispatch to object handler */
	if (matches(argv[0], "link") || matches(argv[0], "l")) {
		return do_iplink(argc, argv);
	} else if (matches(argv[0], "address") || matches(argv[0], "addr") ||
	           strcmp(argv[0], "a") == 0) {
		return do_ipaddr(argc, argv);
	} else if (matches(argv[0], "route") || matches(argv[0], "r")) {
		return do_iproute(argc, argv);
	} else if (matches(argv[0], "neighbour") ||
	           matches(argv[0], "neighbor") ||
	           matches(argv[0], "neigh") ||
	           strcmp(argv[0], "n") == 0) {
		return do_ipneigh(argc, argv);
	} else if (matches(argv[0], "maddress") || matches(argv[0], "maddr")) {
		return do_ipmaddr(argc, argv);
	} else if (matches(argv[0], "mroute")) {
		return do_ipmroute(argc, argv);
	} else if (matches(argv[0], "rule") || matches(argv[0], "ru")) {
		return do_iprule(argc, argv);
	} else if (matches(argv[0], "netns")) {
		return do_ipnetns(argc, argv);
	} else if (matches(argv[0], "help")) {
		usage();
	} else {
		fprintf(stderr, "Object \"%s\" is unknown, try \"ip help\".\n",
		    argv[0]);
		return 1;
	}

	return 0;
}
