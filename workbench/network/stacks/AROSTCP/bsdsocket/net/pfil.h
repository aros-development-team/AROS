/*	$FreeBSD: src/sys/net/pfil.h,v 1.11.2.2 2005/01/31 23:26:23 imp Exp $ */
/*	$NetBSD: pfil.h,v 1.22 2003/06/23 12:57:08 martin Exp $	*/

/*-
 * Copyright (c) 1996 Matthew R. Green
 * Copyright (c) 2005 - 2006 Pavel Fedin
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NET_PFIL_H_
#define _NET_PFIL_H_

#include <sys/systm.h>
#include <sys/queue.h>

/*
 * The packet filter hooks are designed for anything to call them to
 * possibly intercept the packet.
 */
extern struct MinList pfil_list;
extern struct SignalSemaphore pfil_list_lock;

struct packet_filter_hook {
	struct MinNode pfil_link;
	struct ifnet *pfil_if;
	struct Hook *pfil_hook;
	ULONG pfil_hooktype;
};


void	pfil_init(void);
void	pfil_run_hooks(struct mbuf *, struct ifnet *, unsigned char);
                            
#endif /* _NET_PFIL_H_ */

