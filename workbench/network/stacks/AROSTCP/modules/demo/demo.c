/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * demo.atcp (a working name): the smallest module of the stack
 * that is a library file of its own. It carries no protocol. It is there
 * to prove, by being loaded, started, stopped and unloaded on a running
 * system, the way a module library reaches the core
 * (bsdsocket/net/netcore.h, bsdsocket/kern/net_modload.c):
 *
 *  - it is written like the stack's protocol code: it calls the core's
 *    functions and reads the core's variables by name, and the one include
 *    line at the end of the include block turns each of those names into
 *    an entry of this library's copy of the core's table;
 *  - it has ordinary C data of its own - static, initialised, counted in -
 *    which is new each time the library is loaded and is gone when it is
 *    unloaded;
 *  - what it registers with the core when it starts it takes back when it
 *    stops.
 *
 * It needs no other module (nm_requires is empty), so it loads on a stack
 * that has only IPv4, only IPv6, or neither.
 *
 * The tests are developer/debug/test/net/socket/cunit-net-modlib.c, which
 * reads and sets the variables below through sysctl(), and
 * cunit-net-demand.c, which has the library loaded on first use through
 * the two ioctl numbers below (the core starts this module when one of
 * them is asked for: kern/net_demand.c).
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/synch.h>

#include <sys/ioctl.h>

#include <net/if.h>
#include <net/netmodule.h>

#include <net/netcore_map.h>        /* always the last include */

/*
 * The demo's variables as sysctl() names them: net.253.253.<id>. 253 is
 * not an address family; it only keeps the names clear of every real one.
 * (253 is also the IP protocol number set aside for experiments, RFC 3692,
 * which is the number this module would take from a family - see
 * demo_start().)
 */
#define DEMO_MIB_FAMILY 253
#define DEMO_MIB_PROTO  253
#define DEMO_MIB_HOLD   1       /* int, read and set: not 0 = the module is in use */
#define DEMO_MIB_STATS  2       /* struct demo_stats, read only */

#define DEMO_MAGIC      0x44454d4f      /* "DEMO" */

struct demo_stats {
    unsigned int    ds_magic;       /* DEMO_MAGIC: initialised data, as the file has it */
    unsigned int    ds_starts;      /* times started since the library was loaded */
    unsigned int    ds_stops;       /* times stopped since the library was loaded */
    unsigned int    ds_mbufs;       /* mbufs taken from the core and given back */
    unsigned int    ds_interfaces;  /* interfaces seen on the core's list at the last start */
    unsigned int    ds_inet;        /* 1 when the IPv4 module was running at the last start */
};

/*
 * The demo's ioctl numbers (any socket): the core starts the module - and
 * loads this library - when one of them is asked for while it does not run
 * (net/netdemand.h). They are served without the socket calls fenced, as a
 * request that may wait must be, so a caller can be "inside" the module
 * while another task tries to stop it.
 *
 *  DEMOIOC_CALLS   out: how many requests the module has served since the
 *                  library was loaded, this one included
 *  DEMOIOC_WAIT    signals dw_task with dw_inside, then waits in the
 *                  caller's task for any of dw_wait: the caller is inside
 *                  the module until it is signalled
 */
struct demo_wait {
    struct Task    *dw_task;
    ULONG           dw_inside;
    ULONG           dw_wait;
};
#define DEMOIOC_CALLS   _IOR('z', 208, ULONG)
#define DEMOIOC_WAIT    _IOW('z', 209, struct demo_wait)

/* this library's own data: one copy per load */
static struct demo_stats demo_stats = { DEMO_MAGIC, 0, 0, 0, 0, 0 };
static int demo_hold = 0;
static ULONG demo_calls = 0;

static const struct netvar demo_netvars[] = {
    { "demo_hold",  &demo_hold,  sizeof(demo_hold),  NVT_INT,   DEMO_MIB_FAMILY, DEMO_MIB_PROTO, DEMO_MIB_HOLD },
    { "demo_stats", &demo_stats, sizeof(demo_stats), NVT_STATS, DEMO_MIB_FAMILY, DEMO_MIB_PROTO, DEMO_MIB_STATS },
};
#define DEMO_NETVARS    (sizeof(demo_netvars) / sizeof(demo_netvars[0]))

/* in the caller's task, without the fence: exec's calls only, and nothing of the core */
static int
demo_ioctl(struct socket *so, ULONG cmd, caddr_t data)
{
    struct demo_wait *dw;

    switch(cmd) {
    case DEMOIOC_CALLS:
        *(ULONG *)data = ++demo_calls;
        return (0);
    case DEMOIOC_WAIT:
        dw = (struct demo_wait *)data;
        demo_calls++;
        if(dw->dw_task != NULL && dw->dw_inside != 0)
            Signal(dw->dw_task, dw->dw_inside);
        if(dw->dw_wait != 0)
            Wait(dw->dw_wait);
        return (0);
    }
    return (NETIOCTL_PASS);
}

static int
demo_start(struct netmodule *nm)
{
    struct netmodule *inet;
    struct ifnet *ifp;              /* the structure tag is untouched by the mapping */
    struct mbuf *m, *n;
    int error;

    /* a function of the core, through the table */
    if((m = m_get(M_DONTWAIT, MT_DATA)) == NULL)
        return (ENOBUFS);
    m->m_len = 0;
    (void)m_free(m);
    demo_stats.ds_mbufs++;

    /*
     * The inline macros of sys/mbuf.h, as the protocol code uses them:
     * they work on the core's free list and its statistics, which are
     * variables of the core (mfree, mbstats).
     */
    MGET(m, M_DONTWAIT, MT_DATA);
    if(m == NULL)
        return (ENOBUFS);
    MFREE(m, n);
    (void)n;
    demo_stats.ds_mbufs++;

    /* a variable of the core whose own name is a structure tag */
    demo_stats.ds_interfaces = 0;
    for(ifp = ifnet_list; ifp != NULL; ifp = ifp->if_next)
        demo_stats.ds_interfaces++;

    /* a registration: undone in demo_stop() */
    if((error = netvar_register(demo_netvars, DEMO_NETVARS)) != 0)
        return (error);
    /* ... and the ioctl numbers; a start that fails takes back what it did */
    if((error = netioctl_register('z', 208, 209, demo_ioctl, nm, NIOF_UNFENCED)) != 0) {
        (void)netvar_unregister(demo_netvars);
        return (error);
    }

    /*
     * A protocol would now take its number from each family that is
     * there. That registration does not exist yet in a form a module can
     * reach: the dispatch table of IPv4 belongs to the IPv4 code
     * (ip_proto_register()), which is not part of the core, and the
     * family-neutral interface does not offer one. So this module only
     * looks, through the registry, whether the family is running, and
     * says so.
     */
    inet = netmodule_known("inet");
    demo_stats.ds_inet = (inet != NULL && (inet->nm_flags & NMF_STARTED)) ? 1 : 0;

    demo_stats.ds_starts++;
    log(LOG_NOTICE, "module %s: started (start %ld of this load; %ld interfaces; IPv4 %s).",
        nm->nm_name, (long)demo_stats.ds_starts, (long)demo_stats.ds_interfaces,
        demo_stats.ds_inet ? "is running - protocol 253 would be registered with it"
                           : "is not running");
    return (0);
}

/* 0, or why a stop would be refused: while somebody set demo_hold it is "in use". */
static int
demo_can_stop(struct netmodule *nm)
{
    return (demo_hold != 0 ? EBUSY : 0);
}

static int
demo_stop(struct netmodule *nm)
{
    int error;

    /* first, while nothing is changed yet: refused while a caller is inside demo_ioctl() */
    if((error = netioctl_unregister(demo_ioctl)) != 0)
        return (error);
    (void)netvar_unregister(demo_netvars);
    demo_stats.ds_stops++;
    log(LOG_NOTICE, "module %s: stopped (stop %ld of this load).",
        nm->nm_name, (long)demo_stats.ds_stops);
    return (0);
}

/* the one name the library's glue (bsdsocket/net/netmodule_lib.c) expects */
struct netmodule netmodule_this = {
    sizeof(struct netmodule), NETMODULE_VERSION, "demo", 0,
    demo_start, demo_stop, demo_can_stop,
    0, NULL, NULL,
    NULL,           /* needs no other module */
    NULL, NULL
};
