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
 * Copyright (c) 1985, 1988 Regents of the University of California.
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
static char sccsid[] = "@(#)gethostnamadr.c	6.45 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#include <conf.h>

#include <aros/libcall.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <netinet/in.h>
#include <net/if.h> /* for the gethostid() needs */
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/errno.h>

#include <arpa/nameser.h>
#include <api/resolv.h>
#include <kern/amiga_includes.h>
#include <api/amiga_api.h>
#include <api/amiga_libcallentry.h>
#include <api/amiga_raf.h>
#include <api/allocdatabuffer.h>     
#include <kern/amiga_subr.h>

#include <api/gethtbynamadr.h>     /* prototypes (NO MORE BUGS HERE) */
#include <api/apicalls.h>

#include <proto/bsdsocket.h>

#define	MAXALIASES	35
#define	MAXADDRS	35

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

/*
 * macro for getting error value from another library base function
 * ( which is called directly here )
 */

/*
 * hostent structure in SocketBase
 */
#define HOSTENT ((struct hostent *)libPtr->hostents.db_Addr)

/*
 * longword align given pointer (i.e. divides by 4)
 */
#define ALIGN(p) (((u_int)(p) + (sizeof(long) - 1)) &~ (sizeof (long) -1))

#define MAXALIASES	35
#define MAXADDRS	35  
  
typedef char hostbuf_t[512];

struct hoststruct {
  char * host_aliases[MAXALIASES + 1];
  char * h_addr_ptrs[MAXADDRS + 1];
  short host_alias_count;
  short h_addr_count;
  struct hostent host;
  hostbuf_t hostbuf;
};

static struct hostent * makehostent(struct SocketBase * libPtr,
				    struct hoststruct * HS,
				    char * ptr);

LONG usens = 2;

static char *
 getanswer(struct SocketBase * libPtr, querybuf *answer,int anslen, int iquery,
	    struct hoststruct * HS)
{
  register HEADER *hp;
  register u_char *cp; /* pointer to traverse in 'answer' */
  register int n;
  u_char *eom;
  int buflen = sizeof HS->hostbuf;
  char *bp; /* bp -- answer buffer pointer */
  int type, class, ancount, qdcount;
  int haveanswer, getclass = C_ANY;
  char **ap, **hap;

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) getanswer()\n"));
#endif

  eom = answer->buf + anslen;
  /*
   * find first satisfactory answer
   */
  hp = &answer->hdr;
  ancount = ntohs(hp->ancount); /* how many answers returned from nameserver */
  qdcount = ntohs(hp->qdcount); /* how many questions in nameserver query */

  /*
   *  bp, points to start of buffer space where new resolved answer is to 
   *  be written. the bp is moved to next free space. Initially it is
   *  set below, to start of buffer allocated for it-
   */
  bp = HS->hostbuf;
  /*
   * address cp to start of nameserver answers (after static sized header)
   */
  cp = answer->buf + sizeof(HEADER);

  /*
   * Any questions asked..hmm this should always be the case
   */
  if (qdcount) {
#if 0			/* added by too 8.Sep.1993: skipping strange parts */
    /*
     * gethostbyaddr uses inverse query...
     */
    if (iquery) {
      if ((n = dn_expand((u_char *)answer->buf,
			 (u_char *)eom, (u_char *)cp, (u_char *)bp,
			 buflen)) < 0) {
	h_errno = NO_RECOVERY;
	return NULL;
      }
      cp += n + QFIXEDSZ;
      /*
       * Hostname in final hostent structure is written here in case
       * of gethostbyaddr. (from question section ???)
       */
      HS->host.h_name = bp;
      n = strlen(bp) + 1;
      bp += n;
      buflen -= n;
    }
    else
#endif	/* 0 */		/* 8Sep93: now code below skips all question strings */
      /*
       * here is normal query (gethostbyname). skipping query section
       * hmm, wondering why is it originally implemented as 2
       * __dn_skipname function calls ?
       */
      cp += __dn_skipname(cp, eom) + QFIXEDSZ;
    while (--qdcount > 0)
      cp += __dn_skipname(cp, eom) + QFIXEDSZ;
  }
  else if (iquery) {
    /*
     * no questions and inverse query :o
     */
    if (hp->aa)
      h_errno = HOST_NOT_FOUND;
    else
      h_errno = TRY_AGAIN;
    return NULL;
  }
  ap = HS->host_aliases;
  HS->host_alias_count = 1; /* there is always NULL as last pointer */
  hap = HS->h_addr_ptrs;
  HS->h_addr_count = 1; /* there is always NULL as last pointer */
  
  haveanswer = 0;
  while (--ancount >= 0 && cp < eom) {
    if ((n = dn_expand((u_char *)answer->buf, (u_char *)eom,
		       (u_char *)cp, (u_char *)bp, buflen)) < 0)
      break;
    cp += n;
    /*
     * Type and class are type and class of answer in returned resource
     * record. see arpa[_/]nameserver.h for more information.
     */
    type = _getshort(cp); 
    cp += sizeof(u_short);
    class = _getshort(cp);
    cp += sizeof(u_short) + sizeof(u_int32_t);
    n = _getshort(cp);
    cp += sizeof(u_short);
    
    if (type == T_CNAME) {   /* canonical name (add alias names)*/
      cp += n;
      if (HS->host_alias_count >= MAXALIASES)
	continue;
      *ap++ = bp;
      HS->host_alias_count++;
      n = strlen(bp) + 1;
      bp += n;
      buflen -= n;
      continue;
    }
    if (iquery && type == T_PTR) {  /* domain name pointer (get domain
				       name and return) */
      if ((n = dn_expand((u_char *)answer->buf,
			 (u_char *)eom, (u_char *)cp, (u_char *)bp,
			 buflen)) < 0) {
	cp += n;
	continue;
      }
      cp += n;
      HS->host.h_name = bp;   /* well, rewrites name pointer if there were
				 returned questions also... */
      haveanswer = 1;
      bp+= (strlen(bp) + 1);
      break;
    }
    if (iquery || type != T_A)  {
      /*
       * here is strange answer from nameserver: inverse query should have
       * been handled earlyer and there should not be any other types
       * left than "host address"
       */
#ifdef RES_DEBUG
      printf("unexpected answer type %d, size %d\n",
	     type, n);
#endif
      cp += n;
      continue;
    }
    if (haveanswer) {
      /*
       * Here if one host address answer is already returned (rather odd...)
       */
      if (n != HS->host.h_length) {
	cp += n;
	continue;
      }
      if (class != getclass) {
	cp += n;
	continue;
      }
    }
    else {
      /*
       * Fill in host address data and comparing info for next cycle (if any)
       */
      HS->host.h_length = n;
      getclass = class;
      HS->host.h_addrtype = (class == C_IN) ?
	AF_INET : AF_UNSPEC;
      if (!iquery) {
	/*
	 * if not inverse query and haveanswer = 0 host name is first in
	 * bp pointed buffer. (rather strange if answer already returned
	 * and new addresses are to be added since aren't in that case
	 * also names returned or is it inconsistent or have i missed
	 * something ?
	 */
	int n1;

	HS->host.h_name = bp;
	n1 = strlen(bp) + 1;
	bp += n1;
	buflen -= n1;
      }
    }

/*    bp = (char *)ALIGN(bp); /* align answer buffer for next host address */
		
    if (HS->host.h_length >= buflen) {
#ifdef RES_DEBUG
      printf("size (%d) too big\n", HS->host.h_length);
#endif
      break;
    }
    /*
     * Fill next host address in address list
     */
    bcopy(cp, *hap++ = bp, n);
    HS->h_addr_count++;
    bp += n;
    buflen -= n;
    cp += n;
    haveanswer++;
  } /* while (--ancount ...) */
  
  if (haveanswer) {
    *ap = NULL;
    *hap = NULL;
    return bp;
  }
  else {
    h_errno = TRY_AGAIN;
    return NULL;
  }
}

int isipaddr(const char *name)
{
	const char *c;
	int i=0;
	for (c=name;*c;c++)
	{
		if (!isdigit(*c))
		{
			if ((*c) == '.')
				i++;
			else
				return 0;
		}
	}
	return (i == 3);
}

struct hostent *__gethostbyname(const char *name, struct SocketBase *libPtr)
{
  querybuf *buf;
  int n;
  char * ptr;
  struct hoststruct * HS = NULL;
  struct hostent * anshost = NULL;

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname('%s')\n", name));
#endif

  CHECK_TASK2();

  /*
   * check if name consists of only dots and digits.
   */
  if (isipaddr(name)) {
    struct in_addr inaddr;
    u_long * lptr;

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: name IS an IP address\n"));
#endif

    if (!__inet_aton(name, &inaddr)) {
      writeErrnoValue(libPtr, EINVAL);
      h_errno = 0;
      return NULL;
    }

    if (allocDataBuffer(&libPtr->hostents,
			sizeof (struct hostent) + 28) == FALSE) {
      writeErrnoValue(libPtr, ENOMEM);
      h_errno = 0;
      return NULL;
    }
    HOSTENT->h_addrtype = AF_INET;
    HOSTENT->h_length = sizeof (struct in_addr);
    lptr = (u_long *)(HOSTENT + 1);
    *lptr++ = inaddr.s_addr;
    *(u_long **)(lptr) = lptr - 1;
    HOSTENT->h_addr_list = (char **)lptr;
    *++lptr = NULL;
    HOSTENT->h_aliases = (char **)lptr;
    HOSTENT->h_name = strcpy((char *)++lptr, name);

    return HOSTENT;
  }
  else
  {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: name ISNT an IP address\n"));
#endif
  }
  
  /*
   * Search local database (first) is usens not FIRST
   */
  if (usens != 1)
    if ((anshost =_gethtbyname(libPtr, name)) != NULL || usens == 0)
    {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: host found in local database\n"));
#endif
      return anshost;
    }
  /*
   * Here if usens is FIRST or host not in local database and usens is SECOND
   */
  if ((HS = bsd_malloc(sizeof (querybuf) + sizeof (struct hoststruct),
		       M_TEMP, M_WAITOK)) == NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: couldnt allocate memory for res search entry\n"));
#endif
    writeErrnoValue(libPtr, ENOMEM);
    return NULL;
  }
  buf = (querybuf *)(HS + 1);

  n = res_search(libPtr, name, C_IN, T_A, buf->buf, sizeof (querybuf));
  if (n >= 0) {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [res_search] returns %d\n", n));
#endif
    ptr = getanswer(libPtr, buf, n, 0, HS);
    if (ptr != NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [getanswer] returns valid ptr\n"));
#endif
      if ((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [makehostent] returns success\n"));
#endif
	anshost->h_addrtype = HS->host.h_addrtype;
	anshost->h_length = HS->host.h_length;
      }
    }
  }
  else {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [res_search] failed\n"));
#endif
#ifdef RES_DEBUG
    printf("res_search failed\n");
#endif
    /*
     * If usens is FIRST and host not found using resolver.
     */
    if (usens != 2)
      anshost =_gethtbyname(libPtr, name);
  }
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: finished search\n"));
#endif

  if (HS)
    bsd_free(HS, M_TEMP);
  return anshost;
}

AROS_LH1(struct hostent *, gethostbyname,
   AROS_LHA(char *, name, A0),
   struct SocketBase *, libPtr, 30, UL)
{
  AROS_LIBFUNC_INIT
  return __gethostbyname(name, libPtr);
  AROS_LIBFUNC_EXIT
}



struct hostent * __gethostbyaddr(UBYTE *addr, int len, int type, struct SocketBase * libPtr)
{
  querybuf * buf;
  int n;
  char * ptr;
  struct hoststruct * HS = NULL;
  char * qbuf;
  struct hostent * anshost = NULL;
  
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) __gethostbyaddr()\n"));  
#endif

  CHECK_TASK2();
  
  if (type != AF_INET)
    return ((struct hostent *) NULL);
  
  /*
   * Search local database (first) is usens not FIRST
   */
  if (usens != 1)
    if ((anshost =_gethtbyaddr(libPtr, addr, len, type)) != NULL || usens == 0)
      return anshost;

  /*
   * Here if usens is FIRST or host not in local database and usens is SECOND
   */
  if ((HS = bsd_malloc(sizeof (querybuf) + MAXDNAME + 1 +
		       sizeof (struct hoststruct),  M_TEMP, M_WAITOK))
      == NULL) {
    writeErrnoValue(libPtr, ENOMEM);
    return NULL;
  }
  buf = (querybuf *)(HS + 1);
  qbuf = (caddr_t)(buf + 1);
  
  (void)sprintf(qbuf, "%lu.%lu.%lu.%lu.in-addr.arpa",
		((unsigned)addr[3] & 0xff),
		((unsigned)addr[2] & 0xff),
		((unsigned)addr[1] & 0xff),
		((unsigned)addr[0] & 0xff));
  n = res_query(libPtr, qbuf, C_IN, T_PTR, (char *)buf, sizeof (querybuf));

  if (n >= 0) {
    ptr = getanswer(libPtr, buf, n, 1, HS);
    if (ptr != NULL) {
      if (HS->h_addr_count == 1) {
	HS->h_addr_count++;
	bcopy(addr, ptr, len);
	HS->h_addr_ptrs[0] = ptr;
	ptr += len;
      }      
      else
	bcopy(addr, &HS->h_addr_ptrs[0], len);
      HS->h_addr_ptrs[1] = NULL;
      if ((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
	anshost->h_addrtype = type;
	anshost->h_length = len;
      }
    }
  }
  else {
#ifdef RES_DEBUG
    printf("res_query failed\n");
#endif
    /*
     * If usens is FIRST and host not found using resolver.
     */
    if (usens != 2)
      anshost = _gethtbyaddr(libPtr, addr, len, type);
  }
  if (HS)
    bsd_free(HS, M_TEMP);
  return anshost;
}

AROS_LH3(struct hostent *, gethostbyaddr,
   AROS_LHA(UBYTE *, addr, A0),
   AROS_LHA(int, len, D0),
   AROS_LHA(int, type, D1),
   struct SocketBase *, libPtr, 31, UL)
{
  AROS_LIBFUNC_INIT
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostbyaddr()\n"));
#endif

  return __gethostbyaddr(addr, len, type, libPtr);
  AROS_LIBFUNC_EXIT
}

static struct hostent * makehostent(struct SocketBase * libPtr,
				    struct hoststruct * HS,
				    char * ptr)
{
  int n, i;

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) makehostent()\n"));
#endif

  i = (caddr_t)ALIGN(ptr) - (caddr_t)&HS->hostbuf;
  n = i + sizeof (struct hostent) + HS->h_addr_count * sizeof (char *) +
    HS->host_alias_count * sizeof (char *);
    
  if (allocDataBuffer(&libPtr->hostents, n) == FALSE) {
    writeErrnoValue(libPtr, ENOMEM);    
    return NULL;
  }
  /*
   * copy ent data to user buffer (pointers will be set later)
   */
  bcopy(HS->hostbuf, (caddr_t)(HOSTENT + 1), i);

  /*
   * how much to add to old pointers
   */
  n = (caddr_t)HOSTENT + sizeof(struct hostent) - (caddr_t)&HS->hostbuf;
  
  /*
   * fill vital fields in user hostent structure
   */
  HOSTENT->h_name = HS->host.h_name + n;

  HOSTENT->h_aliases = (char **)((char *)(HOSTENT + 1) + i);
  for (i = 0; HS->host_aliases[i]; i++)
    HOSTENT->h_aliases[i] = HS->host_aliases[i] + n;
  HOSTENT->h_aliases[i++] = NULL;

  HOSTENT->h_addr_list = HOSTENT->h_aliases + i;
  for (i = 0; HS->h_addr_ptrs[i]; i++)
    HOSTENT->h_addr_list[i] = HS->h_addr_ptrs[i] + n;
  HOSTENT->h_addr_list[i] = NULL;

  return HOSTENT;
}

/*
 * id_addr variable is used by both the gethostname() and gethostid().
 * 
 * host_name is the host_name configuration variable.
 */
static ULONG id_addr = 0;

void findid(ULONG *); /* defined in net/if.c */

/*
 * Global host name and name length
 */
char host_name[MAXHOSTNAMELEN+1] = { 0 };
size_t host_namelen = 0;

/****i* AmiTCP/sethostname *********************************************
*
*   NAME   
*       sethostname -- set the name of the host
*
*   SYNOPSIS
*       error = sethostname(name, namelen);
*
*       int sethostname(const char *, size_t);
*
*   FUNCTION
*       Set the name of the host to the given 'name' of length 'namelen'.
*
*   INPUTS
*       name    - Pointer to the name string.
*       namelen - Length of the name.
*
*   RESULT
*       error   - 0 on success.
*  
*   EXAMPLE
*
*   NOTES
*       This function is not intended to be provided to the applications,
*       this is for AmiTCP internal use only (at least for now).
*
*   BUGS
*
*   SEE ALSO
*****************************************************************************
*
*/
int sethostname(const char *name, size_t namelen)
{
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) sethostname()\n"));
#endif

  if (namelen > MAXHOSTNAMELEN)
    namelen = MAXHOSTNAMELEN;

  memcpy(host_name, name, namelen);
  host_name[namelen] = '\0';
  host_namelen = namelen;
  SetVar("HOSTNAME", name, namelen, GVF_GLOBAL_ONLY);

  return 0;
}

/****** bsdsocket.library/gethostname *************************************
*
*   NAME   
*       gethostname -- get the name of the host
*
*   SYNOPSIS
*       error = gethostname(name, namelen);
*
*       long gethostname(char *, long);
*
*   FUNCTION
*       Get the name of the host to the buffer name of length namelen.
*       The name is queried from the netdb and/or the name server if 
*       it is not explicitly configured (configuration variable
*       HOSTNAME).
*
*   INPUTS
*       name    - Pointer to the buffer where the name should be
*                 stored.
*       namelen - Length of the buffer name.
*
*   RESULT
*       error   - 0 on success.
*  
*   EXAMPLE
*       char hostname[MAXHOSTNAMELEN];
*       long error;
*       
*       error = gethostname(hostname, sizeof(hostname));
*       if (error < 0)
*         exit(10);
*       
*       printf("My name is \"%s\".\n", hostname);
*
*   NOTES
*
*   BUGS
*       Unlike the Unix version, this version assures that the
*       resulting string is always NULL-terminated.
*
*   SEE ALSO
*       gethostid()
*****************************************************************************
*
*/
LONG __gethostname(STRPTR name, LONG namelen, struct SocketBase * libPtr)
{
  size_t host_namelen = 0;

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) __gethostname()\n"));
#endif

  CHECK_TASK();

  /*
   * Get the name with the gethostbyaddr(), if the name is not set yet.
   */
  if (*host_name == '\0')
  {
    struct hostent * hent;
    /* gethostid() */
    if (id_addr == 0)
      findid(&id_addr);

    if (id_addr != 0)
    { /* query if we have an address */
      hent = __gethostbyaddr((UBYTE *)&id_addr,
			   sizeof(id_addr), AF_INET, libPtr);

      if (hent != NULL)
      {
	     host_namelen = strlen(hent->h_name);
	     sethostname(hent->h_name, host_namelen);
      }
      else
      {
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) __gethostname: BUG!?! No Host ??\n"));
#endif
      }
    }
  }
  else host_namelen = strlen(host_name);

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) __gethostname: namelen: %d host_namelen: %d\n", namelen, host_namelen));
#endif

  /*
   * Copy the name to the user buffer. stccpy() ensures that the buffer
   * is not written over and that it will be null-terminated.
   */
  if (namelen > host_namelen)
    namelen = host_namelen;
  else
    namelen--;			/* make space for the trailing '\0' */

  memcpy(name, host_name, namelen);
  name[namelen] = '\0';

  API_STD_RETURN(0, 0);
}

AROS_LH2(LONG, gethostname,
   AROS_LHA(STRPTR, name, A0),
   AROS_LHA(LONG, namelen, D0),
   struct SocketBase *, libPtr, 32, UL)
{
  AROS_LIBFUNC_INIT
#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostname()\n"));
#endif
  return __gethostname(name, namelen, libPtr);
  AROS_LIBFUNC_EXIT
}

/****** bsdsocket.library/gethostid ***************************************
*
*   NAME   
*       gethostid -- get an unique 32-bit id to this host
*
*   SYNOPSIS
*       id = gethostid();
*
*       ULONG gethostid(void);
*
*   FUNCTION
*       Return the 32-bit unique id for this host. The Internet
*       address if the primary interface is used as the unique id.
*       This means that this function is also a supported way to get
*       the hosts IP address in AmiTCP/IP. If no interfaces are
*       configured, zero is returned. Any non-loobpack interface with
*       is preferred. Only if no other interfaces are present, is the
*       loopback address returned.
*       
*   INPUTS
*
*   RESULT
*       id  - non-zero on success.
*  
*   EXAMPLE
*       ULONG id;
*       
*       id = gethostid();
*       if (id == 0)
*         exit(10);
*       
*       printf("My primary IP address is: %s.\n", Inet_NtoA(id));
*
*   NOTES
*       Non-zero id is returned as soon as a interface is configured.
*       After that the id will not change, not even if the id is the
*       address of the loopback interface.
*
*   BUGS
*
*   SEE ALSO
*****************************************************************************
*
*/
/*ULONG SAVEDS gethostid(
   REG(a6, struct SocketBase * libPtr))*/
AROS_LH0(ULONG, gethostid,
   struct SocketBase *, libPtr, 33, UL)
{
  AROS_LIBFUNC_INIT

#if defined(__AROS__)
D(bug("[AROSTCP](gethostnameadr.c) gethostid()\n"));
#endif

  if (id_addr == 0)
    findid(&id_addr);
  return id_addr;
  AROS_LIBFUNC_EXIT
}
