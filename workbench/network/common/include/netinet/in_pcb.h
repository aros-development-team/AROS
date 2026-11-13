/*
 * Copyright (C) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2006 Pavel Fedin
 * Copyright (C) 2006-2026 The AROS Development Team.  All rights reserved.
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
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
 * $Id$
 */

#ifndef _NETINET_IN_PCB_H_
#define _NETINET_IN_PCB_H_

struct ip6_moptions;  /* forward declaration for IPv6 multicast options */

/*
 * Common structure pcb for internet protocol implementation.
 * Here are stored pointers to local and foreign host table
 * entries, local and foreign socket numbers, and pointers
 * up (to a socket structure) and down (to a protocol-specific)
 * control block.
 */
LIST_HEAD(inpcbhead, inpcb);

struct inetfamily;

/*
 * An address of whichever family the PCB is of. A PCB has one local and one
 * foreign address, in one place, and inp_af says of which family they are;
 * an address that is all zero is "any", in every family. (An IPv4 connection
 * accepted on an AF_INET6 socket is an IPv4 PCB: inp_af is AF_INET, and the
 * socket's family presents the addresses in IPv4-mapped form.)
 */
#define INP_ADDRMAX	16

union inp_addr {
	u_char		ia_bytes[INP_ADDRMAX];
	struct in_addr	ia_in;
	struct in6_addr	ia_in6;
};

#define inp_faddr	inp_fa.ia_in
#define inp_laddr	inp_la.ia_in
#define inp_faddr6	inp_fa.ia_in6
#define inp_laddr6	inp_la.ia_in6

struct inpcb {
	LIST_ENTRY(inpcb) inp_list;		/* list for all PCBs of this proto */
	LIST_ENTRY(inpcb) inp_hash;		/* hash list */
	struct	inpcbinfo *inp_pcbinfo;
	union	inp_addr inp_fa;	/* foreign address (inp_faddr, inp_faddr6) */
	u_short	inp_fport;		/* foreign port */
	union	inp_addr inp_la;	/* local address (inp_laddr, inp_laddr6) */
	u_short	inp_lport;		/* local port */
	u_char	inp_af;			/* AF_xxx of the addresses above */
	struct	socket *inp_socket;	/* back pointer to socket */
	caddr_t	inp_ppcb;		/* pointer to per-protocol pcb */
	struct	route inp_route;	/* placeholder for routing entry */
	int	inp_flags;		/* generic IP/datagram flags */
	struct	ip inp_ip;		/* header prototype; should have more */
	struct	mbuf *inp_options;	/* IP options */
	struct	ip_moptions *inp_moptions; /* IP multicast options */
	/* SO_BINDTODEVICE: datagrams leave by, and are accepted from, this
	 * interface only (NULL = not bound).  Must stay ahead of the INET6
	 * members: not every file that uses this structure is built with
	 * INET6 defined, and those that are not must still agree on where
	 * this field is. */
	struct	ifnet *inp_boundif;
	/* The family of the socket (netinet/inetfamily.h in the stack): what
	 * the transports ask instead of testing which family this is. Also
	 * ahead of the INET6 members, for the same reason. */
	const struct inetfamily *inp_family;
#if INET6
	struct	ip6_moptions *in6p_moptions; /* IPv6 multicast options */
	int	in6p_hops;		/* IPv6 unicast hop limit (-1=default) */
	int	in6p_v6only;		/* IPv6-only socket flag */
	int	in6p_tclass;		/* IPv6 traffic class */
	int	in6p_flags;		/* IPv6 per-socket flags (IN6P_*) */
#endif
};

struct inpcbinfo {
	struct inpcbhead *listhead;
	struct inpcbhead *hashbase;
	unsigned long hashsize;
	unsigned short lastport;
};

/* flags in inp_flags: */
#define	INP_RECVOPTS		0x01	/* receive incoming IP options */
#define	INP_RECVRETOPTS		0x02	/* receive IP options for reply */
#define	INP_RECVDSTADDR		0x04	/* receive IP dst address */
#define	INP_CONTROLOPTS		(INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|INP_RECVTOS|INP_RECVTTL)
#define	INP_HDRINCL		0x08	/* user supplies entire IP header */
#define	INP_DONTFRAG		0x10	/* IP_DONTFRAG: DF on datagrams */
#define	INP_RECVTOS		0x20	/* receive the TOS byte */
#define	INP_BOUNDGONE		0x40	/* the interface it was bound to went away */
#define	INP_RECVTTL		0x80	/* receive the TTL */

/*
 * What a raw socket of any family has besides its PCB (netinet/raw_ip.c);
 * inp_ppcb points to it. The family's own part (inf_rawprivlen bytes of
 * its struct inetfamily, e.g. IPv6's ICMPv6 filter) follows it.
 */
struct rawpcb {
	int	rp_proto;		/* protocol; 0 takes every protocol */
	int	rp_cksum;		/* offset, in the data, of a checksum the
					 * stack makes on output and verifies on
					 * input; -1: none */
	int	rp_flags;
#define	RPF_FILTER	0x1		/* the family is asked about each packet
					 * (inf_rawfilter) */
};
#define	intorawpcb(inp)	((struct rawpcb *)(inp)->inp_ppcb)
#define	rawpcbpriv(rp)	((void *)((struct rawpcb *)(rp) + 1))

#define	INPLOOKUP_WILDCARD	1
#define	INPLOOKUP_HASHED	2	/* only a PCB filed under exactly these
					 * addresses and ports: the list is not
					 * walked */
#define	INPLOOKUP_ATSPLNET	4	/* the caller is at splnet, as the input
					 * of a packet always is (sana_poll()):
					 * the level is not raised again, which
					 * would be four calls into exec */

#define	sotoinpcb(so)	((struct inpcb *)(so)->so_pcb)

#ifdef KERNEL
/*
 * Raw sockets, once for every family (netinet/raw_ip.c). A family's input
 * hands a packet to rip_deliver() with its own instance of the family
 * interface; everything else is reached through the protocol table.
 */
struct mbuf;
struct ifnet;
int	 rip_deliver __P((struct mbuf *, int, int, const struct inetfamily *,
	    int));
#define	RIP_CKSUMOK	0x1	/* rip_deliver(): the family's protocol has
				 * verified the checksum (rp_cksum) already */
void	 rip_ifgone __P((struct ifnet *));
#endif

#ifdef KERNEL
struct ifnet;
struct sockaddr;

/*
 * The PCB layer, once for every family (netinet/in_pcb.c). A PCB knows its
 * family (inp_family); where there is no PCB yet the family is an argument.
 * Addresses are passed as pointers to inf_addrlen bytes, even-aligned; a
 * NULL address is "any". Ports are in network byte order.
 */

/*
 * Addresses, for every family: inf_addrlen bytes (a multiple of 4, checked
 * when the family registers), all zero meaning "any". They are read a word
 * at a time, through a type that may alias whatever the caller's address
 * really is; it must be even-aligned (one in a PCB, or copied out of a
 * packet).
 *
 * These three are always expanded in place: for a 4-byte address each is
 * one or two instructions, where a call costs more code than the loop and,
 * on a 68000, some sixty cycles for every address compared on the way of
 * every packet.
 */
typedef u_int32_t __attribute__((__may_alias__)) inp_word_t;

/*
 * The same word where it lies in a packet: in a network header, which the
 * stack keeps even-aligned in its mbuf but not always aligned for a 32-bit
 * word on machines that want more than that.
 */
typedef u_int32_t __attribute__((__may_alias__, __aligned__(2))) inp_pktword_t;

#define	INP_ALWAYS_INLINE	static __inline __attribute__((__always_inline__))

INP_ALWAYS_INLINE int
inpcb_addrany(const void *a, u_int len)
{
	const inp_word_t *w = (const inp_word_t *)a;

	for (; len != 0; len -= sizeof(inp_word_t))
		if (*w++ != 0)
			return (0);
	return (1);
}

INP_ALWAYS_INLINE int
inpcb_addreq(const void *a, const void *b, u_int len)
{
	const inp_word_t *v = (const inp_word_t *)a, *w = (const inp_word_t *)b;

	for (; len != 0; len -= sizeof(inp_word_t))
		if (*v++ != *w++)
			return (0);
	return (1);
}

/*
 * Copy an address (len bytes: a family's inf_addrlen, never 0) between a
 * packet's network header and an even-aligned place of the stack's own -
 * a PCB, a local union inp_addr - in either direction.
 */
INP_ALWAYS_INLINE void
inpcb_addrcopy(void *dst, const void *src, u_int len)
{
	const inp_pktword_t *s = (const inp_pktword_t *)src;
	inp_pktword_t *d = (inp_pktword_t *)dst;

	do
		*d++ = *s++;
	while ((len -= sizeof(inp_pktword_t)) != 0);
}

/*
 * Does this PCB stand for every family? One whose family says so (an
 * AF_INET6 socket without IPV6_V6ONLY) and that has no address of its own:
 * a listener, or an unconnected datagram socket, on "any". Its addresses
 * are then all zero, which reads as "any" in every family. (A macro: the
 * family interface, netinet/inetfamily.h, is not known to this header.)
 */
#define	inpcb_anyfamily(inp) \
	(inpcb_addrany(&(inp)->inp_la, sizeof((inp)->inp_la)) && \
	 inpcb_addrany(&(inp)->inp_fa, sizeof((inp)->inp_fa)) && \
	 (*(inp)->inp_family->inf_pcbanyfamily)(inp))

struct inpcbhead *
	 inpcb_hash __P((struct inpcbinfo *, const struct inetfamily *,
	    const void *, u_int, u_int));
void	 inpcb_inshash __P((struct inpcb *));
void	 inpcb_rehash __P((struct inpcb *));
struct inpcb *
	 inpcb_lookup __P((struct inpcbinfo *, const struct inetfamily *,
	    const void *, u_int, const void *, u_int,
	    const struct ifnet *, int));
void	 inpcb_notify __P((struct inpcbhead *, const struct inetfamily *,
	    const void *, u_int, const void *, u_int, int,
	    void (*)(struct inpcb *, int)));
void	 inpcb_setconn __P((struct inpcb *, const void *, u_int,
	    const void *, u_int, struct ifnet *));
int	 inpcb_bind __P((struct inpcb *, struct mbuf *));
int	 inpcb_connect __P((struct inpcb *, struct mbuf *,
	    int (*)(struct inpcb *)));
void	 inpcb_disconnect __P((struct inpcb *));
void	 inpcb_setsockaddr __P((struct inpcb *, struct mbuf *));
void	 inpcb_setpeeraddr __P((struct inpcb *, struct mbuf *));

void	 in_losing __P((struct inpcb *));
int	 in_pcballoc __P((struct socket *, struct inpcbinfo *));
int	 in_pcbsetfamily __P((struct inpcb *, const struct inetfamily *));
void	 in_pcbdetach __P((struct inpcb *));
void	 in_pcbifgone __P((const struct inpcbhead *, const struct ifnet *));
void	 in_rtchange __P((struct inpcb *, int));

/*
 * IPv4's implementations of the family operations the PCB layer asks for
 * (netinet/in_family_pcb.c), for IPv4's instance of struct inetfamily.
 */
void	 in_pcbinit __P((struct inpcb *));
int	 in_pcbanyfamily __P((struct inpcb *));
int	 in_sockaddr_parse __P((struct mbuf *, const void **, u_short *,
	    struct ifnet **));
void	 in_mksockaddr __P((struct sockaddr *, const void *, u_int,
	    const struct ifnet *));
int	 in_addr_from __P((void *, const struct inetfamily *, const void *));
const struct ifnet *
	 in_pcbzone __P((struct inpcb *));
int	 in_ownaddr __P((const void *, struct ifnet *));
int	 in_selectsrc __P((struct inpcb *, const void *, struct ifnet *,
	    void *));
int	 in_pcbbindaddr __P((struct inpcb *, struct mbuf *, const void **,
	    u_short *, struct ifnet **));
int	 in_pcbladdr __P((struct inpcb *, struct mbuf *, void *,
	    struct ifnet **));

/* IPv6's (netinet6/in6_family_pcb.c), for IPv6's instance. */
void	 in6_pcbinit __P((struct inpcb *));
int	 in6_pcbanyfamily __P((struct inpcb *));
int	 in6_sockaddr_parse __P((struct mbuf *, const void **, u_short *,
	    struct ifnet **));
void	 in6_mksockaddr __P((struct sockaddr *, const void *, u_int,
	    const struct ifnet *));
int	 in6_addr_from __P((void *, const struct inetfamily *, const void *));
const struct ifnet *
	 in6_pcbzone __P((struct inpcb *));
int	 in6_ownaddr __P((const void *, struct ifnet *));
int	 in6_selectsrc __P((struct inpcb *, const void *, struct ifnet *,
	    void *));
int	 in6_pcbbindaddr __P((struct inpcb *, struct mbuf *, const void **,
	    u_short *, struct ifnet **));
int	 in6_pcbladdr __P((struct inpcb *, struct mbuf *, void *,
	    struct ifnet **));
#endif

#endif
