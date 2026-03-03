/*
 * Copyright (C) 2026 The AROS Development Team. All rights reserved.
 * BSD 3-Clause License (see ip.c for full text).
 *
 * ip rule  - policy routing rules (RPDB)
 * ip netns - network namespaces
 *
 * These features are not applicable to AROS, which has a single
 * routing table and no kernel namespace support. Stubs are provided
 * for command-line compatibility.
 */

#define INET6 1

#include <stdio.h>
#include <string.h>

#include "ip.h"

/*
 * ip rule show - AROS uses a single routing table with no policy
 * routing database. Show the implicit default rule.
 */
int
do_iprule(int argc, char **argv)
{
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "show") == 0)) {
		printf("0:\tfrom all lookup local\n");
		printf("32766:\tfrom all lookup main\n");
		printf("32767:\tfrom all lookup default\n");
		return 0;
	}

	if (argc >= 1 && strcmp(argv[0], "help") == 0) {
		fprintf(stderr,
		    "Usage: ip rule show\n"
		    "       ip rule help\n"
		    "Note: AROS uses a single routing table; "
		    "policy rules are not configurable.\n");
		return 0;
	}

	fprintf(stderr,
	    "Command \"%s\" is unknown, try \"ip rule help\".\n",
	    argv[0]);
	return 1;
}

/*
 * ip netns - AROS does not support network namespaces.
 */
int
do_ipnetns(int argc, char **argv)
{
	argc--;
	argv++;

	if (argc == 0 || (argc >= 1 && strcmp(argv[0], "list") == 0)) {
		/* No namespaces — empty output (like Linux with none) */
		return 0;
	}

	if (argc >= 1 && strcmp(argv[0], "help") == 0) {
		fprintf(stderr,
		    "Usage: ip netns list\n"
		    "       ip netns help\n"
		    "Note: AROS does not support network namespaces.\n");
		return 0;
	}

	fprintf(stderr,
	    "Network namespaces are not supported on this platform.\n");
	return 1;
}
