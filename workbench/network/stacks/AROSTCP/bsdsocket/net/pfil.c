/*	$FreeBSD: src/sys/net/pfil.c,v 1.8.4.4 2005/01/31 23:26:23 imp Exp $ */
/*	$NetBSD: pfil.c,v 1.20 2001/11/12 23:49:46 lukem Exp $	*/

/*-
 * Copyright (c) 1996 Matthew R. Green
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

#include <conf.h>

#include <aros/asmcall.h>
#include <exec/lists.h>
#include <libraries/miami.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/pfil.h>

struct SignalSemaphore pfil_list_lock;
struct MinList pfil_list;
              
static __inline void
PFIL_RLOCK(void)
{
	ObtainSemaphoreShared(&pfil_list_lock);
}

static __inline void
PFIL_RUNLOCK(void)
{
	ReleaseSemaphore(&pfil_list_lock);
}

void pfil_init(void)
{
	InitSemaphore(&pfil_list_lock);
	NewList((struct List *)&pfil_list);
}

/*
 * pfil_run_hooks() runs the specified packet filter hooks.
 */
void
pfil_run_hooks(struct mbuf *m, struct ifnet *ifp, unsigned char pr)
{
	struct packet_filter_hook *pfh;
	struct MiamiPFBuffer pfb;
	unsigned char ifname[IFNAMSIZ+3];
	void (*pfil_func)(struct Hook *, APTR, struct MiamiPFBuffer *);

	DPF(kprintf("pfil_run_hooks(0x%08lx, %s%u, %u) called\n", ifp, ifp->if_name, ifp->if_unit, pr);)
	pfb.data = mtod(m, unsigned char *);
	pfb.length = m->m_len;
	sprintf(ifname, "%s%u", ifp->if_name, ifp->if_unit);
	pfb.name = ifname;
	if (ifp->if_flags & IFF_LOOPBACK)
		pfb.itype = MIAMIPFBIT_LOOP;
	else
		pfb.itype = MIAMIPFBIT_BUILTIN;
	pfb.ptype = pr;
	PFIL_RLOCK();
	for (pfh = (struct packet_filter_hook *)pfil_list.mlh_Head;
	     pfh->pfil_link.mln_Succ;
	     pfh = (struct packet_filter_hook *)pfh->pfil_link.mln_Succ) {
/* TODO: NicJA - Where is CHECK_POINTER() !!!! */
#if !defined(__AROS__)
	     CHECK_POINTER(pfh);
#endif
	     DPF(kprintf("Checking handle 0x%08lx\n", pfh);)
	     DPF(kprintf("Interface: 0x%08lx\n", pfh->pfil_if);)
	     DPF(kprintf("Hook: 0x%08lx\n", pfh->pfil_hook);)
	     DPF(kprintf("Function: 0x%08lx\n", pfh->pfil_hook->h_Entry);)
	     DPF(kprintf("CPU type: %ld\n", pfh->pfil_hooktype);)
	     if (pfh->pfil_if == ifp) {
		pfil_func = (APTR)pfh->pfil_hook->h_Entry;
		DPF(kprintf("Executing packet filter routine: 0x%08lx\n", pfil_func);)
		switch (pfh->pfil_hooktype) {
		case MIAMICPU_M68KREG:
			AROS_UFC3NR(void, pfil_func,
				AROS_UFCA(struct Hook *, pfh->pfil_hook, A0),
				AROS_UFCA(APTR, pfh, A2),
				AROS_UFCA(struct MiamiPFBuffer *, &pfb, A1));
			break;
		case MIAMICPU_PPCV4:
			pfil_func(pfh->pfil_hook, pfh, &pfb);
			break;
		}
	     }
	}
	PFIL_RUNLOCK();
}
    
