/*	$KAME: if_indextoname.c,v 1.0 2005/11/14 16:44:30 sonic Exp $	*/

/*-
 * Copyright (c) 1997, 2000
 *	Berkeley Software Design, Inc.  All rights reserved.
 * Copyright (c) 2005
 *	Pavel Fedin
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY Berkeley Software Design, Inc. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Berkeley Software Design, Inc. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI Id: if_indextoname.c,v 2.3 2000/04/17 22:38:05 dab Exp
 */

//#include <emul/emulregs.h>
#include <exec/libraries.h>

#include <conf.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <api/amiga_api.h>
#include <api/ifaddrs.h>
#include <string.h>
#include <errno.h>

#include "miami_api.h"

/*
 * From RFC 2533:
 *
 * The second function maps an interface index into its corresponding
 * name.
 *
 *    #include <net/if.h>
 *
 *    char  *if_indextoname(unsigned int ifindex, char *ifname);
 *
 * The ifname argument must point to a buffer of at least IF_NAMESIZE
 * bytes into which the interface name corresponding to the specified
 * index is returned.  (IF_NAMESIZE is also defined in <net/if.h> and
 * its value includes a terminating null byte at the end of the
 * interface name.) This pointer is also the return value of the
 * function.  If there is no interface corresponding to the specified
 * index, NULL is returned, and errno is set to ENXIO, if there was a
 * system error (such as running out of memory), if_indextoname returns
 * NULL and errno would be set to the proper value (e.g., ENOMEM).
 */

AROS_LH2(char *, if_indextoname,
         AROS_LHA(LONG, ifindex, D0),
         AROS_LHA(char *, ifname, A0),
         struct MiamiBase *, MiamiBase, 47, Miami
)
{
	AROS_LIBFUNC_INIT

	struct ifaddrs *ifaddrs, *ifa;
	int error = 0;

	if (getifaddrs(&ifaddrs, MiamiBase->_SocketBase) < 0)
		return(NULL);	/* getifaddrs properly set errno */

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
		    ifa->ifa_addr->sa_family == AF_LINK &&
		    ifindex == ((struct sockaddr_dl*)ifa->ifa_addr)->sdl_index)
			break;
	}

	if (ifa == NULL) {
		error = ENXIO;
		ifname = NULL;
	}
	else
		strncpy(ifname, ifa->ifa_name, IFNAMSIZ);

	freeifaddrs(ifaddrs);

	writeErrnoValue(MiamiBase->_SocketBase, error);
	return(ifname);

	AROS_LIBFUNC_EXIT
}

