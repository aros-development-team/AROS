/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <conf.h>

#include <exec/types.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include <libraries/netservice.h>
#include <api/netservice_api.h>
#include <kern/amiga_svcmgr.h>
#include <kern/amiga_netsvc.h>
#include <kern/amiga_netdb.h>
#include <kern/amiga_dhcp.h>
#include <kern/amiga_rc.h>
#include <api/amiga_api.h>
#include <proto/dos.h>

/* Grace period (ticks, 1/50 s) between the reconfigure-begin signal and the
 * teardown, so cooperative consumers can drop sockets bound to addresses that
 * are about to disappear before we delete them. */
#define NET_RELOAD_GRACE_STEP   5       /* poll interval (ticks) while awaiting acks */
#define NET_RELOAD_GRACE_MAX    100     /* cap (~2s): proceed even if some don't ack */

/* implemented in net/if_sana.c */
extern void sana_reconfig_teardown(void);

/*
 * Service ops.  Start order (descending ln_Pri): resolver -> interfaces ->
 * dhcp -> rc.  Stop order is the reverse, so the resolver's dynamic DNS is
 * wiped last (after interfaces and DHCP have stopped feeding it) and the
 * rc-daemons are stopped first.
 */

/* resolver: dynamic DNS.  Repopulated by interfaces/DHCP on start, so start is
 * a no-op; stop wipes the dynamic nameserver/domain lists. */
static LONG svc_resolver_stop(struct NetService *s, BOOL force)
{
    (void)s; (void)force;
    dyndb_flush();
    return 0;
}
static const struct NetServiceOps resolver_ops = { NULL, svc_resolver_stop };

/* interfaces/config: start re-reads the config database (addifent re-applies
 * each interface - MODIFYOLD for an existing one, ADDNEW for a new one); stop
 * drops every interface's addresses, routes, DNS and autoip in place. */
static LONG svc_interfaces_start(struct NetService *s)
{
    (void)s;
    return netdb_reload();
}
static LONG svc_interfaces_stop(struct NetService *s, BOOL force)
{
    (void)s; (void)force;
    sana_reconfig_teardown();
    return 0;
}
static const struct NetServiceOps interfaces_ops = { svc_interfaces_start, svc_interfaces_stop };

/* dhcp is now an EXTERNAL service daemon (services/dhcp -> db/services.d/dhcp); it
 * registers itself through netservices.library and manages dhclient in its own
 * process, so there is no internal dhcp netservice here any more. */

/* rc-launched daemons (inetd, ...): cycle them so they release and reacquire
 * their stack bindings around the reconfigure. */
static LONG svc_rc_start(struct NetService *s)
{
    (void)s;
    rc_start();
    return 0;
}
static LONG svc_rc_stop(struct NetService *s, BOOL force)
{
    (void)s; (void)force;
    rc_stop();
    return 0;
}
static const struct NetServiceOps rc_ops = { svc_rc_start, svc_rc_stop };

static struct NetService svc_resolver;
static struct NetService svc_interfaces;
static struct NetService svc_rc;

static void reg(struct NetService *s, CONST_STRPTR name, BYTE pri,
                const struct NetServiceOps *ops)
{
    s->ns_Node.ln_Name = (char *)name;
    s->ns_Node.ln_Pri  = pri;
    s->ns_Ops          = ops;
    netservice_register(s);
    /* boot has already brought these subsystems up via the normal init path */
    s->ns_State = NSSTATE_RUNNING;
}

void net_services_register(void)
{
    D(bug("[AROSTCP](amiga_netsvc.c) net_services_register()\n"));
    reg(&svc_resolver,   "resolver",   NSPRI_RESOLVER,   &resolver_ops);
    reg(&svc_interfaces, "interfaces", NSPRI_INTERFACES, &interfaces_ops);
    /*
     * rc-daemon cycling: rc_stop/rc_start spawn async processes that walk the
     * NDB rc list under the NDB lock (so netdb_reload()'s locked NDB swap can't
     * free it underneath them).  The legacy rc-done CTRL-F signal, which collided
     * with the main loop's breakmask and made cycling look like a shutdown, has
     * been removed from rc_stop_process().  Daemons cycle asynchronously around
     * the reconfigure.
     */
    reg(&svc_rc,         "rc",         NSPRI_RCDAEMONS,  &rc_ops);

    /* External service-daemon launcher (db/services).  Registers the
     * "servicemgr" netservice and launches the manifest daemons for boot. */
    svcmgr_register();
}

LONG net_reload(void)
{
    LONG error;

    /* Tell cooperative consumers a reconfigure is starting so they can quiesce
     * (drop sockets bound to doomed addresses) before we tear the config down.
     * Wait for them to acknowledge (SBTC_RECONFIG_ACK) rather than a fixed
     * delay: proceed as soon as every signalled consumer has acked, and cap the
     * wait so a consumer that never acks cannot stall the reload.  With no
     * subscribers (expected == 0) this returns at once. */
    api_sendreconfig(TRUE);
    netservice_signal_external(NSPHASE_BEGIN);
    {
        int waited;
        for(waited = 0;
                api_reconfig_acked < api_reconfig_expected &&
                    waited < NET_RELOAD_GRACE_MAX;
                waited += NET_RELOAD_GRACE_STEP)
            Delay(NET_RELOAD_GRACE_STEP);
    }

    /*
     * Fence the teardown + reconfigure against consumers doing socket I/O: hold
     * the syscall semaphore so socket API calls from other tasks block until the
     * new config is in place and cannot observe a half-torn interface/route set.
     * tsleep() releases this semaphore while a call blocks (kern_synch.c), so a
     * consumer parked in recv()/WaitSelect() cannot stall the reload.  The grace
     * above and the begin/end signals are deliberately outside the fence, so an
     * external service can do socket I/O to drop/reacquire its bindings without
     * blocking on the semaphore we hold here.
     */
    ObtainSemaphore(&syscall_semaphore);
    D(bug("[AROSTCP](amiga_netsvc.c) net_reload(): stopping services\n"));
    netservice_stop_all(FALSE);
    D(bug("[AROSTCP](amiga_netsvc.c) net_reload(): starting services\n"));
    error = netservice_start_all();
    ReleaseSemaphore(&syscall_semaphore);

    /* New config is up: tell external services to re-establish their bindings,
     * then bump the generation and notify cooperative consumers to refresh. */
    netservice_signal_external(NSPHASE_END);
    api_sendreconfig(FALSE);

    return error;
}
