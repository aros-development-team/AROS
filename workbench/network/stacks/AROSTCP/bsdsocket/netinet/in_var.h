/*
 * Copyright (C) 1985, 1986 Regents of the University of California.
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
 *
 *	@(#)in_var.h	7.6 (Berkeley) 6/28/90
 */

#ifndef AROSTCP_NETINET_IN_VAR_H
#define AROSTCP_NETINET_IN_VAR_H

/*
 * Kernel-private view of <netinet/in_var.h>.  Stack sources include this
 * local header (via -iquote) rather than the public one so that they pick
 * up both the public interface-address definitions and the kernel-internal
 * additions below.  The public definitions are pulled in unchanged from the
 * shared header in common/include.
 */
#include <netinet/in_var.h>

#ifdef ENABLE_MULTICAST
/*
 * IPv4 multicast host membership.  One in_multi is allocated for every
 * (group, interface) pair the host has joined.  The list is walked by the
 * output path (loopback / IGMP report suppression) and by igmp.c when
 * servicing membership queries and report timers.  It mirrors the IPv6
 * in6_multi list in netinet6/in6_var.h.  These types are private to the
 * stack; applications use struct ip_mreq and the IP_*_MEMBERSHIP socket
 * options declared in the public <netinet/in.h>.
 */
struct in_multi {
	struct	in_addr inm_addr;	/* IP multicast address (network order) */
	struct	ifnet  *inm_ifp;	/* back pointer to ifnet */
	u_int	inm_refcount;		/* reference count */
	struct	in_multi *inm_next;	/* linked list */
	u_int	inm_state;		/* IGMP membership state */
	u_int	inm_timer;		/* IGMP report delay timer (fast ticks) */
};

extern struct in_multi *in_multihead;	/* global IPv4 multicast list */

/*
 * Multicast options attached to an inpcb (IP_MULTICAST_* / IP_*_MEMBERSHIP).
 */
#define	IP_MAX_MEMBERSHIPS	20	/* max memberships per socket */
#define	IP_DEFAULT_MULTICAST_TTL  1	/* normally limit to one subnet */
#define	IP_DEFAULT_MULTICAST_LOOP 1	/* normally hear sends if a member */

struct ip_moptions {
	struct	ifnet *imo_multicast_ifp; /* ifp for outgoing multicasts */
	u_char	imo_multicast_ttl;	/* TTL for outgoing multicasts */
	u_char	imo_multicast_loop;	/* 1 => hear sends if a member */
	u_short	imo_num_memberships;	/* no. memberships this socket */
	struct	in_multi *imo_membership[IP_MAX_MEMBERSHIPS];
};

/*
 * Look up the in_multi record for a given (group, interface) pair.
 */
#define	IN_LOOKUP_MULTI(addr, ifp, inm)					\
{									\
	register struct in_multi *_inm;					\
	for(_inm = in_multihead; _inm != NULL; _inm = _inm->inm_next)	\
		if(_inm->inm_addr.s_addr == (addr).s_addr &&		\
		   _inm->inm_ifp == (ifp))				\
			break;						\
	(inm) = _inm;							\
}

/*
 * Given a local IP address, return the interface it is configured on.
 */
#define	INADDR_TO_IFP(addr, ifp)					\
{									\
	register struct in_ifaddr *_ia;					\
	for(_ia = in_ifaddr;						\
	    _ia != NULL && IA_SIN(_ia)->sin_addr.s_addr != (addr).s_addr; \
	    _ia = _ia->ia_next)						\
		;							\
	(ifp) = (_ia == NULL) ? NULL : _ia->ia_ifp;			\
}

/*
 * Given an interface, return the first in_ifaddr configured on it.
 */
#define	IFP_TO_IA(ifp, ia)						\
{									\
	register struct in_ifaddr *_ia;					\
	for(_ia = in_ifaddr;						\
	    _ia != NULL && _ia->ia_ifp != (ifp);			\
	    _ia = _ia->ia_next)						\
		;							\
	(ia) = _ia;							\
}

struct in_multi *in_addmulti(struct in_addr *, struct ifnet *);
void in_delmulti(struct in_multi *);
#endif /* ENABLE_MULTICAST */

#endif /* !AROSTCP_NETINET_IN_VAR_H */
