/*
 * Copyright (C) 2005 Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this file; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 *
 */

/*
 * Copyright (c) 1982, 1986, 1990 Regents of the University of California.
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
 *	@(#)in.h	7.11 (Berkeley) 4/20/91
 */

#ifndef IN_H
#define IN_H

#include <sys/types.h>
#include <stdint.h>
#include <endian.h>

/*
 * Endianness conversion macros
 */
#define FlipWord(A) \
   ({ \
      uint16_t _FlipWord_A = (A); \
      _FlipWord_A = (_FlipWord_A << 8) | (_FlipWord_A >> 8); \
   })

#define FlipLong(A) \
   ({ \
      uint32_t _FlipLong_A = (A); \
      _FlipLong_A = \
         (FlipWord(_FlipLong_A) << 16) | FlipWord(_FlipLong_A >> 16); \
   })

#if _BYTE_ORDER == _BIG_ENDIAN

#define BEWord(A) \
   (A)

#define BELong(A) \
   (A)

#define LEWord(A) \
   FlipWord(A)

#define LELong(A) \
   FlipLong(A)

#elif _BYTE_ORDER == _LITTLE_ENDIAN

#define BEWord(A) \
   FlipWord(A)

#define BELong(A) \
   FlipLong(A)

#define LEWord(A) \
   (A)

#define LELong(A) \
   (A)

#else

#error <netinet/in.h> - Byte order for this architecture is unsupported!

#endif

#define MakeBEWord(A) \
   BEWord(A)

#define MakeBELong(A) \
   BELong(A)

#define MakeLEWord(A) \
   LEWord(A)

#define MakeLELong(A) \
   LELong(A)

/*
 * Macros for network/external number representation conversion.
 */
 
#define  INET_ADDRSTRLEN 16
 
#ifndef ntohl
#define	ntohl(x)	BELong(x)
#define	ntohs(x)	BEWord(x)
#define	htonl(x)	MakeBELong(x)
#define	htons(x)	MakeBEWord(x)

#define	NTOHL(x)	((x) = ntohl(x))
#define	NTOHS(x)	((x) = ntohs(x))
#define	HTONL(x)	((x) = htonl(x))
#define	HTONS(x)	((x) = htons(x))
#endif

/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981.
 */

/*
 * Protocols
 */
#define	IPPROTO_IP		0		/* dummy for IP */
#define	IPPROTO_ICMP		1		/* control message protocol */
#define	IPPROTO_GGP		3		/* gateway^2 (deprecated) */
#define	IPPROTO_TCP		6		/* tcp */
#define	IPPROTO_EGP		8		/* exterior gateway protocol */
#define	IPPROTO_PUP		12		/* pup */
#define	IPPROTO_UDP		17		/* user datagram protocol */
#define	IPPROTO_IDP		22		/* xns idp */
#define	IPPROTO_TP		29 		/* tp-4 w/ class negotiation */
#define	IPPROTO_IPV6		41		/* IPv6 protocol */
#define	IPPROTO_EON		80		/* ISO cnlp */
#define	IPPROTO_UDPLITE		136		/* UDP Lite */

#define	IPPROTO_RAW		255		/* raw IP packet */
#define	IPPROTO_MAX		256


/*
 * Local port number conventions:
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 * Ports > IPPORT_USERRESERVED are reserved
 * for servers, not necessarily privileged.
 */
#define	IPPORT_RESERVED		1024
#define	IPPORT_USERRESERVED	5000

/*
 * Internet address (a structure for historical reasons)
 */
typedef uint32_t in_addr_t;

struct in_addr {
	in_addr_t s_addr;
};

/*
 * Definitions of bits in internet address integers.
 * On subnets, the decomposition of addresses to host and net parts
 * is done according to subnet mask, not the masks here.
 */
#define	IN_CLASSA(i)		(((long)(i) & 0x80000000) == 0)
#define	IN_CLASSA_NET		0xff000000
#define	IN_CLASSA_NSHIFT	24
#define	IN_CLASSA_HOST		0x00ffffff
#define	IN_CLASSA_MAX		128

#define	IN_CLASSB(i)		(((long)(i) & 0xc0000000) == 0x80000000)
#define	IN_CLASSB_NET		0xffff0000
#define	IN_CLASSB_NSHIFT	16
#define	IN_CLASSB_HOST		0x0000ffff
#define	IN_CLASSB_MAX		65536

#define	IN_CLASSC(i)		(((long)(i) & 0xe0000000) == 0xc0000000)
#define	IN_CLASSC_NET		0xffffff00
#define	IN_CLASSC_NSHIFT	8
#define	IN_CLASSC_HOST		0x000000ff

#define	IN_CLASSD(i)		(((long)(i) & 0xf0000000) == 0xe0000000)
#define	IN_MULTICAST(i)		IN_CLASSD(i)

#define	IN_EXPERIMENTAL(i)	(((long)(i) & 0xe0000000) == 0xe0000000)
#define	IN_BADCLASS(i)		(((long)(i) & 0xf0000000) == 0xf0000000)

#define	INADDR_ANY		(u_long)0x00000000
#define	INADDR_BROADCAST	(u_long)0xffffffff	/* must be masked */
#define	INADDR_LOOPBACK		(u_long)0x7f000001	/* loopback address */
#if !defined(KERNEL) || defined(AMITCP)
#define	INADDR_NONE		0xffffffff		/* -1 return */
#endif

#define	IN_LOOPBACKNET		127			/* official! */

/*
 * Socket address, internet style.
 */
struct sockaddr_in {
	uint8_t	sin_len;
	uint8_t	sin_family;
	uint16_t	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};

/*
 * Structure used to describe IP options.
 * Used to store options internally, to pass them to a process,
 * or to restore options retrieved earlier.
 * The ip_dst is used for the first-hop gateway when using a source route
 * (this gets put into the header proper).
 */
struct ip_opts {
	struct	in_addr ip_dst;		/* first hop, 0 w/o src rt */
	char	ip_opts[40];		/* actually variable in size */
};

/*
 * Options for use with [gs]etsockopt at the IP level.
 * First word of comment is data type; bool is stored in int.
 */
#define	IP_OPTIONS	1	/* buf/ip_opts; set/get IP per-packet options */
#define	IP_HDRINCL	2	/* int; header is included with data (raw) */
#define	IP_TOS		3	/* int; IP type of service and precedence */
#define	IP_TTL		4	/* int; IP time to live */
#define	IP_RECVOPTS	5	/* bool; receive all IP options w/datagram */
#define	IP_RECVRETOPTS	6	/* bool; receive IP options for response */
#define	IP_RECVDSTADDR	7	/* bool; receive IP dst addr w/datagram */
#define	IP_RETOPTS	8	/* ip_opts; set/get IP per-packet options */

#ifdef KERNEL
struct in_addr in_makeaddr(u_long net,
                           u_long host);
u_long in_netof(struct in_addr in);
u_long in_lnaof(struct in_addr in);
#endif

/* IPv6 structures */
struct in6_addr {
    union {
        uint8_t         u8_addr[16];
        uint16_t        u16_addr[8];
        uint32_t        u32_addr[4];
    } un;
#define s6_addr un.u8_addr
};

#define IN6ADDR_ANY_INIT {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}}}          /* :: */
#define IN6ADDR_LOOPBACK_INIT {{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}}     /* ::1 */

struct sockaddr_in6 {
    uint8_t     sin6_len;
    uint8_t     sin6_family;
    uint16_t    sin6_port;
    uint32_t    sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t    sin6_scope_id;
};

/* IPv6 socket options */
#define IPV6_V6ONLY     26      /* Restrict AF_INET6 sockets to IPv6 only. */

#endif /* !IN_H */
