/*	$FreeBSD: src/sys/net/pfil.c,v 1.8.4.4 2005/01/31 23:26:23 imp Exp $ */
/*	$NetBSD: pfil.c,v 1.20 2001/11/12 23:49:46 lukem Exp $	*/

/*-
 * Copyright (C) 1996 Matthew R. Green
 * Copyright (C) 2005-2026 The AROS Dev Team
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

#ifdef ENABLE_PACKET_FILTER

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
#include <sys/synch.h>

#include <sys/mbuf.h>

#include <net/if.h>
#include <net/pfil.h>

/*
 * THE MIAMI HOOKS (MiamiPFAddHook(), api/miami_api.c): their list, and how
 * many are on it - in all, and of them those that asked for IPv6 packets
 * too. Changed with pfil_list_lock held exclusively; run under it shared.
 */
struct SignalSemaphore pfil_list_lock;
struct MinList pfil_list;
volatile LONG pfil_hooks = 0;
static LONG pfil_hooks6 = 0;

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

/*
 * THE HOOKS OF EACH PACKET TYPE, and the point of the family that owns the
 * type while it runs (net/pfil.h). Indexed by MIAMIPFBPT_*.
 */
static struct pfil_type {
    struct pfil_point *pt_point;
    struct pfil_hook  *pt_hooks;
} pfil_types[PFIL_TYPES];

/* whether the Miami hooks are to see packets of the type */
static __inline int
pfil_miami_wants(int type)
{
    return (type == MIAMIPFBPT_IP6 ? pfil_hooks6 : pfil_hooks) != 0;
}

/*
 * The point's word and hook list from what is hooked now. With
 * pfil_list_lock held exclusively: a Miami hook is added and removed by
 * an application's task without the fence, the point's hooks by a
 * module's start or stop or a fenced request (at splnet as well).
 */
static void
pfil_update(int type)
{
    struct pfil_point *pp = pfil_types[type].pt_point;
    struct pfil_hook *ph;
    unsigned short active = 0;

    if(pp == NULL)
        return;
    for(ph = pfil_types[type].pt_hooks; ph != NULL; ph = ph->ph_next)
        active |= ph->ph_dirs & (PFIL_IN | PFIL_OUT | PFIL_FWD);
    if(pfil_miami_wants(type))
        active |= PFIL_IN | PFIL_OUT | PFIL_MIAMI;
    pp->pp_hooks = pfil_types[type].pt_hooks;
    pp->pp_active = active;
}

/*
 * The lock and the Miami list are set up by whichever comes first: the
 * families register their points while the domains are initialised,
 * before the stack's start calls pfil_init(). Both happen in the stack's
 * task before any application can add a Miami hook.
 */
static int pfil_ready = 0;

static void
pfil_setup(void)
{
    if(!pfil_ready) {
        InitSemaphore(&pfil_list_lock);
        NewList((struct List *)&pfil_list);
        pfil_ready = 1;
    }
}

void
pfil_init(void)
{
    pfil_setup();
}

/*
 * THE FAMILIES' POINTS.
 */
int
pfil_point_register(struct pfil_point *pp)
{
    int type;
    spl_t s;

    if(pp == NULL || (type = pp->pp_type) >= PFIL_TYPES)
        return (EINVAL);
    if(pfil_types[type].pt_point != NULL)
        return (pfil_types[type].pt_point == pp ? 0 : EEXIST);
    pfil_setup();
    s = splnet();
    ObtainSemaphore(&pfil_list_lock);
    pfil_types[type].pt_point = pp;
    pfil_update(type);
    ReleaseSemaphore(&pfil_list_lock);
    splx(s);
    return (0);
}

/* the hooks stay the core's: they are on the point again when it is registered again */
int
pfil_point_unregister(struct pfil_point *pp)
{
    int type;
    spl_t s;

    if(pp == NULL || (type = pp->pp_type) >= PFIL_TYPES || pfil_types[type].pt_point != pp)
        return (ENOENT);
    s = splnet();
    ObtainSemaphore(&pfil_list_lock);
    pp->pp_active = 0;
    pp->pp_hooks = NULL;
    pfil_types[type].pt_point = NULL;
    ReleaseSemaphore(&pfil_list_lock);
    splx(s);
    return (0);
}

struct pfil_point *
pfil_point_find(int type)
{
    return ((unsigned)type < PFIL_TYPES ? pfil_types[type].pt_point : NULL);
}

/*
 * THE HOOKS. Kept sorted by ph_pri; one of equal priority goes behind
 * those already there.
 */
int
pfil_hook_add(int type, struct pfil_hook *ph)
{
    struct pfil_hook **pp, *h;
    spl_t s;

    if((unsigned)type >= PFIL_TYPES || ph == NULL || ph->ph_func == NULL ||
            (ph->ph_dirs & (PFIL_IN | PFIL_OUT | PFIL_FWD)) == 0)
        return (EINVAL);
    for(h = pfil_types[type].pt_hooks; h != NULL; h = h->ph_next)
        if(h == ph)
            return (EEXIST);
    pfil_setup();
    s = splnet();
    ObtainSemaphore(&pfil_list_lock);
    for(pp = &pfil_types[type].pt_hooks; *pp != NULL && (*pp)->ph_pri <= ph->ph_pri;
            pp = &(*pp)->ph_next)
        ;
    ph->ph_next = *pp;
    *pp = ph;
    pfil_update(type);
    ReleaseSemaphore(&pfil_list_lock);
    splx(s);
    return (0);
}

int
pfil_hook_remove(int type, struct pfil_hook *ph)
{
    struct pfil_hook **pp;
    spl_t s;

    if((unsigned)type >= PFIL_TYPES || ph == NULL)
        return (EINVAL);
    for(pp = &pfil_types[type].pt_hooks; *pp != NULL; pp = &(*pp)->ph_next)
        if(*pp == ph)
            break;
    if(*pp == NULL)
        return (ENOENT);
    s = splnet();
    ObtainSemaphore(&pfil_list_lock);
    *pp = ph->ph_next;
    ph->ph_next = NULL;
    pfil_update(type);
    ReleaseSemaphore(&pfil_list_lock);
    splx(s);
    return (0);
}

/*
 * THE MIAMI HOOKS, added and removed by an application's task
 * (api/miami_api.c).
 */
void
pfil_miami_add(struct packet_filter_hook *pfh)
{
    int i;

    ObtainSemaphore(&pfil_list_lock);
    AddTail((struct List *)&pfil_list, (struct Node *)pfh);
    pfil_hooks++;
    if(pfh->pfil_ip6)
        pfil_hooks6++;
    for(i = 0; i < PFIL_TYPES; i++)
        pfil_update(i);
    ReleaseSemaphore(&pfil_list_lock);
}

void
pfil_miami_remove(struct packet_filter_hook *pfh)
{
    int i;

    ObtainSemaphore(&pfil_list_lock);
    Remove((struct Node *)pfh);
    pfil_hooks--;
    if(pfh->pfil_ip6)
        pfil_hooks6--;
    for(i = 0; i < PFIL_TYPES; i++)
        pfil_update(i);
    ReleaseSemaphore(&pfil_list_lock);
}

/*
 * The Miami hooks of the packet's interface, each with the packet's first
 * mbuf, as they always were called. They cannot drop the packet.
 */
static void
pfil_miami_run(struct mbuf *m, struct ifnet *ifp, unsigned char type)
{
    struct packet_filter_hook *pfh;
    struct MiamiPFBuffer pfb;
    unsigned char ifname[IFNAMSIZ + 3];
    void (*pfil_func)(struct Hook *, APTR, struct MiamiPFBuffer *);

    pfb.flags = 0;
    pfb.data = mtod(m, unsigned char *);
    pfb.length = m->m_len;
    if_fullname(ifp, (char *)ifname, sizeof(ifname));
    pfb.name = ifname;
    if(ifp->if_flags & IFF_LOOPBACK)
        pfb.itype = MIAMIPFBIT_LOOP;
    else
        pfb.itype = MIAMIPFBIT_BUILTIN;
    pfb.ptype = type;
    PFIL_RLOCK();
    for(pfh = (struct packet_filter_hook *)pfil_list.mlh_Head;
            pfh->pfil_link.mln_Succ;
            pfh = (struct packet_filter_hook *)pfh->pfil_link.mln_Succ) {
        DPF(kprintf("Checking handle 0x%p\n", pfh);)
        if(pfh->pfil_if != ifp || (type == MIAMIPFBPT_IP6 && !pfh->pfil_ip6))
            continue;
        pfil_func = (APTR)pfh->pfil_hook->h_Entry;
        DPF(kprintf("Executing packet filter routine: 0x%p\n", pfil_func);)
        switch(pfh->pfil_hooktype) {
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
    PFIL_RUNLOCK();
}

/*
 * The point's hooks for the direction, in order, then the Miami hooks.
 * The packet is not freed here.
 */
static int
pfil_dorun(struct pfil_point *pp, struct mbuf **mp, struct ifnet *ifp, int dir)
{
    struct pfil_hook *ph;
    int error;

    for(ph = pp->pp_hooks; ph != NULL; ph = ph->ph_next) {
        if(!(ph->ph_dirs & dir))
            continue;
        if((error = (*ph->ph_func)(ph->ph_arg, mp, ifp, dir)) != 0)
            return (error);
        if(*mp == NULL)
            return (ENOBUFS);       /* a hook that took the packet must say so */
    }
    if((pp->pp_active & PFIL_MIAMI) && (dir & (PFIL_IN | PFIL_OUT)))
        pfil_miami_run(*mp, ifp, pp->pp_type);
    return (0);
}

int
pfil_run(struct pfil_point *pp, struct mbuf **mp, struct ifnet *ifp, int dir)
{
    int error;

    DPF(kprintf("pfil_run(0x%p, %s%u, %u, %d) called\n", *mp, ifp->if_name, ifp->if_unit,
                pp->pp_type, dir);)
    if((error = pfil_dorun(pp, mp, ifp, dir)) != 0 && *mp != NULL) {
        m_freem(*mp);
        *mp = NULL;
    }
    return (error);
}

/*
 * The hooks of the type for one packet, as the services table's entry of
 * the same name always did it: non-zero means the packet is to be dropped
 * and the caller frees it. (The filter engine never frees a packet it
 * drops, so the caller's pointer stays good.)
 */
int
pfil_run_hooks(struct mbuf *m, struct ifnet *ifp, unsigned char pr, int dir)
{
    struct pfil_point *pp;

    if(pr >= PFIL_TYPES || (pp = pfil_types[pr].pt_point) == NULL ||
            !(pp->pp_active & dir))
        return 0;
    return (pfil_dorun(pp, &m, ifp, dir) != 0);
}

#endif /* ENABLE_PACKET_FILTER */
