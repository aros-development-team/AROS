/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
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

/*-
 * Copyright (c) 1982, 1988, 1991 The Regents of the University of California.
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
 *	@(#)systm.h	7.17 (Berkeley) 5/25/91
 */

#ifndef SYS_SYSTM_H
#define SYS_SYSTM_H

#ifndef SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef SYS_CDEFS_H
#include <sys/cdefs.h>		/* for the inlines */
#endif

#include <stdarg.h>

#include <dos/rdargs.h>

/*
 * queue node definition for _insque and _remque. _insque and _remque expect
 * to find this structure from the start of every node they handle.
 */

struct queue_node {
  struct queue_node *next;
  struct queue_node *prev;
};

/* casts to keep lint happy */
#define	insque(q,p)	_insque((struct queue_node *)q,(struct queue_node *)p)

static inline void 
_insque(register struct queue_node *node, register struct queue_node *pred)
{
  node->next = pred->next;
  node->prev = pred;
  (pred->next)->prev = node;
  pred->next = node;
}

#define	remque(q)	_remque((struct queue_node *)q)

static inline struct queue_node *
_remque(register struct queue_node *node)
{
  (node->next)->prev = node->prev;
  (node->prev)->next = node->next;
  return(node);
}

/*
 * General function declarations.
 */

void cs_putchar(unsigned char, struct CSource *);
void panic(const char *, ...);
void __log(unsigned long, const char *, ...);
int vlog(unsigned long, const char *, const char *, va_list);
int printf(const char *, ...);
int sprintf(char * buf, const char * fmt, ...);
int csprintf(struct CSource* buf, const char * fmt, ...);
int vsprintf(char * buf, const char * fmt, va_list);
int vcsprintf(struct CSource* buf, const char * fmt, va_list);
char * csprintn(u_long n, int base, char *buf, int buflen);
int  ultoa(unsigned long ul, char *buffer);
int  ltoa(long l, char *buffer);

#define itoa(i,buffer) ltoa((long)i,buffer)

#ifndef AMIGA_SUBR_H
#include <kern/amiga_subr.h>
#endif

#endif /* !SYS_SYSTM_H */
