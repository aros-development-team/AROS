/*
 * Copyright (c) 1983, 1987, 1989 The Regents of the University of California.
 * All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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
 *
 *	@(#)resolv.h	5.10 (Berkeley) 6/1/90
 */

#ifndef API_RESOLV_H
#define API_RESOLV_H

#include <arpa/nameser.h>

/*
 * Global defines and variables for resolver stub. (INSIDE AmiTCP/IP)
 */
#define	MAXDFLSRCH	3		    /* # default domain levels to try */
#define	LOCALDOMAINPARTS 2		/* min levels in name that is "local"*/

#define	RES_TIMEOUT	5		    /* min. seconds between retries */

/*
 * defines to hook variables to the library base.
 */
#define h_errno (*libPtr->hErrnoPtr)
#define _res (libPtr->res_state)
#define res_sock (libPtr->res_socket)

/*
 * Resolver options
 */
#define RES_INIT	0x0001		/* address initialized */
#ifndef AMITCP /* IMPORTANT TO HAVE RES_DEBUG NOT DEFINED IF NOT DEBUGGING */
#define RES_DEBUG	0x0002		/* print debug messages */
#endif
#define RES_AAONLY	0x0004		/* authoritative answers only */
#define RES_USEVC	0x0008		/* use virtual circuit */
#define RES_PRIMARY	0x0010		/* query primary server only */
#define RES_IGNTC	0x0020		/* ignore trucation errors */
#define RES_RECURSE	0x0040		/* recursion desired */
#define RES_DEFNAMES	0x0080		/* use default domain name */
#define RES_STAYOPEN	0x0100		/* Keep TCP socket open */
#define RES_DNSRCH	0x0200		/* search up local domain tree */

#define RES_DEFAULT	(RES_RECURSE | RES_DEFNAMES | RES_INIT)

#define AROSTCP_RES_DISABLED 0x8000       /* Implements MiamiDisallowDNS() */

/* Resolver state */
struct state {
	int	retrans;	 	/* retransmition time interval */
	int	retry;			/* number of times to retransmit */
	long	options;		/* option flags - see below. */
	u_short	id;			/* current packet id */
	ULONG   dbserial;
        char **dnsrch;
        struct in_addr *nsaddr_list;
};

#ifndef AMITCP /* AmiTCP has this in the SocketBase */ 
extern struct state _res;
#endif

/*
 * Prototypes
 */
extern int res_init(struct state *state);
extern int res_update_db(struct state *state);
extern void res_cleanup_db(struct state *state);
extern int res_query(struct SocketBase *,
		     const char *, int, int, u_char *, int);
extern int res_search(struct SocketBase *,
		      const char *, int, int, u_char *, int);
extern int dn_expand(const u_char *, const u_char *, const u_char *, u_char *,
		     int);
extern int dn_comp(const u_char *, u_char *, int, u_char **, u_char **);
extern int __dn_skipname(const u_char *, const u_char *);

/* struct rrec; */
extern int res_mkquery(struct SocketBase *,
		       int, const char *, int, int, const char *,
		       int, const struct rrec *, char *, int);
extern int res_querydomain(struct SocketBase *, const char *, const char *,
			   int, int, u_char *, int);
extern int res_send(struct SocketBase *, const char *, int, char *, int);
extern void _res_close(struct SocketBase *);

extern u_short _getshort(u_char *);
extern u_long _getlong(u_char *);
extern void __putshort(u_short, u_char *);
extern void __putlong(u_long, u_char *);

#endif /* API_RESOLV_H */
