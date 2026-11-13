/* $Id$
 *
 *      linkntoa.c - link level address printing
 *
 *      Copyright (C) 1994 AmiTCP/IP Group, 
 *                       Network Solutions Development Inc.
 *                       All rights reserved.
 *
 *      Copyright (C) 1991 Regents of the University of California.
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

	/* the name, and a colon only before an address (as FreeBSD's);
	 * both within obuf */
	if (sdl->sdl_nlen) {
		i = sdl->sdl_nlen < (int)sizeof(obuf) - 1 ? sdl->sdl_nlen : (int)sizeof(obuf) - 1;
		bcopy(sdl->sdl_data, obuf, i);
		out += i;
		if (sdl->sdl_alen && out < obuf + sizeof(obuf) - 1)
			*out++ = ':';
	}
	while (in < inlim && out < obuf + sizeof(obuf) - 3) {
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

