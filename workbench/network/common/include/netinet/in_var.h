/*
 * Copyright (c) 1985, 1986 Regents of the University of California.
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

#ifndef IN_VAR_H
#define IN_VAR_H

/*
 * Interface address, Internet version.  One of these structures
 * is allocated for each interface with an Internet address.
 * The ifaddr structure contains the protocol-independent part
 * of the structure and is assumed to be first.
 */
struct in_ifaddr {
	struct	ifaddr ia_ifa;		/* protocol-independent info */
#define	ia_ifp		ia_ifa.ifa_ifp
#define ia_flags	ia_ifa.ifa_flags
					/* ia_{,sub}net{,mask} in host order */
	u_long	ia_net;			/* network number of interface */
	u_long	ia_netmask;		/* mask of net part */
	u_long	ia_subnet;		/* subnet number, including net */
	u_long	ia_subnetmask;		/* mask of subnet part */
	struct	in_addr ia_netbroadcast; /* to recognize net broadcasts */
	struct	in_ifaddr *ia_next;	/* next in list of internet addresses */
	struct	sockaddr_in ia_addr;	/* reserve space for interface name */
	struct	sockaddr_in ia_dstaddr; /* reserve space for broadcast addr */
#define	ia_broadaddr	ia_dstaddr
	struct	sockaddr_in ia_sockmask; /* reserve space for general netmask */
};

struct	in_aliasreq {
	char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	sockaddr_in ifra_addr;
	struct	sockaddr_in ifra_broadaddr;
#define ifra_dstaddr ifra_broadaddr
	struct	sockaddr_in ifra_mask;
};
/*
 * Given a pointer to an in_ifaddr (ifaddr),
 * return a pointer to the addr as a sockaddr_in.
 */
#define	IA_SIN(ia) (&(((struct in_ifaddr *)(ia))->ia_addr))

/*
 * Interface address, IPv6 version.  One of these structures is
 * allocated for each interface with an IPv6 address.
 */
struct in6_ifaddr {
	struct	ifaddr ia_ifa;		/* protocol-independent info */
#define	ia6_ifp		ia_ifa.ifa_ifp
#define	ia6_flags	ia_ifa.ifa_flags
	struct	sockaddr_in6 ia_addr;	/* IPv6 address */
	struct	sockaddr_in6 ia_dstaddr; /* point-to-point dst addr */
	struct	sockaddr_in6 ia_prefixmask; /* prefix mask */
	int	ia6_plen;		/* prefix length (bits) */
	u_int32_t ia6_lifetime_vltime; /* valid lifetime */
	u_int32_t ia6_lifetime_pltime; /* preferred lifetime */
	u_int32_t ia6_ifaflags;		/* address flags */
	struct	in6_ifaddr *ia_next;	/* next in list */
};

/* ia6_ifaflags */
#define	IN6_IFF_ANYCAST		0x01	/* anycast address */
#define	IN6_IFF_TENTATIVE	0x02	/* tentative address (DAD) */
#define	IN6_IFF_DUPLICATED	0x04	/* DAD detected duplicate */
#define	IN6_IFF_DETACHED	0x08	/* may be detached from ifp */
#define	IN6_IFF_DEPRECATED	0x10	/* deprecated address */
#define	IN6_IFF_AUTOCONF	0x40	/* auto-configured address */
#define	IN6_IFF_TEMPORARY	0x80	/* temporary (privacy) address */
#define	IN6_IFF_PREFER_SOURCE	0x0100	/* preferred source address */

/* Alias request for adding/removing IPv6 interface addresses */
struct in6_addrlifetime {
	u_int32_t ia6t_expire;
	u_int32_t ia6t_preferred;
	u_int32_t ia6t_vltime;
	u_int32_t ia6t_pltime;
};

struct in6_aliasreq {
	char	ifra_name[IFNAMSIZ];
	struct	sockaddr_in6 ifra_addr;
	struct	sockaddr_in6 ifra_dstaddr;
	struct	sockaddr_in6 ifra_prefixmask;
	int	ifra_flags;
	struct in6_addrlifetime ifra_lifetime;
};

/* ifreq variant for IPv6 */
struct in6_ifreq {
	char	ifr_name[IFNAMSIZ];
	union {
		struct	sockaddr_in6 ifru_addr;
		struct	sockaddr_in6 ifru_dstaddr;
		int	ifru_flags;
		int	ifru_flags6;
		int	ifru_metric;
		struct in6_addrlifetime ifru_lifetime;
	} ifr_ifru;
#define ifr6_addr	ifr_ifru.ifru_addr
#define ifr6_dstaddr	ifr_ifru.ifru_dstaddr
};

#define	IA6_SIN6(ia6) (&(((struct in6_ifaddr *)(ia6))->ia_addr))

/*#ifdef	KERNEL*/
#if 1
extern struct	in_ifaddr *in_ifaddr;
extern struct	ifqueue	ipintrq;		/* ip packet input queue */
extern struct	in6_ifaddr *in6_ifaddr;		/* list of IPv6 addresses */
#endif

#endif /* !IN_VAR_H */
