/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
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
 * Copyright (C) 1985, 1988 Regents of the University of California.
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

/*
 * The host look-up proper of the resolver module (api/resolver_ops.h): by
 * name through the hosts table and the DNS (with the DNS cache), by
 * address through both. The vectors that call it, and what they answer
 * without it - numeric addresses, the localhost names - are the core's
 * (api/resolver_stubs.c).
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <netinet/in.h>
#include <net/if.h> /* for the gethostid() needs */
#include <arpa/inet.h>
#include <netdb.h>
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
#include <api/dns_cache.h>         /* dns_cache_lookup/insert, dns_cache_enabled */
#include <kern/amiga_netdb.h>      /* NDB, LOCK_R_NDB, HostentNode */
#include <api/resolver_ops.h>

#include <net/netcore_map.h>        /* always the last include */

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
#define ALIGN(p) (((long)(p) + (sizeof(long) - 1)) &~ (sizeof (long) -1))

#define MAXALIASES	35
#define MAXADDRS	35

typedef char hostbuf_t[512];

struct hoststruct {
    char *host_aliases[MAXALIASES + 1];
    char *h_addr_ptrs[MAXADDRS + 1];
    short host_alias_count;
    short h_addr_count;
    struct hostent host;
    hostbuf_t hostbuf;
};

static struct hostent *makehostent(struct SocketBase *libPtr,
                                   struct hoststruct *HS,
                                   char *ptr);

static char *
getanswer(struct SocketBase *libPtr, querybuf *answer, int anslen, int iquery,
          int qtype, struct hoststruct *HS)
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
    if(qdcount) {
#if 0			/* added by too 8.Sep.1993: skipping strange parts */
        /*
         * gethostbyaddr uses inverse query...
         */
        if(iquery) {
            if((n = dn_expand((u_char *)answer->buf,
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
            n = strnlen(bp, buflen) + 1;
            bp += n;
            buflen -= n;
        } else
#endif	/* 0 */		/* 8Sep93: now code below skips all question strings */
            /*
             * here is normal query (gethostbyname). skipping query section
             * hmm, wondering why is it originally implemented as 2
             * __dn_skipname function calls ?
             */
            cp += __dn_skipname(cp, eom) + QFIXEDSZ;
        while(--qdcount > 0)
            cp += __dn_skipname(cp, eom) + QFIXEDSZ;
    } else if(iquery) {
        /*
         * no questions and inverse query :o
         */
        if(hp->aa)
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
    while(--ancount >= 0 && cp < eom) {
        if((n = dn_expand((u_char *)answer->buf, (u_char *)eom,
                          (u_char *)cp, (u_char *)bp, buflen)) < 0)
            break;
        cp += n;
        /*
         * Type and class are type and class of answer in returned resource
         * record. see arpa[_/]nameserver.h for more information.
         *
         * Guard against a truncated/malformed packet: the fixed RR header
         * (type, class, ttl, rdlength) is 10 bytes.
         */
        if(cp + 10 > eom)
            break;
        type = _getshort(cp);
        cp += sizeof(u_short);
        class = _getshort(cp);
        cp += sizeof(u_short) + sizeof(u_int32_t);
        n = _getshort(cp);
        cp += sizeof(u_short);
        /*
         * rdlength (n) must not run past the end of the message.
         */
        if(cp + n > eom)
            break;

        if(type == T_CNAME) {    /* canonical name (add alias names)*/
            cp += n;
            if(HS->host_alias_count >= MAXALIASES)
                continue;
            *ap++ = bp;
            HS->host_alias_count++;
            n = strnlen(bp, buflen) + 1;
            bp += n;
            buflen -= n;
            continue;
        }
        if(iquery && type == T_PTR) {
            /* domain name pointer (get domain
            			       name and return) */
            if((n = dn_expand((u_char *)answer->buf,
                              (u_char *)eom, (u_char *)cp, (u_char *)bp,
                              buflen)) < 0) {
                cp += n;
                continue;
            }
            cp += n;
            HS->host.h_name = bp;   /* well, rewrites name pointer if there were
				 returned questions also... */
            haveanswer = 1;
            bp += (strnlen(bp, buflen) + 1);
            break;
        }
        if(iquery || type != qtype)  {
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
        if(haveanswer) {
            /*
             * Here if one host address answer is already returned (rather odd...)
             */
            if(n != HS->host.h_length) {
                cp += n;
                continue;
            }
            if(class != getclass) {
                cp += n;
                continue;
            }
        } else {
            /*
             * Fill in host address data and comparing info for next cycle (if any)
             */
            HS->host.h_length = n;
            getclass = class;
            HS->host.h_addrtype = (class == C_IN) ?
                                  (type == T_AAAA ? AF_INET6 : AF_INET) : AF_UNSPEC;
            if(!iquery) {
                /*
                 * if not inverse query and haveanswer = 0 host name is first in
                 * bp pointed buffer. (rather strange if answer already returned
                 * and new addresses are to be added since aren't in that case
                 * also names returned or is it inconsistent or have i missed
                 * something ?
                 */
                int n1;

                HS->host.h_name = bp;
                n1 = strnlen(bp, buflen) + 1;
                bp += n1;
                buflen -= n1;
            }
        }

//    bp = (char *)ALIGN(bp); /* align answer buffer for next host address */

        if(HS->host.h_length >= buflen) {
#ifdef RES_DEBUG
            printf("size (%d) too big\n", HS->host.h_length);
#endif
            break;
        }
        /*
         * Do not overflow the fixed-size h_addr_ptrs[MAXADDRS+1] array.
         */
        if(HS->h_addr_count >= MAXADDRS)
            break;
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

    if(haveanswer) {
        *ap = NULL;
        *hap = NULL;
        return bp;
    } else {
        h_errno = TRY_AGAIN;
        return NULL;
    }
}

/*
 * The reverse name of an address (in-addr.arpa, ip6.arpa) into qbuf, which
 * has room for MAXDNAME + 1 octets.
 */
static void
reverse_name(const UBYTE *addr, int len, int type, char *qbuf)
{
    struct CSource cs;

    if(type == AF_INET6 && len == 16) {
        /* each nibble in reverse */
        char *p = qbuf;
        int i;
        for(i = 15; i >= 0; i--) {
            *p++ = "0123456789abcdef"[addr[i] & 0x0f];
            *p++ = '.';
            *p++ = "0123456789abcdef"[(addr[i] >> 4) & 0x0f];
            *p++ = '.';
        }
        memcpy(p, "ip6.arpa", 9);
    } else {
        cs.CS_Buffer = qbuf;
        cs.CS_Length = MAXDNAME + 1;
        cs.CS_CurChr = 0;
        (void)csprintf(&cs, "%lu.%lu.%lu.%lu.in-addr.arpa",
                       ((unsigned long)addr[3] & 0xff),
                       ((unsigned long)addr[2] & 0xff),
                       ((unsigned long)addr[1] & 0xff),
                       ((unsigned long)addr[0] & 0xff));
    }
}

struct hostent *res_gethostbyname(const char *name, struct SocketBase *libPtr)
{
    querybuf *buf;
    int n;
    char *ptr;
    struct hoststruct *HS = NULL;
    struct hostent *anshost = NULL;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) gethostbyname('%s')\n", name));
#endif

    CHECK_TASK2();

    /*
     * A dotted quad, a localhost name: answered by the core as by its
     * vector (getaddrinfo() asks here with whatever it was given).
     */
    if(resolver_literal(libPtr, name, AF_INET, &anshost))
        return anshost;

    /*
     * Search local database (first) is usens not FIRST
     */
    if(usens != 1)
        if((anshost = _gethtbyname(libPtr, name)) != NULL || usens == 0) {
#if defined(__AROS__)
            D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: host found in local database\n"));
#endif
            return anshost;
        }
    /*
     * Here if usens is FIRST or host not in local database and usens is SECOND
     */
    if((HS = bsd_malloc(sizeof(querybuf) + sizeof(struct hoststruct),
                        M_TEMP, M_WAITOK)) == NULL) {
#if defined(__AROS__)
        D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: couldnt allocate memory for res search entry\n"));
#endif
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    buf = (querybuf *)(HS + 1);

    /*
     * Cache probe: replay a stored raw response through the very same
     * getanswer() path a fresh query would use. Any miss or parse failure
     * simply falls through to the normal resolver query below.
     */
    if(dns_cache_enabled) {
        n = dns_cache_lookup(name, T_A, buf->buf, sizeof(querybuf));
        if(n > 0) {
#if defined(__AROS__)
            D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [dns_cache] hit, %d bytes\n", n));
#endif
            ptr = getanswer(libPtr, buf, n, 0, T_A, HS);
            if(ptr != NULL &&
                    (anshost = makehostent(libPtr, HS, ptr)) != NULL) {
                anshost->h_addrtype = HS->host.h_addrtype;
                anshost->h_length = HS->host.h_length;
                bsd_free(HS, M_TEMP);
                return anshost;
            }
            anshost = NULL;     /* cached bytes unusable: do a real query */
        }
    }

    n = res_search(libPtr, name, C_IN, T_A, buf->buf, sizeof(querybuf));
    if(n >= 0) {
#if defined(__AROS__)
        D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [res_search] returns %d\n", n));
#endif
        dns_cache_insert(name, T_A, buf->buf, n);
        ptr = getanswer(libPtr, buf, n, 0, T_A, HS);
        if(ptr != NULL) {
#if defined(__AROS__)
            D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [getanswer] returns valid ptr\n"));
#endif
            if((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
#if defined(__AROS__)
                D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [makehostent] returns success\n"));
#endif
                anshost->h_addrtype = HS->host.h_addrtype;
                anshost->h_length = HS->host.h_length;
            }
        }
    } else {
#if defined(__AROS__)
        D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: [res_search] failed\n"));
#endif
#ifdef RES_DEBUG
        printf("res_search failed\n");
#endif
        /*
         * If usens is FIRST and host not found using resolver.
         */
        if(usens != 2)
            anshost = _gethtbyname(libPtr, name);
    }
#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) gethostbyname: finished search\n"));
#endif

    if(HS)
        bsd_free(HS, M_TEMP);
    return anshost;
}

struct hostent *res_gethostbyname2(const char *name, int af, struct SocketBase *libPtr)
{
    querybuf *buf;
    int n;
    char *ptr;
    struct hoststruct *HS = NULL;
    struct hostent *anshost = NULL;
    int qtype;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) gethostbyname2('%s', af=%d)\n", name, af));
#endif

    CHECK_TASK2();

    if(af == AF_INET)
        return res_gethostbyname(name, libPtr);

    /*
     * A family other than IPv6, a localhost name, an IPv6 literal (it has
     * a ':'): answered by the core, never looked up.
     */
    if(resolver_literal(libPtr, name, af, &anshost))
        return anshost;

    qtype = T_AAAA;

    /*
     * Search local database (first) if usens is not FIRST
     */
    if(usens != 1)
        if((anshost = _gethtbyname2(libPtr, name, AF_INET6)) != NULL || usens == 0)
            return anshost;

    if((HS = bsd_malloc(sizeof(querybuf) + sizeof(struct hoststruct),
                        M_TEMP, M_WAITOK)) == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    buf = (querybuf *)(HS + 1);

    /*
     * Cache probe (AAAA). A hit is replayed through getanswer(); anything
     * short of a usable answer falls through to a normal resolver query.
     */
    if(dns_cache_enabled) {
        n = dns_cache_lookup(name, qtype, buf->buf, sizeof(querybuf));
        if(n > 0) {
#if defined(__AROS__)
            D(bug("[AROSTCP](gethostnameadr.c) gethostbyname2: [dns_cache] hit, %d bytes\n", n));
#endif
            ptr = getanswer(libPtr, buf, n, 0, qtype, HS);
            if(ptr != NULL &&
                    (anshost = makehostent(libPtr, HS, ptr)) != NULL) {
                anshost->h_addrtype = HS->host.h_addrtype;
                anshost->h_length = HS->host.h_length;
                bsd_free(HS, M_TEMP);
                return anshost;
            }
            anshost = NULL;     /* cached bytes unusable: do a real query */
        }
    }

    n = res_search(libPtr, name, C_IN, qtype, buf->buf, sizeof(querybuf));
    if(n >= 0) {
#if defined(__AROS__)
        D(bug("[AROSTCP](gethostnameadr.c) gethostbyname2: [res_search] returns %d\n", n));
#endif
        dns_cache_insert(name, qtype, buf->buf, n);
        ptr = getanswer(libPtr, buf, n, 0, qtype, HS);
        if(ptr != NULL) {
            if((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
                anshost->h_addrtype = HS->host.h_addrtype;
                anshost->h_length = HS->host.h_length;
            }
        }
    } else {
#if defined(__AROS__)
        D(bug("[AROSTCP](gethostnameadr.c) gethostbyname2: [res_search] AAAA failed\n"));
#endif
    }

    if(HS)
        bsd_free(HS, M_TEMP);
    /*
     * If usens is FIRST and host not found using resolver.
     */
    if(anshost == NULL && usens != 2)
        anshost = _gethtbyname2(libPtr, name, AF_INET6);
    return anshost;
}

struct hostent *res_gethostbyaddr(UBYTE *addr, int len, int type, struct SocketBase *libPtr)
{
    querybuf *buf;
    int n;
    char *ptr;
    struct hoststruct *HS = NULL;
    char *qbuf;
    struct hostent *anshost = NULL;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) res_gethostbyaddr()\n"));
#endif

    CHECK_TASK2();

    if(type != AF_INET && type != AF_INET6)
        return ((struct hostent *) NULL);

    /*
     * Search local database (first) is usens not FIRST
     */
    if(usens != 1)
        if((anshost = _gethtbyaddr(libPtr, addr, len, type)) != NULL || usens == 0)
            return anshost;

    /*
     * Here if usens is FIRST or host not in local database and usens is SECOND
     */
    if((HS = bsd_malloc(sizeof(querybuf) + MAXDNAME + 1 +
                        sizeof(struct hoststruct),  M_TEMP, M_WAITOK))
            == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        return NULL;
    }
    buf = (querybuf *)(HS + 1);
    qbuf = (caddr_t)(buf + 1);

    reverse_name(addr, len, type, qbuf);
    /*
     * Cache probe (PTR / reverse). On a hit the stored response is run
     * through getanswer() and the very same address-injection the live path
     * performs; any failure falls through to a real res_query() below.
     */
    if(dns_cache_enabled) {
        n = dns_cache_lookup(qbuf, T_PTR, buf->buf, sizeof(querybuf));
        if(n > 0) {
#if defined(__AROS__)
            D(bug("[AROSTCP](gethostnameadr.c) res_gethostbyaddr: [dns_cache] hit, %d bytes\n", n));
#endif
            ptr = getanswer(libPtr, buf, n, 1, T_PTR, HS);
            if(ptr != NULL) {
                if(HS->h_addr_count == 1) {
                    HS->h_addr_count++;
                    bcopy(addr, ptr, len);
                    HS->h_addr_ptrs[0] = ptr;
                    ptr += len;
                } else if(HS->h_addr_count > 1) {
                    bcopy(addr, HS->h_addr_ptrs[0], len);
                }
                HS->h_addr_ptrs[1] = NULL;
                if((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
                    anshost->h_addrtype = type;
                    anshost->h_length = len;
                    bsd_free(HS, M_TEMP);
                    return anshost;
                }
            }
            anshost = NULL;     /* cached bytes unusable: do a real query */
        }
    }

    n = res_query(libPtr, qbuf, C_IN, T_PTR, (char *)buf, sizeof(querybuf));

    if(n >= 0) {
        dns_cache_insert(qbuf, T_PTR, buf->buf, n);
        ptr = getanswer(libPtr, buf, n, 1, T_PTR, HS);
        if(ptr != NULL) {
            if(HS->h_addr_count == 1) {
                HS->h_addr_count++;
                bcopy(addr, ptr, len);
                HS->h_addr_ptrs[0] = ptr;
                ptr += len;
            } else if(HS->h_addr_count > 1) {
                bcopy(addr, HS->h_addr_ptrs[0], len);
            }
            HS->h_addr_ptrs[1] = NULL;
            if((anshost = makehostent(libPtr, HS, ptr)) != NULL) {
                anshost->h_addrtype = type;
                anshost->h_length = len;
            }
        }
    } else {
#ifdef RES_DEBUG
        printf("res_query failed\n");
#endif
        /*
         * If usens is FIRST and host not found using resolver.
         */
        if(usens != 2)
            anshost = _gethtbyaddr(libPtr, addr, len, type);
    }
    if(HS)
        bsd_free(HS, M_TEMP);
    return anshost;
}

static struct hostent *makehostent(struct SocketBase *libPtr,
                                   struct hoststruct *HS,
                                   char *ptr)
{
    int n, i;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) makehostent()\n"));
#endif

    i = (caddr_t)ALIGN(ptr) - (caddr_t)&HS->hostbuf;
    n = i + sizeof(struct hostent) + HS->h_addr_count * sizeof(char *) +
        HS->host_alias_count * sizeof(char *);

    if(res_allocdatabuffer(&libPtr->hostents, n) == FALSE) {
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
    for(i = 0; HS->host_aliases[i]; i++)
        HOSTENT->h_aliases[i] = HS->host_aliases[i] + n;
    HOSTENT->h_aliases[i++] = NULL;

    HOSTENT->h_addr_list = HOSTENT->h_aliases + i;
    for(i = 0; HS->h_addr_ptrs[i]; i++)
        HOSTENT->h_addr_list[i] = HS->h_addr_ptrs[i] + n;
    HOSTENT->h_addr_list[i] = NULL;

    return HOSTENT;
}

/*
 * makehostent_r -- marshal hoststruct (from getanswer) into caller's buffer.
 * Returns 0 on success, ERANGE if buf too small.
 */
static int
makehostent_r(struct hoststruct *HS, char *ptr,
              struct hostent *result, char *buf, LONG buflen)
{
    int i, data_len, total;
    long offset;

    /* data_len = aligned size of string/address data in hostbuf */
    data_len = (caddr_t)ALIGN(ptr) - (caddr_t)&HS->hostbuf;

    /* Total buffer needed: string data + alias ptrs + addr ptrs */
    total = data_len +
            HS->host_alias_count * sizeof(char *) +
            HS->h_addr_count * sizeof(char *);

    if (total > buflen)
        return ERANGE;

    /* Copy string/address data into buf */
    bcopy(HS->hostbuf, buf, data_len);

    /* Pointer adjustment: buf base vs HS->hostbuf base */
    offset = (caddr_t)buf - (caddr_t)&HS->hostbuf;

    result->h_name = HS->host.h_name + offset;

    result->h_aliases = (char **)(buf + data_len);
    for (i = 0; HS->host_aliases[i]; i++)
        result->h_aliases[i] = HS->host_aliases[i] + offset;
    result->h_aliases[i++] = NULL;

    result->h_addr_list = result->h_aliases + i;
    for (i = 0; HS->h_addr_ptrs[i]; i++)
        result->h_addr_list[i] = HS->h_addr_ptrs[i] + offset;
    result->h_addr_list[i] = NULL;

    return 0;
}

/*
 * hostent_copyout_r -- copy a fully-formed hostent into caller's buffer.
 * Works with any struct hostent * whose pointers reference valid data
 * (e.g. from the NDB HostentNode). Returns 0 on success, ERANGE if
 * buf is too small.
 */
static int
hostent_copyout_r(const struct hostent *src, struct hostent *result,
                  char *buf, LONG buflen)
{
    char *bp = buf;
    int naliases, naddrs, i, namelen, total;
    char *alias_strs[MAXALIASES + 1];
    char *addr_data[MAXADDRS + 1];
    char **ptrp;

    /* Count aliases */
    for (naliases = 0; src->h_aliases && src->h_aliases[naliases]; naliases++)
        ;
    /* Count addresses */
    for (naddrs = 0; src->h_addr_list && src->h_addr_list[naddrs]; naddrs++)
        ;

    /* Calculate total required space */
    namelen = strnlen(src->h_name, MAXDNAME) + 1;
    total = namelen;
    for (i = 0; i < naliases; i++)
        total += strnlen(src->h_aliases[i], MAXDNAME) + 1;

    /* Align for address data */
    total = (int)ALIGN(total);
    total += naddrs * src->h_length;

    /* Align for pointer arrays */
    total = (total + sizeof(char *) - 1) & ~(sizeof(char *) - 1);
    total += (naliases + 1) * sizeof(char *);  /* alias ptrs + NULL */
    total += (naddrs + 1) * sizeof(char *);    /* addr ptrs + NULL */

    if (total > buflen)
        return ERANGE;

    /* Copy h_name */
    result->h_name = bp;
    memcpy(bp, src->h_name, namelen);
    bp[namelen - 1] = '\0';
    bp += namelen;

    /* Copy alias strings */
    for (i = 0; i < naliases; i++) {
        int slen = strnlen(src->h_aliases[i], MAXDNAME) + 1;
        alias_strs[i] = bp;
        memcpy(bp, src->h_aliases[i], slen);
        bp[slen - 1] = '\0';
        bp += slen;
    }

    /* Align for address data */
    bp = (char *)ALIGN(bp);

    /* Copy address data */
    for (i = 0; i < naddrs; i++) {
        addr_data[i] = bp;
        memcpy(bp, src->h_addr_list[i], src->h_length);
        bp += src->h_length;
    }

    /* Align for pointer arrays */
    bp = (char *)(((long)bp + sizeof(char *) - 1) & ~(sizeof(char *) - 1));

    /* Write alias pointer array */
    ptrp = (char **)bp;
    result->h_aliases = ptrp;
    for (i = 0; i < naliases; i++)
        ptrp[i] = alias_strs[i];
    ptrp[naliases] = NULL;
    bp += (naliases + 1) * sizeof(char *);

    /* Write address pointer array */
    ptrp = (char **)bp;
    result->h_addr_list = ptrp;
    for (i = 0; i < naddrs; i++)
        ptrp[i] = addr_data[i];
    ptrp[naddrs] = NULL;

    result->h_addrtype = src->h_addrtype;
    result->h_length = src->h_length;

    return 0;
}

/*
 * _gethtbyname_r -- search local host database, copy result to caller's buf.
 * Returns 0 on success, HOST_NOT_FOUND if not found, ERANGE if buf too small.
 * Must be called without NDB lock held.
 */
static int
_gethtbyname_r(struct SocketBase *libPtr, const char *name,
               struct hostent *result, char *buf, LONG buflen)
{
    struct HostentNode *entNode;
    int rc = HOST_NOT_FOUND;

    LOCK_R_NDB(NDB);
    for (entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
         entNode->hn_Node.mln_Succ;
         entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ) {
        if (entNode->hn_Ent.h_addrtype != AF_INET)
            continue;   /* the table also holds IPv6 entries */
        if (res_strcasecmp(entNode->hn_Ent.h_name, name) == 0) {
            rc = hostent_copyout_r(&entNode->hn_Ent, result, buf, buflen);
            break;
        }
        /* Check aliases */
        if (entNode->hn_Ent.h_aliases) {
            char **ap;
            for (ap = entNode->hn_Ent.h_aliases; *ap; ap++) {
                if (res_strcasecmp(*ap, name) == 0) {
                    rc = hostent_copyout_r(&entNode->hn_Ent, result,
                                           buf, buflen);
                    goto done;
                }
            }
        }
    }
done:
    UNLOCK_NDB(NDB);
    return rc;
}

/*
 * _gethtbyaddr_r -- search local host database by address, copy to caller's buf.
 * Returns 0 on success, HOST_NOT_FOUND if not found, ERANGE if buf too small.
 */
static int
_gethtbyaddr_r(struct SocketBase *libPtr, const char *addr, int len, int type,
               struct hostent *result, char *buf, LONG buflen)
{
    struct HostentNode *entNode;
    int rc = HOST_NOT_FOUND;

    LOCK_R_NDB(NDB);
    for (entNode = (struct HostentNode *)NDB->ndb_Hosts.mlh_Head;
         entNode->hn_Node.mln_Succ;
         entNode = (struct HostentNode *)entNode->hn_Node.mln_Succ) {
        if (entNode->hn_Ent.h_addrtype == type &&
            !bcmp(entNode->hn_Ent.h_addr, addr, len)) {
            rc = hostent_copyout_r(&entNode->hn_Ent, result, buf, buflen);
            break;
        }
    }
    UNLOCK_NDB(NDB);
    return rc;
}

/*
 * res_gethostbyname_r -- thread-safe gethostbyname.
 * Returns result on success, NULL on failure.
 * Sets *h_errnop on DNS errors. Sets errno via writeErrnoValue.
 * Its vector (api/resolver_stubs.c) has checked the arguments and answered
 * a dotted quad and a localhost name itself: here is a name to look up.
 */
struct hostent *
res_gethostbyname_r(const char *name, struct hostent *result,
                  char *buf, LONG buflen, LONG *h_errnop,
                  struct SocketBase *libPtr)
{
    querybuf *qbuf;
    int n, rc;
    char *ptr;
    struct hoststruct *HS = NULL;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) res_gethostbyname_r('%s')\n", name));
#endif

    /*
     * Search local database (first if usens != FIRST)
     */
    if (usens != 1) {
        rc = _gethtbyname_r(libPtr, name, result, buf, buflen);
        if (rc == 0) {
            if (h_errnop) *h_errnop = 0;
            return result;
        }
        if (rc == ERANGE) {
            writeErrnoValue(libPtr, ERANGE);
            if (h_errnop) *h_errnop = NETDB_INTERNAL;
            return NULL;
        }
        if (usens == 0) {
            if (h_errnop) *h_errnop = HOST_NOT_FOUND;
            return NULL;
        }
    }

    /*
     * DNS lookup
     */
    if ((HS = bsd_malloc(sizeof(querybuf) + sizeof(struct hoststruct),
                         M_TEMP, M_WAITOK)) == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        if (h_errnop) *h_errnop = NETDB_INTERNAL;
        return NULL;
    }
    qbuf = (querybuf *)(HS + 1);

    n = res_search(libPtr, name, C_IN, T_A, qbuf->buf, sizeof(querybuf));
    if (n >= 0) {
        ptr = getanswer(libPtr, qbuf, n, 0, T_A, HS);
        if (ptr != NULL) {
            rc = makehostent_r(HS, ptr, result, buf, buflen);
            if (rc == 0) {
                result->h_addrtype = HS->host.h_addrtype;
                result->h_length = HS->host.h_length;
                bsd_free(HS, M_TEMP);
                if (h_errnop) *h_errnop = 0;
                return result;
            }
            if (rc == ERANGE) {
                writeErrnoValue(libPtr, ERANGE);
                if (h_errnop) *h_errnop = NETDB_INTERNAL;
                bsd_free(HS, M_TEMP);
                return NULL;
            }
        }
    } else {
        /* DNS failed — try local DB if usens is FIRST */
        if (usens != 2) {
            rc = _gethtbyname_r(libPtr, name, result, buf, buflen);
            if (rc == 0) {
                bsd_free(HS, M_TEMP);
                if (h_errnop) *h_errnop = 0;
                return result;
            }
            if (rc == ERANGE) {
                writeErrnoValue(libPtr, ERANGE);
                if (h_errnop) *h_errnop = NETDB_INTERNAL;
                bsd_free(HS, M_TEMP);
                return NULL;
            }
        }
    }

    /* Not found */
    if (h_errnop) *h_errnop = h_errno;
    bsd_free(HS, M_TEMP);
    return NULL;
}

/*
 * res_gethostbyaddr_r -- thread-safe gethostbyaddr.
 * Returns result on success, NULL on failure.
 */
struct hostent *
res_gethostbyaddr_r(const char *addr, LONG len, LONG type,
                  struct hostent *result, char *buf, LONG buflen,
                  LONG *h_errnop, struct SocketBase *libPtr)
{
    querybuf *qb;
    int n, rc;
    char *ptr;
    struct hoststruct *HS = NULL;
    char *revname;

#if defined(__AROS__)
    D(bug("[AROSTCP](gethostnameadr.c) res_gethostbyaddr_r()\n"));
#endif

    if (!addr || !result || !buf || buflen <= 0) {
        writeErrnoValue(libPtr, EINVAL);
        if (h_errnop) *h_errnop = NETDB_INTERNAL;
        return NULL;
    }

    if (type != AF_INET && type != AF_INET6) {
        if (h_errnop) *h_errnop = HOST_NOT_FOUND;
        return NULL;
    }

    /*
     * Search local database first (if usens != FIRST)
     */
    if (usens != 1) {
        rc = _gethtbyaddr_r(libPtr, addr, len, type, result, buf, buflen);
        if (rc == 0) {
            if (h_errnop) *h_errnop = 0;
            return result;
        }
        if (rc == ERANGE) {
            writeErrnoValue(libPtr, ERANGE);
            if (h_errnop) *h_errnop = NETDB_INTERNAL;
            return NULL;
        }
        if (usens == 0) {
            if (h_errnop) *h_errnop = HOST_NOT_FOUND;
            return NULL;
        }
    }

    /*
     * DNS reverse lookup
     */
    if ((HS = bsd_malloc(sizeof(querybuf) + MAXDNAME + 1 +
                         sizeof(struct hoststruct), M_TEMP, M_WAITOK))
        == NULL) {
        writeErrnoValue(libPtr, ENOMEM);
        if (h_errnop) *h_errnop = NETDB_INTERNAL;
        return NULL;
    }
    qb = (querybuf *)(HS + 1);
    revname = (caddr_t)(qb + 1);

    reverse_name((const UBYTE *)addr, len, type, revname);

    n = res_query(libPtr, revname, C_IN, T_PTR, (char *)qb, sizeof(querybuf));
    if (n >= 0) {
        ptr = getanswer(libPtr, qb, n, 1, T_PTR, HS);
        if (ptr != NULL) {
            /* For PTR lookups, getanswer may not set address data.
             * Inject the queried address into the hoststruct. */
            if (HS->h_addr_count == 1) {
                HS->h_addr_count++;
                bcopy(addr, ptr, len);
                HS->h_addr_ptrs[0] = ptr;
                ptr += len;
            } else if (HS->h_addr_count > 1) {
                /* Address slot(s) already exist — overwrite first */
                bcopy(addr, HS->h_addr_ptrs[0], len);
            }
            HS->h_addr_ptrs[1] = NULL;

            rc = makehostent_r(HS, ptr, result, buf, buflen);
            if (rc == 0) {
                result->h_addrtype = type;
                result->h_length = len;
                bsd_free(HS, M_TEMP);
                if (h_errnop) *h_errnop = 0;
                return result;
            }
            if (rc == ERANGE) {
                writeErrnoValue(libPtr, ERANGE);
                if (h_errnop) *h_errnop = NETDB_INTERNAL;
                bsd_free(HS, M_TEMP);
                return NULL;
            }
        }
    } else {
        /* DNS failed — try local DB if usens is FIRST */
        if (usens != 2) {
            rc = _gethtbyaddr_r(libPtr, addr, len, type, result, buf, buflen);
            if (rc == 0) {
                bsd_free(HS, M_TEMP);
                if (h_errnop) *h_errnop = 0;
                return result;
            }
            if (rc == ERANGE) {
                writeErrnoValue(libPtr, ERANGE);
                if (h_errnop) *h_errnop = NETDB_INTERNAL;
                bsd_free(HS, M_TEMP);
                return NULL;
            }
        }
    }

    /* Not found */
    if (h_errnop) *h_errnop = h_errno;
    bsd_free(HS, M_TEMP);
    return NULL;
}
