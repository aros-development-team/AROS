/*	$FreeBSD: src/sys/net/pfil.h,v 1.11.2.2 2005/01/31 23:26:23 imp Exp $ */
/*	$NetBSD: pfil.h,v 1.22 2003/06/23 12:57:08 martin Exp $	*/

/*-
 * Copyright (C) 1996 Matthew R. Green
 * Copyright (C) 2005-2006 Pavel Fedin
 * Copyright (C) 2005-2026, The AROS Development Team.
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

/*
 * PACKET HOOK POINTS (net/pfil.c).
 *
 * A family - IPv4, IPv6 - owns a hook point for each kind of packet it
 * handles (struct pfil_point: static data of the family's library) and
 * registers it with the core when it starts. At the places where a packet
 * may be looked at - in, out, being forwarded - the family tests one word
 * of its own point and calls pfil_run() only when something is hooked for
 * that direction:
 *
 *      if(PFIL_HOOKED(&ip_pfil, PFIL_IN))
 *          if(pfil_run(&ip_pfil, &m, ifp, PFIL_IN) != 0)
 *              goto next;              (the packet is gone)
 *
 * So a packet costs one test and no call while nothing is hooked.
 *
 * Whoever wants to see packets (the filter engine, a module of its own)
 * hooks a packet TYPE, not a point (struct pfil_hook: static data of the
 * hooking module): the core keeps the hooks of each type and hands them to
 * the type's point while the family that owns it runs. A family that stops
 * takes its point back whatever is hooked - the hooks wait in the core and
 * are on the point again when the family starts again - so a hook never
 * keeps a family from stopping, and a family started after the hook was
 * added is hooked too.
 *
 * The hooks added with MiamiPFAddHook() (miami.library) are the core's
 * too and stay as they were: PFIL_MIAMI in a point says some exist for its
 * type, and pfil_run() then calls them after the point's own hooks, as
 * pfil_run_hooks() did for IPv4 and ARP. An IPv6 packet goes only to a
 * Miami hook that asked for IPv6 with MIAMITAG_IPV6 (libraries/miami.h):
 * a hook written for IPv4 and ARP is never shown a packet it would
 * misread.
 *
 * The points and the hooks are changed at splnet - the level every
 * packet path runs at - with pfil_list_lock held; the Miami hooks (added
 * from an application's task) under pfil_list_lock alone, which pfil_run()
 * holds shared while it calls them. pp_active is one word, written whole.
 * Adding or removing a hook is for a module's start or stop, or a request
 * made with the socket calls fenced (the filter's enable).
 */

#include <exec/types.h>
#include <libraries/miami.h>        /* MIAMIPFBPT_*: the packet types */

#ifndef MIAMIPFBPT_IP6
#define MIAMIPFBPT_IP6  2
#endif

/* directions; PFIL_IN and PFIL_OUT have the values of IPF_IN and IPF_OUT */
#define PFIL_IN     0x01
#define PFIL_OUT    0x02
#define PFIL_FWD    0x04        /* being forwarded: the route is known, before output */
#define PFIL_MIAMI  0x80        /* in pp_active: a MiamiPFAddHook() hook exists for the type */

/*
 * The test at a packet site: whether anything is hooked on the point for
 * the direction(s). One word read; the call that follows is the rare case,
 * and the compiler is told so (it keeps the call's set-up off the path of
 * the packets that pass).
 */
#define PFIL_HOOKED(pp, dirs)   __builtin_expect(((pp)->pp_active & (dirs)) != 0, 0)

/* the packet types the core keeps hooks for: MIAMIPFBPT_IP, _ARP, _IP6 */
#define PFIL_TYPES  3

struct mbuf;
struct ifnet;
struct netmodule;

/*
 * A hook. Called at the level of the packet path (splnet, or the stack's
 * task in input), with the packet whole: 0 lets it go on (*mp may have
 * been replaced by the hook - a pull-up, a translation); anything else is
 * an errno and the packet is not to go on: the hook may free it and set
 * *mp to NULL, else pfil_run() frees it.
 */
struct pfil_hook {
    struct pfil_hook *ph_next;          /* owned by the core */
    int             (*ph_func)(void *arg, struct mbuf **mp, struct ifnet *ifp, int dir);
    void             *ph_arg;
    unsigned char     ph_dirs;          /* PFIL_IN | PFIL_OUT | PFIL_FWD */
    signed char       ph_pri;           /* lower runs first */
    struct netmodule *ph_owner;
};

/* A family's point for one packet type. */
struct pfil_point {
    unsigned char     pp_type;          /* MIAMIPFBPT_IP, _ARP, _IP6 */
    unsigned char     pp_af;            /* AF_INET, AF_INET6; 0 for ARP */
    volatile unsigned short pp_active;  /* PFIL_* with something to call: the word
                                           tested for every packet; set by the core */
    struct pfil_hook *pp_hooks;         /* set by the core: the type's hooks, by ph_pri */
    struct netmodule *pp_owner;         /* the family */
};

#ifdef ENABLE_PACKET_FILTER

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <sys/systm.h>
#include <sys/queue.h>

/*
 * The family, in its start and stop. register: EINVAL (no such type),
 * EEXIST (another point of the type is registered). unregister: ENOENT.
 */
int     pfil_point_register(struct pfil_point *pp);
int     pfil_point_unregister(struct pfil_point *pp);
/* the point of a type while its family runs, else NULL */
struct pfil_point *pfil_point_find(int type);

/*
 * A hooking module, in its start and stop: EINVAL, EEXIST (on the type
 * already); ENOENT (not on it).
 */
int     pfil_hook_add(int type, struct pfil_hook *ph);
int     pfil_hook_remove(int type, struct pfil_hook *ph);

/*
 * The family's packet path, only when the direction's bit is in pp_active.
 * 0: the packet goes on, as *mp (which a hook may have replaced); else an
 * errno and the packet is freed: *mp is NULL.
 */
int     pfil_run(struct pfil_point *pp, struct mbuf **mp, struct ifnet *ifp, int dir);

/*
 * The Miami hooks (api/miami_api.c, MiamiPFAddHook() and
 * MiamiPFRemoveHook()): the core's list, with the hooks' count.
 */
extern struct MinList pfil_list;
extern struct SignalSemaphore pfil_list_lock;
extern volatile LONG pfil_hooks;

struct packet_filter_hook {
    struct MinNode pfil_link;
    struct ifnet *pfil_if;
    struct Hook *pfil_hook;
    ULONG pfil_hooktype;
    UBYTE pfil_ip6;             /* asked for IPv6 packets too (MIAMITAG_IPV6) */
};

void    pfil_miami_add(struct packet_filter_hook *pfh);
void    pfil_miami_remove(struct packet_filter_hook *pfh);

void    pfil_init(void);

/*
 * The hooks of a type, run for one packet without the per-family point
 * test - the entry of the services table that protocol modules built
 * before the points used (net/netcore.def keeps every line). Non-zero:
 * drop the packet, which the caller frees.
 */
int     pfil_run_hooks(struct mbuf *, struct ifnet *, unsigned char, int);

#else /* !ENABLE_PACKET_FILTER */

#define pfil_init()                          do { } while (0)
#define pfil_run_hooks(m, ifp, pr, dir)      0
#define pfil_point_register(pp)              0
#define pfil_point_unregister(pp)            0
#define pfil_point_find(type)                ((struct pfil_point *)0)
#define pfil_run(pp, mp, ifp, dir)           0

#endif /* ENABLE_PACKET_FILTER */

#endif /* _NET_PFIL_H_ */
