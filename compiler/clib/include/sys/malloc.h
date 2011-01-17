/*
 * Copyright (c) 1987 Regents of the University of California.
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
 *	@(#)malloc.h	7.25 (Berkeley) 5/15/91
 */

#ifndef SYS_MALLOC_H
#define	SYS_MALLOC_H

/*
 * flags to malloc
 */
#define	M_WAITOK	0x0000
#define	M_NOWAIT	0x0001

/*
 * Types of memory to be allocated
 */
#define	M_FREE		0	/* should be on free list */
#define	M_MBUF		1	/* mbuf */
#define	M_SOCKET	2	/* socket structure */
#define	M_PCB		3	/* protocol control block */
#define	M_RTABLE	4	/* routing tables */
#define	M_HTABLE	5	/* IMP host tables */
#define	M_FTABLE	6	/* fragment reassembly header */
#define	M_IFADDR	7	/* interface address */
#define	M_SOOPTS	8 	/* socket options */
#define	M_SONAME	9 	/* socket name */
#define	M_IOCTLOPS	10	/* ioctl data buffer */
#define	M_FILEDESC	11	/* Open file descriptor table */
#define	M_TEMP		12	/* misc temporary data buffers */
#define	M_IFNET		13	/* network interface structure */
#define M_CFGVAR 	14	/* configureable variable */
#define M_NETDB  	15	/* netdb node */
#define M_ARPENT        16	/* ARP entry */
#define	M_LAST		17

#ifdef KERNEL

/*
 * Use malloc & free of the standard library.
 *
 * NOTE: Because these are called from different tasks concurrently, these
 * have to be protected with a mutex.
 */
#ifndef AMIGA_INCLUDES_H
#include <kern/amiga_includes.h>
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

/*
 * prototype for the initialization function
 */
BOOL malloc_init(void);

/*
 * prototypes for BSD malloc wrapper functions.
 */

#ifndef USE_MALLOC_TYPE_TRACKING
/*
 * These macros are used to prevent unnecessary passing of arguments, which 
 * are currently not used.
 */
#define bsd_malloc(size, type, flags) bsd_malloc(size)
#define bsd_free(addr, type) bsd_free(addr)
#define bsd_realloc(mem, size, type, flags) bsd_realloc(mem, size)
#endif

void *	bsd_malloc(unsigned long size, int type, int flags);
void	bsd_free(void *addr, int type);
void *	bsd_realloc(void * mem, unsigned long size, int type, int flags);

/*
 * Macro versions for the usual cases of malloc/free
 */
#define	MALLOC(space, cast, size, type, flags) \
	(space) = (cast)bsd_malloc((u_long)(size), type, flags)
#define FREE(addr, type) bsd_free((caddr_t)(addr), type)

#endif /* KERNEL */
#endif /* !SYS_MALLOC_H */
