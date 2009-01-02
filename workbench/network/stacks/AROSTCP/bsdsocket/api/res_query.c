/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 - 2007 The AROS Dev Team
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
 * Copyright (c) 1988 Regents of the University of California.
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)res_query.c	5.11 (Berkeley) 3/6/91";
#endif /* LIBC_SCCS and not lint */

#include <conf.h>

#include <sys/param.h>
#include <sys/malloc.h>
#include <netinet/in.h>
/* #include <arpa/inet.h> */
#include <arpa/nameser.h>
#include <api/resolv.h>
#include <netdb.h>
#ifndef AMITCP
#include <ctype.h>
#endif     
#include <sys/errno.h>

#include <kern/amiga_includes.h>
#include <kern/amiga_subr.h>     
#include <api/amiga_api.h>
#include <kern/amiga_netdb.h>

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, -1 on error.
 * Caller must parse answer and determine whether it answers the question.
 */
int
res_query(struct SocketBase *	libPtr,
	  const char *		name,		/* domain name */
	  int 			class,
	  int			type,		/* class and type of query */
	  u_char *		answer,		/* buffer to put answer */
	  int 			anslen)		/* size of answer buffer */
{
	char *buf;
	HEADER *hp;
	int n;

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query()\n"));
#endif

	if ((_res.options & RES_INIT) == 0 && res_init(&_res) == -1)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: resolver not initialised/failed to init\n"));
#endif
		return (-1);
	}

	if ((_res.dbserial != ndb_Serial) && res_update_db(&_res) == -1)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: resolver failed to update\n"));
#endif
		return (-1);
	}


	/* an idea to remove reduntant bsd_mallocs and bsd_frees. let's make
	   a new structure in gethostby(name|addr) and put libPrt->buffer
	   point to it (first item in new structure is old value of that
	   buffer). that structure holds pointers to all needed character
	   buffers and is initialized to NULL. at end gethostby... bsd_frees
	   all buffers that are pointed there */
	
	if ((buf = bsd_malloc(MAXPACKET, M_TEMP, M_WAITOK)) == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: Failed to allocate query entry\n"));
#endif
	  writeErrnoValue(libPtr, ENOMEM);
	  return -1;
	}

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: name: %s class: %d type %d\n", name, class, type));
#endif
#ifdef RES_DEBUG
		printf("res_query(%s, %d, %d)\n", name, class, type);
#endif
	n = res_mkquery(libPtr, QUERY, name, class, type, (char *)NULL, 0, NULL,
			buf, MAXPACKET);

	if (n <= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_mkquery] Failed\n"));
#endif
#ifdef RES_DEBUG
			printf("res_query: mkquery failed\n");
#endif
			h_errno = NO_RECOVERY;
			goto Return;
	}

#warning "TODO: NicJA - Where is CHECK_POINTER() !!!!"
#if !defined(__AROS__)
	CHECK_POINTER(buf);
#endif

	n = res_send(libPtr, buf, n, (char *)answer, anslen);

	if (n < 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Failed\n"));
#endif
#ifdef RES_DEBUG
			printf("res_query: send error\n");
#endif
			h_errno = TRY_AGAIN;
			goto Return;
	}

	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Error: rcode = %d, ancount = %d\n", hp->rcode, ntohs(hp->ancount)));
#endif
#ifdef RES_DEBUG
			printf("rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Error: Host not found!\n"));
#endif
				h_errno = HOST_NOT_FOUND;
				break;
			case SERVFAIL:
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Error: Server Fail!\n"));
#endif
				h_errno = TRY_AGAIN;
				break;
			case NOERROR:
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Error: !IMPOSSIBLE CASE!\n"));
#endif
				h_errno = NO_DATA;
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
			default:
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_query: [res_send] Error: Unknown!\n"));
#endif
				h_errno = NO_RECOVERY;
				break;
		}
		n = -1;
		goto Return;
	}

Return:
	bsd_free(buf, M_TEMP);
	return(n);
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error number is left in h_errno.
 * Only useful for queries in the same name hierarchy as the local host
 * (not, for example, for host address-to-name lookups in domain in-addr.arpa).
 */
int
res_search(struct SocketBase *	libPtr,
	   const char *		name,		/* domain name */
	   int			class,
	   int			type,		/* class and type of query */
	   u_char *		answer,		/* buffer to put answer */
	   int 			anslen)		/* size of answer */
{
	register const char *cp;
	char **domain;
	
	int n, ret, got_nodata = 0;
	int tmp_errno;

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search('%s')\n", name));
#endif
	
#ifndef AMICTP
	char *__hostalias();
#endif /* ! AMITCP */	
	if ((_res.options & RES_INIT) == 0 && res_init(&_res) == -1)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver not initialised/failed to init\n"));
#endif
		return (-1);
	}
	else
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver initialised.  base @ %x\n", &_res));
#endif		
	}

	if ((_res.dbserial != ndb_Serial) && res_update_db(&_res) == -1)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver failed to update\n"));
#endif
		return (-1);
	}
	else
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver db updated\n"));
#endif		
	}

	writeErrnoValue(libPtr, 0);
	h_errno = HOST_NOT_FOUND;	       /* default, if we never query */
	for (cp = name, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;

#ifndef AMITCP
	if (n == 0 && (cp = __hostalias(name)))
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: returning hostalias response\n"));
#endif
		return (res_query(cp, class, type, answer, anslen));
	}
#endif /* !AMITCP */

	/*
	 * We do at least one level of search if
	 *	- there is no dot and RES_DEFNAME is set, or
	 *	- there is at least one dot, there is no trailing dot,
	 *	  and RES_DNSRCH is set.
	 */
#if defined(__AROS__) && defined(DEBUG)
D(bug("[AROSTCP](res_query.c) res_search: n =%d\n", n));
	if (_res.options & RES_DEFNAMES)
	{
D(bug("[AROSTCP](res_query.c) res_search: OPTION:Search local domain\n"));
	}
	
	if (_res.options & RES_DNSRCH)
	{
D(bug("[AROSTCP](res_query.c) res_search: OPTION:Search using DNS\n"));
	}
#endif
	
	if ((n == 0 && _res.options & RES_DEFNAMES) ||
	   (n != 0 && *--cp != '.' && _res.options & RES_DNSRCH))
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: Querying DNS servers .. \n"));
#endif
		for (domain = _res.dnsrch; *domain; domain++)
		{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver querying domain '%s'\n", *domain));
#endif
			ret = res_querydomain(libPtr, name, *domain,
				  class, type, answer, anslen);
			if (ret > 0)
			{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: returning querydomain response\n"));
#endif
				return (ret);
			}
			/*
			 * If no server present, give up.
			 * If name isn't found in this domain,
			 * keep trying higher domains in the search list
			 * (if that's enabled).
			 * On a NO_DATA error, keep trying, otherwise
			 * a wildcard entry of another type could keep us
			 * from finding this entry higher in the domain.
			 * If we get some other error (negative answer or
			 * server failure), then stop searching up,
			 * but try the input name below in case it's fully-qualified.
			 */
			tmp_errno = readErrnoValue(libPtr);
			if ((tmp_errno == ECONNREFUSED) || (tmp_errno == ETIMEDOUT) || (tmp_errno == EINTR)) {
				h_errno = TRY_AGAIN;

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver failed to connect, setting retry\n"));
#endif
				return (-1);
			}

			if (h_errno == NO_DATA)
			{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver got no response\n"));
#endif
				got_nodata++;
			}

			if ((h_errno != HOST_NOT_FOUND && h_errno != NO_DATA) ||
				(_res.options & RES_DNSRCH) == 0)
			{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: resolver got response\n"));
#endif
				break;
			}
		}
	}
		/*
		 * If the search/default failed, try the name as fully-qualified,
		 * but only if it contained at least one dot (even trailing).
		 * This is purely a heuristic; we assume that any reasonable query
		 * about a top-level domain (for servers, SOA, etc) will not use
		 * res_search.
		 */
		
	if (n && (ret = res_querydomain(libPtr, name, (char *)NULL,
					class, type, answer, anslen)) > 0)
   {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: returning querydomain.2 result\n"));
#endif
		return (ret);
	}
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_search: finished search.\n"));
#endif
	if (got_nodata) h_errno = NO_DATA;

	return (-1);
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
int
res_querydomain(struct SocketBase *	libPtr,
		const char *		name,
		const char *		domain,
		int 			class,
		int 			type,	/* class and type of query */
		u_char *		answer,	/* buffer to put answer */
		int 			anslen) /* size of answer */
{
	char * nbuf;
	const char * longname;
	char * ptr;
	int n;

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_querydomain('%s', '%s')\n", name, domain));
#endif

	if ((nbuf = bsd_malloc(2*MAXDNAME+2, M_TEMP, M_WAITOK)) == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_querydomain: failed to allocate buffer\n"));
#endif
	  writeErrnoValue(libPtr, ENOMEM);
	  return -1;
	}
	longname = nbuf;
#ifdef RES_DEBUG
		printf("res_querydomain(%s, %s, %d, %d)\n",
		    name, domain, class, type);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
	        n = strlen(name) - 1;
		if (name[n] == '.' && n < (2*MAXDNAME+2) - 1) {
			bcopy(name, nbuf, n);
		        nbuf[n] = '\0';
		} else
			longname = name;
	} else {
		strncpy(nbuf, name, MAXDNAME);
		ptr = nbuf + strlen(name);
		*ptr++ = '.';
		(void)strncpy(ptr, domain, MAXDNAME);
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_querydomain: '%s'\n", nbuf));
#endif
	}

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) res_querydomain: using name '%s'\n", longname));
#endif
	n = res_query(libPtr, longname, class, type, answer, anslen);
	bsd_free(nbuf, M_TEMP);
	return n;
}

#ifndef AMITCP

char *
__hostalias(name)
	register const char *name;
{
	register char *C1, *C2;
	FILE *fp;
	char *file, *getenv(), *strcpy(), *strncpy();
	char buf[BUFSIZ];
	static char abuf[MAXDNAME];

#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) __hostalias('%s')\n", name));
#endif

	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = fopen(file, "r")) == NULL)
	{
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) __hostalias: failed to find env setting\n"));
#endif
		return (NULL);
	}

	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (C1 = buf; *C1 && !isspace(*C1); ++C1);
		if (!*C1)
			break;
		*C1 = '\0';
		if (!strcasecmp(buf, name)) {
			while (isspace(*++C1));
			if (!*C1)
				break;
			for (C2 = C1 + 1; *C2 && !isspace(*C2); ++C2);
			abuf[sizeof(abuf) - 1] = *C2 = '\0';
			(void)strncpy(abuf, C1, sizeof(abuf) - 1);
			fclose(fp);
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) __hostalias: returning %s\n", abuf));
#endif
			return (abuf);
		}
	}
#if defined(__AROS__)
D(bug("[AROSTCP](res_query.c) __hostalias: bad data\n"));
#endif
	fclose(fp);
	return (NULL);
}

#endif /* !AMITCP */
