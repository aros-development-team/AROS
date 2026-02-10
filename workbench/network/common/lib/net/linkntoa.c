/* $Id$
 *
 *      linkntoa.c - link level address printing
 *
 *      Copyright © 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *
 *      Copyright © 1991 Regents of the University of California.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <string.h>

static char hexlist[] = "0123456789abcdef";

char *
link_ntoa(sdl)
	register const struct sockaddr_dl *sdl;
{
	static char obuf[64];
	register char *out = obuf; 
	register int i;
	register u_char *in = (u_char *)LLADDR(sdl);
	u_char *inlim = in + sdl->sdl_alen;
	int firsttime = 1;

	if (sdl->sdl_nlen) {
		bcopy(sdl->sdl_data, obuf, sdl->sdl_nlen);
		out += sdl->sdl_nlen;
		*out++ = ':';
	}
	while (in < inlim) {
		if (firsttime) firsttime = 0; else *out++ = '.';
		i = *in++;
		if (i > 0xf) {
			out[1] = hexlist[i & 0xf];
			i >>= 4;
			out[0] = hexlist[i];
			out += 2;
		} else
			*out++ = hexlist[i];
	}
	*out = 0;
	return(obuf);
}

