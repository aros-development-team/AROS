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

/*
 * Copyright (C) 1982, 1986 Regents of the University of California.
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
 *	@(#)uipc_domain.c	7.9 (Berkeley) 3/4/91
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>

#include <kern/amiga_includes.h>
#include <kern/uipc_domain_protos.h>
#include <net/netmodule.h>

/* --- start moved from sys/domain.h --- */
struct	domain *domains = NULL;
/* --- end moved from sys/domain.h --- */

static BOOL domain_initialized = FALSE;

/*
 * The set of modules linked into the stack binary (kern/net_module.c walks
 * it). The empty default is defined here, away from the code that reads
 * the list, so that the compiler cannot assume its length.
 */
#include <aros/symbolsets.h>
DEFINESET(NETMODULES)

static void
domain_sizes(void)
{
#ifdef AMITCP
    /*
     * No space needed for the link header with SanaII drivers
     */
    max_linkhdr = 0;
#else
    if(max_linkhdr < 16)		/* XXX */
        max_linkhdr = 16;
#endif
    max_hdr = max_linkhdr + max_protohdr;
    max_datalen = MHLEN - max_hdr;
}

/*
 * Attach a domain: link it at the end of the list (so the domains keep the
 * order they were attached in), then initialise it and its protocols.
 */
int
domain_attach(struct domain *dp)
{
    struct domain **dpp;
    struct protosw *pr;

    for(dpp = &domains; *dpp; dpp = &(*dpp)->dom_next)
        if(*dpp == dp || (*dpp)->dom_family == dp->dom_family)
            return (EEXIST);
    dp->dom_next = NULL;
    dp->dom_refs = 0;
    *dpp = dp;

    if(dp->dom_init)
        (*dp->dom_init)();
    for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
        if(pr->pr_init)
            (*pr->pr_init)();
    domain_sizes();
    return (0);
}

/*
 * Detach a domain. Only the link is undone here: what the domain's protocols
 * own (tables, queues, timers) is released by the module that attached it,
 * before it calls this.
 */
int
domain_detach(struct domain *dp)
{
    struct domain **dpp;

    if(dp->dom_refs != 0)
        return (EBUSY);
    for(dpp = &domains; *dpp; dpp = &(*dpp)->dom_next)
        if(*dpp == dp) {
            *dpp = dp->dom_next;
            dp->dom_next = NULL;
            return (0);
        }
    return (ENOENT);
}

/*
 * The stack is ending and the modules are about to go (net/netmodule.h):
 * the list is dropped as a whole. The domains and their protocol tables
 * are the modules' own data; the protocols' timers have been deleted
 * (timer_deinit()) and no socket exists.
 */
void
domain_forget_all(void)
{
    domains = NULL;
}

/*
 * Start the families compiled into the stack. Each is a module
 * (net/netmodule.h) whose start function attaches its domain; the order is
 * the order the domains have always been initialised in.
 */
BOOL
domaininit()
{
    if(domain_initialized)
        return TRUE;

    netmodule_start_builtin();
    domain_sizes();

    domain_initialized = TRUE;
    return TRUE;
}

struct protosw *
pffindtype(family, type)
int family, type;
{
    register struct domain *dp;
    register struct protosw *pr;

    for(dp = domains; dp; dp = dp->dom_next)
        if(dp->dom_family == family)
            goto found;
    return (0);
found:
    for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
        if(pr->pr_type && pr->pr_type == type)
            return (pr);
    return (0);
}

struct protosw *
pffindproto(family, protocol, type)
int family, protocol, type;
{
    register struct domain *dp;
    register struct protosw *pr;
    struct protosw *maybe = 0;

    if(family == 0)
        return (0);
    for(dp = domains; dp; dp = dp->dom_next)
        if(dp->dom_family == family)
            goto found;
    return (0);
found:
    for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
        if((pr->pr_protocol == protocol) && (pr->pr_type == type))
            return (pr);

        if(type == SOCK_RAW && pr->pr_type == SOCK_RAW &&
                pr->pr_protocol == 0 && maybe == (struct protosw *)0)
            maybe = pr;
    }
    return (maybe);
}

void
pfctlinput(cmd, sa)
int cmd;
struct sockaddr *sa;
{
    register struct domain *dp;
    register struct protosw *pr;

    for(dp = domains; dp; dp = dp->dom_next)
        for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
            if(pr->pr_ctlinput)
                (*pr->pr_ctlinput)(cmd, sa, (caddr_t) 0);
}

void
pfslowtimo()
{
    register struct domain *dp;
    register struct protosw *pr;

    for(dp = domains; dp; dp = dp->dom_next)
        for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
            if(pr->pr_slowtimo)
                (*pr->pr_slowtimo)();
#ifndef AMITCP
    /*
     * Timeouts are scheduled from amiga_main.c in AmiTCP/IP
     */
    timeout(pfslowtimo, (caddr_t)0, hz / 2);
#endif /* AMITCP */
}

void
pffasttimo()
{
    register struct domain *dp;
    register struct protosw *pr;

    for(dp = domains; dp; dp = dp->dom_next)
        for(pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
            if(pr->pr_fasttimo)
                (*pr->pr_fasttimo)();
#ifndef AMITCP
    /*
     * Timeouts are scheduled from amiga_main.c in AmiTCP/IP
     */
    timeout(pffasttimo, (caddr_t)0, hz / 5);
#endif /* AMITCP */
}
