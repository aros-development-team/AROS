/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 Pavel Fedin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * Copyright (c) 1983, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <conf.h>

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socketvar.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/malloc.h>
#include <sys/synch.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <netinet/in.h>

#include <kern/amiga_includes.h>

#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/allocdatabuffer.h>

#include <ctype.h>

#include <proto/bsdsocket.h>

/*
 * Functions which are defined in link library in unix systems
 */

/* from inet_ntoa.c */
/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
char * __Inet_NtoA(ULONG s_addr, struct SocketBase *libPtr)
{
  NTOHL(s_addr);

  CHECK_TASK2();
  sprintf(libPtr->inet_ntoa,
	  "%ld.%ld.%ld.%ld", 
	  (long)(s_addr>>24) & 0xff, 
	  (long)(s_addr>>16) & 0xff, 
	  (long)(s_addr>>8) & 0xff, 
	  (long)s_addr & 0xff);
  return ((char *)libPtr->inet_ntoa);
}
AROS_LH1(char *, Inet_NtoA,
   AROS_LHA(ULONG, s_addr, D0),
   struct SocketBase *, libPtr, 29, UL)
{
  AROS_LIBFUNC_INIT
  D(__log(LOG_DEBUG,"Inet_NtoA(0x%08lx) called",s_addr);)
  return __Inet_NtoA(s_addr, libPtr);
  AROS_LIBFUNC_EXIT
}

/* from inet_addr.c */
/* 
 * Check whether "cp" is a valid ascii representation
 * of an Internet address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 * This replaces inet_addr, the return value from which
 * cannot distinguish between failure and a local broadcast address.
 */

LONG __inet_aton(CONST_STRPTR cp,  struct in_addr * addr)
{
	register u_long val, base, n;
	register char c;
	u_long parts[4], *pp = parts;
	
	for (;;) {
		/*
		 * Collect number up to ``.''.
		 * Values are specified as for C:
		 * 0x=hex, 0=octal, other=decimal.
		 */
		val = 0; base = 10;
		if (*cp == '0') {
			if (*++cp == 'x' || *cp == 'X')
				base = 16, cp++;
			else
				base = 8;
		}
		while ((c = *cp) != '\0') {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				cp++;
				continue;
			}
			if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) + 
					(c + 10 - (islower(c) ? 'a' : 'A'));
				cp++;
				continue;
			}
			break;
		}
		if (*cp == '.') {
			/*
			 * Internet format:
			 *	a.b.c.d
			 *	a.b.c	(with c treated as 16-bits)
			 *	a.b	(with b treated as 24 bits)
			 */
			if (pp >= parts + 3 || val > 0xff)
				return (0);
			*pp++ = val, cp++;
		} else
			break;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && (!isascii(*cp) || !isspace(*cp)))
		return (0);
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts + 1;
	switch (n) {

	case 1:				/* a -- 32 bits */
		break;

	case 2:				/* a.b -- 8.24 bits */
		if (val > 0xffffff)
			return (0);
		val |= parts[0] << 24;
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		if (val > 0xffff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		if (val > 0xff)
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}
	if (addr)
		addr->s_addr = htonl(val);
	return (1);
}

#if defined(__CONFIG_ROADSHOW__)
AROS_LH2(LONG, inet_aton,
   AROS_LHA(STRPTR, cp, A0),
   AROS_LHA(struct in_addr *, addr, A1),
   struct SocketBase *, libPtr, 99, UL)
{
	AROS_LIBFUNC_INIT

	return __inet_aton(cp,  addr);

	AROS_LIBFUNC_EXIT

}
#endif

/*
 * Ascii internet address interpretation routine.
 * The value returned is in network order.
 */

/*ULONG SAVEDS inet_addr(
   REG(a0, const char *cp),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH1(ULONG, inet_addr,
   AROS_LHA(const char *, cp, A0),
   struct SocketBase *, libPtr, 30, UL)
{
        AROS_LIBFUNC_INIT
	struct in_addr val;

	if (__inet_aton(cp, &val))
		return (val.s_addr);
	return (INADDR_NONE);
        AROS_LIBFUNC_EXIT
}

/* from inet_lnaof.c */
/*
 * Return the local network address portion of an
 * internet address; handles class a/b/c network
 * number formats.
 */
/*ULONG SAVEDS Inet_LnaOf(
   REG(d0, ULONG s_addr),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH1(ULONG, Inet_LnaOf,
   AROS_LHA(ULONG, s_addr, D0),
   struct SocketBase *, libPtr, 31, UL)
{
        AROS_LIBFUNC_INIT
	NTOHL(s_addr);

	if (IN_CLASSA(s_addr))
		return ((s_addr)&IN_CLASSA_HOST);
	else if (IN_CLASSB(s_addr))
		return ((s_addr)&IN_CLASSB_HOST);
	else
		return ((s_addr)&IN_CLASSC_HOST);
        AROS_LIBFUNC_EXIT
}

/* from inet_netof.c */
/*
 * Return the network number from an internet
 * address; handles class a/b/c network #'s.
 */
/*ULONG SAVEDS Inet_NetOf(
   REG(d0, ULONG s_addr),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH1(ULONG, Inet_NetOf,
   AROS_LHA(ULONG, s_addr, D0),
   struct SocketBase *, libPtr, 32, UL)
{
        AROS_LIBFUNC_INIT
	NTOHL(s_addr);

	if (IN_CLASSA(s_addr))
		return (((s_addr)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else if (IN_CLASSB(s_addr))
		return (((s_addr)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return (((s_addr)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
        AROS_LIBFUNC_EXIT
}

/* from inet_makeaddr.c */
/*
 * Formulate an Internet address from network + host.  Used in
 * building addresses stored in the ifnet structure.
 */
/*ULONG SAVEDS Inet_MakeAddr(
   REG(d0, ULONG net),
   REG(d1, ULONG host),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH2(ULONG, Inet_MakeAddr,
   AROS_LHA(ULONG, net, D0),
   AROS_LHA(ULONG, host, D1),
   struct SocketBase *, libPtr, 33, UL)
{
	AROS_LIBFUNC_INIT
	u_long addr;

	if (net < 128)
		addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);
	else if (net < 65536)
		addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);
	else if (net < 16777216L)
		addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);
	else
		addr = net | host;

	return htonl(addr);
	AROS_LIBFUNC_EXIT
}

/* from inet_network.c */
/*
 * Internet network address interpretation routine.
 * The library routines call this routine to interpret
 * network numbers.
 */
/*ULONG SAVEDS inet_network(
   REG(a0, const char *cp),
   REG(a6, struct SocketBase *libPtr))*/
AROS_LH1(ULONG, inet_network,
   AROS_LHA(const char *, cp, A0),
   struct SocketBase *, libPtr, 34, UL)
{
	AROS_LIBFUNC_INIT
	register u_long val, base, n;
	register char c;
	u_long parts[4], *pp = parts;
	register int i;

again:
	val = 0; base = 10;
	if (*cp == '0')
		base = 8, cp++;
	if (*cp == 'x' || *cp == 'X')
		base = 16, cp++;
	while (c = *cp) {
		if (isdigit(c)) {
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		if (pp >= parts + 4)
			return (INADDR_NONE);
		*pp++ = val, cp++;
		goto again;
	}
	if (*cp && !isspace(*cp))
		return (INADDR_NONE);
	*pp++ = val;
	n = pp - parts;
	if (n > 4)
		return (INADDR_NONE);
	for (val = 0, i = 0; i < n; i++) {
		val <<= 8;
		val |= parts[i] & 0xff;
	}
	return (val);
	AROS_LIBFUNC_EXIT
}

