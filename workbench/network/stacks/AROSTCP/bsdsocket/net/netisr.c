/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005 Neil Cafferkey
 * Copyright (C) 2005 - 2026 The AROS Dev Team
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

#include <conf.h>

#include <sys/param.h>
#include <sys/errno.h>
#include <kern/amiga_includes.h>	/* This is needed by sys/synch.h */
#include <sys/synch.h>

#include <net/netisr.h>
#include <net/netmodule.h>
#include <kern/netdiag.h>


extern struct MsgPort *SanaPort;

ULONG netisr = 0L;

/*
 * scnednetisr(): schedule net "interrupt"
 * This routine signals TCP/IP task to run net_poll via sana_poll
 */
void schednetisr(int isr)
{
    netisr |= 1 << isr;
    netdiag.nq_sched++;
    Signal(SanaPort->mp_SigTask, 1 << SanaPort->mp_SigBit);
}

void schednetisr_nosignal(int isr)
{
    netisr |= 1 << isr;
    netdiag.nq_sched_quiet++;
}

/*
 * The input queues, in ascending order of their bit - which is the order
 * net_poll() serves them in.
 */
static struct netinput netinputs[NETINPUT_MAX];
static ULONG netinput_mask[NETINPUT_MAX];
static int netinput_count = 0;

int
netinput_register(const struct netinput *ni)
{
    int i;

    if(ni == NULL || ni->ni_intr == NULL || ni->ni_isr < 0 || ni->ni_isr > 31)
        return (EINVAL);
    for(i = 0; i < netinput_count; i++)
        if(netinputs[i].ni_family == ni->ni_family || netinputs[i].ni_isr == ni->ni_isr)
            return (EEXIST);
    if(netinput_count == NETINPUT_MAX)
        return (ENOSPC);
    /* kept in ascending order of their bit: IPv4 before IPv6, as always */
    {
        spl_t s = splimp();

        for(i = netinput_count; i > 0 && netinputs[i - 1].ni_isr > ni->ni_isr; i--) {
            netinputs[i] = netinputs[i - 1];
            netinput_mask[i] = netinput_mask[i - 1];
        }
        netinputs[i] = *ni;
        netinput_mask[i] = 1UL << ni->ni_isr;
        netinput_count++;
        splx(s);
    }
    /* entries have moved: whoever kept a pointer to one looks it up again */
    sana_linktype_uncache();
    return (0);
}

/*
 * The caller has stopped what fills the queue and emptied it. Not from
 * inside a queue's handler: net_poll() is walking the table then.
 */
int
netinput_unregister(int family)
{
    spl_t s;
    int i;

    for(i = 0; i < netinput_count; i++)
        if(netinputs[i].ni_family == family)
            break;
    if(i == netinput_count)
        return (ENOENT);
    s = splimp();
    netisr &= ~netinput_mask[i];
    for(; i < netinput_count - 1; i++) {
        netinputs[i] = netinputs[i + 1];
        netinput_mask[i] = netinput_mask[i + 1];
    }
    netinput_count--;
    splx(s);
    /* the entry is gone and those behind it have moved (net/if_sana.c) */
    sana_linktype_uncache();
    return (0);
}

/*
 * The stack is ending and the modules are about to go (net/netmodule.h):
 * no queue is served any more, and none is remembered.
 */
void
netinput_forget_all(void)
{
    spl_t s = splimp();

    netinput_count = 0;
    netisr = 0;
    splx(s);
    sana_linktype_uncache();
}

const struct netinput *
netinput_find(int family)
{
    int i;

    for(i = 0; i < netinput_count; i++)
        if(netinputs[i].ni_family == family)
            return (&netinputs[i]);
    return (NULL);
}

/*
 * net_poll(): run scheduled network level protocols
 *             this routine is called from sana_poll
 */
void
net_poll(void)
{
    spl_t s = splimp();
    ULONG n = netisr;
    int i;

    netisr = 0;
    splx(s);

    if(!n) return;
    netdiag.nq_run++;
    for(i = 0; i < netinput_count; i++)
        if(n & netinput_mask[i])
            (*netinputs[i].ni_intr)();
}
