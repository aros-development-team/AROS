/*	$KAME: if_nametoindex.c,v 1.0 2005/11/14 16:23:54 sonic Exp $	*/

/*-
 * Copyright (c) 1997, 2000
 *	Berkeley Software Design, Inc.  All rights reserved.
 * Copyright (c) 2005 - 2006
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
 *	BSDI Id: if_nametoindex.c,v 2.3 2000/04/17 22:38:05 dab Exp
 */

//#include <emul/emulregs.h>
#include <exec/libraries.h>

#include <conf.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_protos.h>
#include <api/amiga_api.h>
#include <api/ifaddrs.h>
#include <string.h>
#include <errno.h>

#include "miami_api.h"

/*
 * From RFC 2553:
 *
 * 4.1 Name-to-Index
 *
 *
 *    The first function maps an interface name into its corresponding
 *    index.
 *
 *       #include <net/if.h>
 *
 *       unsigned int  if_nametoindex(const char *ifname);
 *
 *    If the specified interface name does not exist, the return value is
 *    0, and errno is set to ENXIO.  If there was a system error (such as
 *    running out of memory), the return value is 0 and errno is set to the
 *    proper value (e.g., ENOMEM).
 */

unsigned int
__if_nametoindex(char *ifname, struct SocketBase *SocketBase)
{
	int s;
	struct ifnet *ifp;
	struct ifaddrs *ifaddrs, *ifa;
	unsigned int ni;

	ifp = ifunit(ifname);
	if (ifp)
		return ifp->if_index;

	if (getifaddrs(&ifaddrs, SocketBase) < 0)
		return(0);

	ni = 0;

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr &&
		    ifa->ifa_addr->sa_family == AF_LINK &&
		    strcmp(ifa->ifa_name, ifname) == 0) {
			ni = ((struct sockaddr_dl*)ifa->ifa_addr)->sdl_index;
			break;
		}
	}

	freeifaddrs(ifaddrs);
	if (!ni)
		writeErrnoValue(SocketBase, ENXIO);
	return(ni);
}

AROS_LH1(LONG, if_nametoindex,
         AROS_LHA(char *, ifname, A0),
         struct MiamiBase *, MiamiBase, 46, Miami
)
{
	AROS_LIBFUNC_INIT

	return __if_nametoindex(ifname, MiamiBase->_SocketBase);

	AROS_LIBFUNC_EXIT
}

