/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 - 2011 Pavel Fedin
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

/*-
 * Copyright (c) 1980, 1983, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef NETDB_H
#define NETDB_H

#include <sys/types.h>

#include <sys/socket.h>     /* for socklen_t */

/*
 * Structures returned by network data base library.  All addresses are
 * supplied in host order, and returned in network order (suitable for
 * use in system calls).
 */
struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (left in extern int h_errno).
 */
#ifndef KERNEL

#include <bsdsocket/socketbasetags.h>
#include <proto/socket.h>

static inline int *__get_h_errno_ptr(struct Library *SocketBase)
{
    int *ptr;
    struct TagItem tags[] = {
    	{SBTM_GETREF(SBTC_HERRNOLONGPTR), (STACKIPTR)&ptr},
    	{TAG_DONE			, 0   }
    };

    SocketBaseTagList(tags);
    return ptr;
}

#define h_errno (*__get_h_errno_ptr(SocketBase))

#endif

#define	HOST_NOT_FOUND	1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */

struct addrinfo {
  long		ai_flags;		/* AI_PASSIVE, AI_CANONNAME */
  long		ai_family;		/* PF_xxx */
  long		ai_socktype;		/* SOCK_xxx */
  long		ai_protocol;		/* IPPROTO_xxx for IPv4 and IPv6 */
  size_t	   ai_addrlen;		/* length of ai_addr */
  char		*ai_canonname;		/* canonical name for host */
  struct sockaddr	*ai_addr;	/* binary address */
  struct addrinfo	*ai_next;	/* next structure in linked list */
};

			/* following for getaddrinfo() */
#define	AI_PASSIVE	1      /* socket is intended for bind() + listen() */
#define	AI_CANONNAME	2      /* return canonical name */

			/* following for getnameinfo() */
#define	NI_MAXHOST	  1025	/* max hostname returned */
#define	NI_MAXSERV	    32	/* max service name returned */

#define	NI_NOFQDN	     1	/* do not return FQDN */
#define	NI_NUMERICHOST   2	/* return numeric form of hostname */
#define	NI_NAMEREQD	     4	/* return error if hostname not found */
#define	NI_NUMERICSERV   8	/* return numeric form of service name */
#define	NI_DGRAM	    16	/* datagram service for getservbyname() */

			/* error returns */
#define	EAI_ADDRFAMILY	 1	/* address family for host not supported */
#define	EAI_AGAIN	 2	/* temporary failure in name resolution */
#define	EAI_BADFLAGS	 3	/* invalid value for ai_flags */
#define	EAI_FAIL	 4	/* non-recoverable failure in name resolution */
#define	EAI_FAMILY	 5	/* ai_family not supported */
#define	EAI_MEMORY	 6	/* memory allocation failure */
#define	EAI_NODATA	 7	/* no address associated with host */
#define	EAI_NONAME	 8	/* host nor service provided, or not known */
#define	EAI_SERVICE	 9	/* service not supported for ai_socktype */
#define	EAI_SOCKTYPE	10	/* ai_socktype not supported */
#define	EAI_SYSTEM	11	/* system error returned in errno */


/* Flags for ResetNetDB() */
#define NETDB_IFF_CLEANOLD   1
#define NETDB_IFF_MODIFYOLD  2
#define NETDB_IFF_ADDNEW     4

#endif /* !NETDB_H */

