/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
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
 * UDP over IPv6 (udp6_usrreq, udp6_output, udp6_input demux)
 * Derived from netinet/udp_usrreq.c and KAME udp6_usrreq.c.
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
#include <sys/synch.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/udp.h>
#include <netinet6/in6_var.h>

#include <netinet/in_cksum_protos.h>

/* Explicit declarations (in_pcb_protos.h has stale signatures) */
extern int  in_pcballoc(struct socket *, struct inpcbinfo *);
extern void in_pcbdetach(struct inpcb *);
extern int  soreserve(struct socket *, u_long, u_long);
extern int  in6_control(struct socket *, int, caddr_t, struct ifnet *);
extern int  in6_pcbbind(struct inpcb *, struct mbuf *);
extern int  in6_pcbconnect(struct inpcb *, struct mbuf *);
extern void in6_pcbdisconnect(struct inpcb *);
extern void in6_setsockaddr(struct inpcb *, struct mbuf *);
extern void in6_setpeeraddr(struct inpcb *, struct mbuf *);
extern struct inpcb *in6_pcblookup(struct inpcbhead *,
                                   struct in6_addr *, u_int,
                                   struct in6_addr *, u_int);
extern struct in6_ifaddr *in6_ifawithifp(struct ifnet *, struct in6_addr *);
extern int  ip6_defhlim;

/* Share the UDP PCB list with IPv4 UDP so that port numbers are globally unique */
extern struct inpcbhead udb;
extern struct inpcbinfo udbinfo;
extern u_long udp_sendspace;
extern u_long udp_recvspace;

/* ------------------------------------------------------------------ *
 * IPv6 pseudo-header for UDP checksum (RFC 2460 §8.1).
 * ------------------------------------------------------------------ */
struct udp6_pseudo {
	struct in6_addr	src;		/* 16 bytes */
	struct in6_addr	dst;		/* 16 bytes */
	u_int32_t	len;		/* upper-layer packet length (network) */
	u_int8_t	zero[3];	/* must be zero */
	u_int8_t	nxt;		/* next header = IPPROTO_UDP */
};

/* ------------------------------------------------------------------ *
 * udp6_output - build and transmit a UDP/IPv6 datagram.
 * ------------------------------------------------------------------ */
static int
udp6_output(struct inpcb *inp, struct mbuf *m,
            struct mbuf *addr_m, struct mbuf *control)
{
	struct sockaddr_in6 *dst6;
	struct in6_addr laddr6, faddr6;
	u_short lport, fport;
	struct udphdr *uh;
	struct ip6_hdr *ip6;
	struct udp6_pseudo phdr;
	int len, udplen, error = 0;
	int s = 0;

	if (control)
		m_freem(control);

	len = m->m_pkthdr.len;
	udplen = len + sizeof(struct udphdr);

	/* ---- Resolve destination ---- */
	if (addr_m) {
		if (addr_m->m_len < (int)sizeof(struct sockaddr_in6)) {
			error = EINVAL;
			goto release;
		}
		dst6 = mtod(addr_m, struct sockaddr_in6 *);
		if (dst6->sin6_family != AF_INET6) {
			error = EAFNOSUPPORT;
			goto release;
		}
		if (!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
			error = EISCONN;
			goto release;
		}
		s = splnet();
		error = in6_pcbconnect(inp, addr_m);
		if (error) {
			splx(s);
			goto release;
		}
	} else {
		if (IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
			error = ENOTCONN;
			goto release;
		}
	}

	faddr6 = inp->inp_faddr6;
	fport  = inp->inp_fport;
	lport  = inp->inp_lport;

	/* ---- Resolve source address ---- */
	if (!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_laddr6)) {
		laddr6 = inp->inp_laddr6;
	} else {
		/* Auto-select from routing table */
		struct route_in6 ro;
		struct in6_ifaddr *ia;
		bzero(&ro, sizeof(ro));
		ro.ro_dst.sin6_family = AF_INET6;
		ro.ro_dst.sin6_len    = sizeof(ro.ro_dst);
		ro.ro_dst.sin6_addr   = faddr6;
		rtalloc((struct route *)&ro);
		bzero(&laddr6, sizeof(laddr6));
		if (ro.ro_rt != NULL) {
			ia = in6_ifawithifp(ro.ro_rt->rt_ifp, &faddr6);
			if (ia)
				laddr6 = ia->ia_addr.sin6_addr;
			RTFREE(ro.ro_rt);
		}
	}

	/* ---- Prepend UDP header ---- */
	M_PREPEND(m, sizeof(struct udphdr), M_DONTWAIT);
	if (m == NULL) {
		error = ENOBUFS;
		if (addr_m) { in6_pcbdisconnect(inp); splx(s); }
		return error;
	}
	uh = mtod(m, struct udphdr *);
	uh->uh_sport = lport;
	uh->uh_dport = fport;
	uh->uh_ulen  = htons((u_short)udplen);
	uh->uh_sum   = 0;

	/* ---- Compute UDP checksum using IPv6 pseudo-header ---- */
	bzero(&phdr, sizeof(phdr));
	phdr.src = laddr6;
	phdr.dst = faddr6;
	phdr.len = htonl((u_int32_t)udplen);
	phdr.nxt = IPPROTO_UDP;

	/* Temporarily prepend pseudo-header to mbuf chain for in_cksum() */
	M_PREPEND(m, sizeof(phdr), M_DONTWAIT);
	if (m == NULL) {
		error = ENOBUFS;
		if (addr_m) { in6_pcbdisconnect(inp); splx(s); }
		return error;
	}
	bcopy(&phdr, mtod(m, void *), sizeof(phdr));
	uh = (struct udphdr *)(mtod(m, u_int8_t *) + sizeof(phdr));
	uh->uh_sum = in_cksum(m, sizeof(phdr) + udplen);
	if (uh->uh_sum == 0)
		uh->uh_sum = 0xffff; /* RFC 768: replace zero checksum with 0xffff */
	m_adj(m, sizeof(phdr));  /* remove pseudo-header */

	/* ---- Prepend IPv6 header ---- */
	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (m == NULL) {
		error = ENOBUFS;
		if (addr_m) { in6_pcbdisconnect(inp); splx(s); }
		return error;
	}
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_vfc  = IPV6_VERSION;
	ip6->ip6_flow = 0;
	ip6->ip6_plen = htons((u_short)udplen);
	ip6->ip6_nxt  = IPPROTO_UDP;
	ip6->ip6_hlim = (inp->in6p_hops >= 0) ? (u_int8_t)inp->in6p_hops
	                                       : (u_int8_t)ip6_defhlim;
	ip6->ip6_src  = laddr6;
	ip6->ip6_dst  = faddr6;

	error = ip6_output(m, NULL, NULL, 0, inp->in6p_moptions, NULL, inp);

	if (addr_m) {
		in6_pcbdisconnect(inp);
		splx(s);
	}
	return error;

release:
	m_freem(m);
	return error;
}

/* ------------------------------------------------------------------ *
 * udp6_input - demultiplex an incoming UDP/IPv6 datagram to a socket.
 *
 * Called from ip6_input when the next-header is IPPROTO_UDP.
 * 'm' points to the IPv6 header; 'off' is the offset to the UDP header.
 * ------------------------------------------------------------------ */
void
udp6_input(void *args, ...)
{
	struct mbuf *m = args;
	int off;
	va_list va;

	va_start(va, args);
	off = va_arg(va, int);
	va_end(va);

	struct ip6_hdr *ip6;
	struct udphdr *uh;
	struct inpcb *inp;
	struct sockaddr_in6 src6, dst6;
	int len;

	if (m->m_len < off + (int)sizeof(struct udphdr)) {
		if ((m = m_pullup(m, off + sizeof(struct udphdr))) == NULL)
			return;
	}

	ip6 = mtod(m, struct ip6_hdr *);
	uh  = (struct udphdr *)((u_int8_t *)ip6 + off);
	len = ntohs(uh->uh_ulen);

	/* Build sockaddr_in6 for source and destination */
	bzero(&src6, sizeof(src6));
	src6.sin6_family = AF_INET6;
	src6.sin6_len    = sizeof(src6);
	src6.sin6_addr   = ip6->ip6_src;
	src6.sin6_port   = uh->uh_sport;

	bzero(&dst6, sizeof(dst6));
	dst6.sin6_family = AF_INET6;
	dst6.sin6_len    = sizeof(dst6);
	dst6.sin6_addr   = ip6->ip6_dst;
	dst6.sin6_port   = uh->uh_dport;

	/* Find matching socket */
	inp = in6_pcblookup(&udb, &src6.sin6_addr, uh->uh_sport,
	                    &dst6.sin6_addr, uh->uh_dport);
	if (inp == NULL) {
		/* No match: drop */
		m_freem(m);
		return;
	}

	/* Strip IPv6 header + extension headers, leaving UDP header + data */
	m_adj(m, off);

	/* Deliver to socket */
	{
		struct mbuf *opts = NULL;
		if (sbappendaddr(&inp->inp_socket->so_rcv,
		                 (struct sockaddr *)&src6,
		                 m, opts) == 0) {
			m_freem(m);
			return;
		}
		sorwakeup(inp->inp_socket);
	}
}

/* ------------------------------------------------------------------ *
 * udp6_usrreq - user request handler for UDP/IPv6 sockets.
 * ------------------------------------------------------------------ */
int
udp6_usrreq(struct socket *so, int req, struct mbuf *m,
            struct mbuf *addr, struct mbuf *control)
{
	struct inpcb *inp = sotoinpcb(so);
	int error = 0;
	int s;

	/* Socket-level ioctl: delegate to IPv6 interface control */
	if (req == PRU_CONTROL)
		return in6_control(so, (int)(long)m, (caddr_t)addr,
		                   (struct ifnet *)control);

	if (inp == NULL && req != PRU_ATTACH) {
		error = EINVAL;
		goto release;
	}

	switch (req) {

	case PRU_ATTACH:
		if (inp != NULL) {
			error = EINVAL;
			break;
		}
		s = splnet();
		error = in_pcballoc(so, &udbinfo);
		splx(s);
		if (error)
			break;
		error = soreserve(so, udp_sendspace, udp_recvspace);
		if (error)
			break;
		inp = sotoinpcb(so);
		inp->in6p_hops = -1; /* use system default hop limit */
		break;

	case PRU_DETACH:
		s = splnet();
		in_pcbdetach(inp);
		splx(s);
		break;

	case PRU_BIND:
		s = splnet();
		error = in6_pcbbind(inp, addr);
		splx(s);
		break;

	case PRU_LISTEN:
		error = EOPNOTSUPP;
		break;

	case PRU_CONNECT:
		if (!IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
			error = EISCONN;
			break;
		}
		s = splnet();
		error = in6_pcbconnect(inp, addr);
		splx(s);
		if (error == 0)
			soisconnected(so);
		break;

	case PRU_CONNECT2:
		error = EOPNOTSUPP;
		break;

	case PRU_DISCONNECT:
		if (IN6_IS_ADDR_UNSPECIFIED(&inp->inp_faddr6)) {
			error = ENOTCONN;
			break;
		}
		s = splnet();
		in6_pcbdisconnect(inp);
		splx(s);
		so->so_state &= ~SS_ISCONNECTED;
		break;

	case PRU_SHUTDOWN:
		socantsendmore(so);
		break;

	case PRU_SEND:
		return udp6_output(inp, m, addr, control);

	case PRU_ABORT:
		soisdisconnected(so);
		s = splnet();
		in_pcbdetach(inp);
		splx(s);
		break;

	case PRU_SOCKADDR:
		in6_setsockaddr(inp, addr);
		break;

	case PRU_PEERADDR:
		in6_setpeeraddr(inp, addr);
		break;

	case PRU_SENSE:
		return 0;

	case PRU_RCVD:
	case PRU_RCVOOB:
		return EOPNOTSUPP;

	case PRU_SENDOOB:
	case PRU_FASTTIMO:
	case PRU_SLOWTIMO:
	case PRU_PROTORCV:
	case PRU_PROTOSEND:
		error = EOPNOTSUPP;
		break;

	default:
		panic("udp6_usrreq: unknown req %d", req);
	}

release:
	if (control)
		m_freem(control);
	if (m)
		m_freem(m);
	return error;
}

#endif /* INET6 */
