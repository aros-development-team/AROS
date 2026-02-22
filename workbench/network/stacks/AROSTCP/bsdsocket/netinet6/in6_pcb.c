/*
 * Copyright (c) 1982, 1986, 1991, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
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
 *
 * IPv6 PCB management, derived from netinet/in_pcb.c
 */

#include <conf.h>
#if INET6

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/synch.h>

#include <net/route.h>
#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>

/* ------------------------------------------------------------------ *
 * in6_pcbbind - bind an IPv6 PCB to a local address and/or port.
 *
 * If nam is NULL, a wildcard bind with an ephemeral port is performed.
 * Otherwise nam must point to a sockaddr_in6.
 * ------------------------------------------------------------------ */
int
in6_pcbbind(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6;
	u_short lport = 0;

	if (inp->inp_lport || !IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6))
		return EINVAL;

	if (nam == NULL) {
		/* wildcard bind: allocate an ephemeral port */
		lport = htons(++(inp->inp_pcbinfo->lastport));
		if (lport == 0)
			lport = htons(++(inp->inp_pcbinfo->lastport));
		inp->inp_lport = lport;
		/* leave inp_laddr6 as all-zeros (wildcard) */
		return 0;
	}

	if (nam->m_len < (int)sizeof(struct sockaddr_in6))
		return EINVAL;

	sin6 = mtod(nam, struct sockaddr_in6 *);
	if (sin6->sin6_family != AF_INET6)
		return EAFNOSUPPORT;

	inp->inp_laddr6 = sin6->sin6_addr;
	inp->inp_lport  = sin6->sin6_port; /* already in network byte order */

	return 0;
}

/* ------------------------------------------------------------------ *
 * in6_pcbconnect - connect an IPv6 PCB to a remote address and port.
 * ------------------------------------------------------------------ */
int
in6_pcbconnect(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6;

	if (nam == NULL || nam->m_len < (int)sizeof(struct sockaddr_in6))
		return EINVAL;

	sin6 = mtod(nam, struct sockaddr_in6 *);
	if (sin6->sin6_family != AF_INET6)
		return EAFNOSUPPORT;

	if (IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr))
		return EADDRNOTAVAIL;

	/* Bind a local port if not already bound */
	if (inp->inp_lport == 0) {
		int error = in6_pcbbind(inp, NULL);
		if (error)
			return error;
	}

	inp->inp_faddr6 = sin6->sin6_addr;
	inp->inp_fport  = sin6->sin6_port;

	return 0;
}

/* ------------------------------------------------------------------ *
 * in6_pcbdisconnect - disconnect an IPv6 PCB (clear foreign address).
 * ------------------------------------------------------------------ */
void
in6_pcbdisconnect(struct inpcb *inp)
{
	bzero(&inp->inp_faddr6, sizeof(inp->inp_faddr6));
	inp->inp_fport = 0;
	if (inp->inp_socket->so_state & SS_NOFDREF)
		in_pcbdetach(inp);
}

/* ------------------------------------------------------------------ *
 * in6_setsockaddr - fill nam with the local IPv6 address/port.
 * ------------------------------------------------------------------ */
void
in6_setsockaddr(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(struct sockaddr_in6);
	sin6 = mtod(nam, struct sockaddr_in6 *);
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len    = sizeof(*sin6);
	sin6->sin6_port   = inp->inp_lport;
	sin6->sin6_addr   = inp->inp_laddr6;
}

/* ------------------------------------------------------------------ *
 * in6_setpeeraddr - fill nam with the foreign IPv6 address/port.
 * ------------------------------------------------------------------ */
void
in6_setpeeraddr(struct inpcb *inp, struct mbuf *nam)
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(struct sockaddr_in6);
	sin6 = mtod(nam, struct sockaddr_in6 *);
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len    = sizeof(*sin6);
	sin6->sin6_port   = inp->inp_fport;
	sin6->sin6_addr   = inp->inp_faddr6;
}

/* ------------------------------------------------------------------ *
 * in6_pcblookup - find a matching IPv6 PCB.
 *
 * Searches the given PCB list for an entry matching the four-tuple
 * (foreign addr, foreign port, local addr, local port).  Wildcard
 * entries (all-zeros local or foreign address) are lower priority.
 * ------------------------------------------------------------------ */
struct inpcb *
in6_pcblookup(struct inpcbhead *head,
              struct in6_addr *faddr, u_int fport_arg,
              struct in6_addr *laddr, u_int lport_arg)
{
	struct inpcb *inp, *match = NULL;
	int matchwild = 3;
	u_short fport = fport_arg, lport = lport_arg;

	LIST_FOREACH(inp, head, inp_list) {
		if (inp->inp_lport != lport)
			continue;

		int wildcard = 0;

		/* Check local address */
		if (!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
			if (!IN6_ARE_ADDR_EQUAL(&inp->inp_laddr6, laddr))
				continue;
		} else {
			wildcard++;
		}

		/* Check foreign address */
		if (!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
			if (inp->inp_fport != fport ||
			    !IN6_ARE_ADDR_EQUAL(&inp->inp_faddr6, faddr))
				continue;
		} else {
			if (fport != 0)
				wildcard++;
		}

		if (wildcard < matchwild) {
			match = inp;
			matchwild = wildcard;
			if (matchwild == 0)
				break;
		}
	}

	return match;
}

#endif /* INET6 */
