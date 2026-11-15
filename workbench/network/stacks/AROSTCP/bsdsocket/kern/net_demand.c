/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Modules loaded on first use (net/netdemand.h): the demand slots, the
 * hand-over of a start to the stack's task, the ioctl numbers a module
 * serves, and the socket domains a module attaches on demand.
 *
 * WHERE A MODULE IS STARTED. Always in the stack's own task, as at start-up
 * and on a control-port command: a family's start resumes the reads of its
 * link types (net/if_sana.h allows that in the stack's task only), and the
 * loader reads the disk with the socket calls fenced for part of the time
 * (kern/net_modload.c). An application that needs a module started sends
 * NCMD_MODULE_DEMAND to the control port, from its own task and without the
 * fence, and waits for the answer; the stack's task does the work in
 * control_module() (kern/amiga_main.c) like any other command. That wait
 * cannot close a circle: the waiting task holds nothing the stack's task
 * takes - not the fence (netdemand_load() refuses a caller that holds it),
 * no socket, no semaphore of the stack - and the stack's task does not
 * wait for an application while it serves a command.
 *
 * While the stack's task reads a library from disk it does nothing else:
 * packets wait in the drivers' read requests, timers are late. Once, on
 * first use; a machine that cannot have that loads its modules at start
 * (MODULES=, kern/net_module.c).
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/synch.h>

#include <kern/amiga_includes.h>
#include <aros/atomic.h>
#include <exec/ports.h>
#include <exec/tasks.h>
#include <net/netmodule.h>
#include <net/netdemand.h>
#include <bsdsocket/netcontrol.h>

extern struct SignalSemaphore syscall_semaphore;    /* the fence (api/amiga_api.c) */
extern struct Task *AROSTCP_Task;                   /* the stack's own (kern/amiga_main.c) */

/*
 * The demand slots of the stub files (NETDEMAND_SLOT()). The set's empty
 * default is defined in kern/net_module.c, away from this file which walks
 * it, for the reason given at NETMODULES there.
 */
THIS_PROGRAM_HANDLES_SYMBOLSET(NETDEMANDS)
DECLARESET(NETDEMANDS)

/*
 * THE IOCTL RANGES WHOSE MODULE THE CORE STARTS when one of their numbers
 * is asked for while nobody serves it. Fixed, and small: one line per
 * module that takes ioctl numbers and is loaded on demand.
 *
 *  - 'r' 60-95: the filter and NAT requests and the IPv6 filter rules
 *    (90-92) (common/include/sys/ioctl.h), the packet services' (module
 *    "packet": net/ipfilter.c, ipf_ioctl()). A number of the range that
 *    the engine does not know goes on to the routing ioctls as before,
 *    after the module was started for it.
 *  - 'z' 208-209: the demo module's (modules/demo/demo.c), so that the test
 *    of this mechanism (cunit-net-demand) has a module of its own to load,
 *    stop and unload. Without the demo library installed, the first such
 *    request notes it once (nd_failed) and the numbers are what they were.
 */
static struct netioctl_demand {
    UBYTE               nid_group, nid_first, nid_last;
    struct netdemand    nid_slot;
} netioctl_demands[] = {
#ifdef ENABLE_PACKET_FILTER
    { 'r', 60, 95,   { "packet" } },
#endif
    { 'z', 208, 209, { "demo" } },
};
#define NETIOCTL_DEMANDS (sizeof(netioctl_demands) / sizeof(netioctl_demands[0]))

/*
 * THE SOCKET DOMAINS WHOSE MODULE THE CORE STARTS when socket() finds no
 * domain of the family: the routing socket, attached by the administration
 * module (kern/admin_module.c) on first use.
 */
static struct netdomain_demand {
    int                 ndd_family;
    struct netdemand    ndd_slot;
} netdomain_demands[] = {
    { PF_ROUTE, { "admin" } },
};
#define NETDOMAIN_DEMANDS (sizeof(netdomain_demands) / sizeof(netdomain_demands[0]))

/* the ranges that are served now (fenced: registered and taken back by modules) */
static struct netioctl netioctls[NETIOCTL_MAX];

/* one bit per ioctl group: a range of it is served or startable (NETIOCTL_GROUP()) */
UBYTE netioctl_groups[32];

/* the control port, while it exists (Forbid() guards the pointer: see netdemand_load()) */
static struct MsgPort *netdemand_port = NULL;
static BOOL netdemand_ending = FALSE;

/*
 * Every slot of the core, of the given module's name or (NULL) all: `fn'
 * is called for each. The slots are few - two tables of the core and the
 * stub files' - and this runs on the slow path only.
 */
static void
netdemand_each(const char *module, void (*fn)(struct netdemand *, void *), void *arg)
{
    struct netdemand *nd;
    long pos;
    int i;

    for(i = 0; i < NETIOCTL_DEMANDS; i++) {
        nd = &netioctl_demands[i].nid_slot;
        if(module == NULL || strcmp(nd->nd_module, module) == 0)
            (*fn)(nd, arg);
    }
    for(i = 0; i < NETDOMAIN_DEMANDS; i++) {
        nd = &netdomain_demands[i].ndd_slot;
        if(module == NULL || strcmp(nd->nd_module, module) == 0)
            (*fn)(nd, arg);
    }
    ForeachElementInSet(SETNAME(NETDEMANDS), 1, pos, nd)
        if(module == NULL || strcmp(nd->nd_module, module) == 0)
            (*fn)(nd, arg);
}

static void
netdemand_reset1(struct netdemand *nd, void *arg)
{
    nd->nd_ops = NULL;
    nd->nd_busy = 0;
    nd->nd_failed = 0;
}

static void
netioctl_regroup(void)
{
    int i, g;

    bzero(netioctl_groups, sizeof(netioctl_groups));
    for(i = 0; i < NETIOCTL_DEMANDS; i++) {
        g = netioctl_demands[i].nid_group;
        netioctl_groups[g >> 3] |= 1 << (g & 7);
    }
    for(i = 0; i < NETIOCTL_MAX; i++)
        if(netioctls[i].nio_handler != NULL) {
            g = netioctls[i].nio_group;
            netioctl_groups[g >> 3] |= 1 << (g & 7);
        }
}

void
netdemand_init(void)
{
    netdemand_each(NULL, netdemand_reset1, NULL);
    bzero(netioctls, sizeof(netioctls));
    netioctl_regroup();
    netdemand_port = NULL;
    netdemand_ending = FALSE;
}

/*
 * The stack is ending and the module libraries leave memory
 * (netmodule_shutdown_all()): no slot or delegated ioctl may point into one
 * of them any more. The control port and the ending flag are left as they
 * are - they are the shutdown's own business.
 */
void
netdemand_forget_all(void)
{
    netdemand_each(NULL, netdemand_reset1, NULL);
    bzero(netioctls, sizeof(netioctls));
    netioctl_regroup();
}

/*
 * The control port as it is made, and NULL before it is taken away: under
 * Forbid(), so that a task in netdemand_load() either puts its message
 * before - and is answered, at the latest when the stack replies to the
 * stragglers - or sees no port.
 */
void
netdemand_setport(struct MsgPort *port)
{
    Forbid();
    if(port == NULL && netdemand_port != NULL)
        netdemand_ending = TRUE;
    netdemand_port = port;
    Permit();
}

/*
 * THE SLOTS.
 */
int
netdemand_enter_error(struct netdemand *nd, const void **opsp)
{
    const void *ops;
    int error;

    AROS_ATOMIC_INC(nd->nd_busy);
    if((ops = nd->nd_ops) == NULL) {
        AROS_ATOMIC_DEC(nd->nd_busy);
        /* not started (or being stopped): have it started, then once more */
        if((error = netdemand_load(nd->nd_module)) != 0)
            return (error);
        AROS_ATOMIC_INC(nd->nd_busy);
        if((ops = nd->nd_ops) == NULL) {
            /* started, and stopped again at once, or it gave no table */
            AROS_ATOMIC_DEC(nd->nd_busy);
            return (ENOENT);
        }
    }
    if(opsp != NULL)
        *opsp = ops;
    return (0);
}

const void *
netdemand_enter(struct netdemand *nd)
{
    const void *ops;

    return (netdemand_enter_error(nd, &ops) == 0 ? ops : NULL);
}

void
netdemand_leave(struct netdemand *nd)
{
    AROS_ATOMIC_DEC(nd->nd_busy);
}

/*
 * Clear a slot's table, unless a task is inside: the table first, the count
 * after - the counterpart of netdemand_enter_error(), which counts first
 * and reads the table after. The read of the count is made through a locked
 * increment of it, which orders it after the clear on a processor that
 * would otherwise let the read overtake the write. EBUSY: the table is
 * back as it was.
 */
static int
netdemand_clear(struct netdemand *nd)
{
    const void *ops = nd->nd_ops;
    LONG busy;

    nd->nd_ops = NULL;
    AROS_ATOMIC_INC(nd->nd_busy);
    busy = nd->nd_busy - 1;
    AROS_ATOMIC_DEC(nd->nd_busy);
    if(busy != 0) {
        nd->nd_ops = ops;
        return (EBUSY);
    }
    return (0);
}

/*
 * The module's table into the slots of its name (the stub files' slots;
 * the ioctl ranges' slots are kept by netioctl_register()), or out of them.
 * Taken out only when no task is inside any of them, else EBUSY and every
 * slot as it was. A module has a slot or two; more than NETDEMAND_SLOTS_MAX
 * of one name is a mistake of the stub files (EINVAL, nothing changed).
 */
#define NETDEMAND_SLOTS_MAX 8

int
netdemand_set(const char *module, const void *ops)
{
    struct netdemand *nd, *cleared[NETDEMAND_SLOTS_MAX];
    const void *old[NETDEMAND_SLOTS_MAX];
    long pos;
    int n = 0, error = 0;

    if(module == NULL)
        return (EINVAL);
    ForeachElementInSet(SETNAME(NETDEMANDS), 1, pos, nd) {
        if(strcmp(nd->nd_module, module) != 0)
            continue;
        if(ops != NULL) {
            nd->nd_ops = ops;
            continue;
        }
        if(n == NETDEMAND_SLOTS_MAX) {
            error = EINVAL;
            break;
        }
        old[n] = nd->nd_ops;
        if((error = netdemand_clear(nd)) != 0)
            break;
        cleared[n++] = nd;
    }
    if(error != 0)
        while(--n >= 0)
            cleared[n]->nd_ops = old[n];
    return (error);
}

static void
netdemand_fail1(struct netdemand *nd, void *arg)
{
    nd->nd_failed = 1;
}

static void
netdemand_forget1(struct netdemand *nd, void *arg)
{
    nd->nd_failed = 0;
}

static void
netdemand_isfailed1(struct netdemand *nd, void *arg)
{
    if(nd->nd_failed)
        *(int *)arg = 1;
}

void
netdemand_forget_failed(const char *module)
{
    netdemand_each(module, netdemand_forget1, NULL);
}

/*
 * NCMD_MODULE_DEMAND, in the stack's task without the fence: start the
 * module if it is known, else load its library - unless it could not be
 * had before, or its test switch (ENV:AROSTCP/Without_<name>,
 * netmodule_leftout()) is set, which keeps out a module of the program as
 * well as one in a file. Only a module that cannot be had
 * marks its slots (a known module whose start fails - a filter engine on a
 * stack without IPv4 - is asked again next time, as before: that costs no
 * disk access).
 */
int
netdemand_run(const char *module)
{
    struct netmodule *nm;
    char lib[48], var[48];
    int len, failed = 0, error;

    if(module == NULL || (len = strlen(module)) == 0)
        return (EINVAL);
    netdemand_each(module, netdemand_isfailed1, &failed);
    if(failed)
        return (ENOENT);

    /* only this task starts, stops, loads and unloads: nm stays valid without the fence */
    ObtainSemaphore(&syscall_semaphore);
    nm = netmodule_known(module);
    ReleaseSemaphore(&syscall_semaphore);
    if(nm != NULL && (nm->nm_flags & NMF_STARTED))
        return (0);

    /* the test switch keeps a module out whether its code is in the program or in a file */
    if(netmodule_leftout(module, len)) {
        netmodule_leaveout_name(module, len, var, sizeof(var));
        __log(LOG_NOTICE, "module %s is not loaded (%s is set): what needs it fails.",
              module, var);
        netdemand_each(module, netdemand_fail1, NULL);
        return (ENOENT);
    }
    if(nm != NULL) {
        ObtainSemaphore(&syscall_semaphore);
        error = netmodule_use(nm);
        ReleaseSemaphore(&syscall_semaphore);
        return (error);
    }
    if(!netmodule_libname(module, len, NETMODULE_KIND_ATCP, lib, sizeof(lib)))
        return (ENOENT);
    /* the loader says in the log why not, once: then the slots are marked */
    if((error = netmodule_load_optional(lib, &nm)) == 0 && strcmp(nm->nm_name, module) != 0) {
        __log(LOG_ERR, "%s holds module %s, not %s.", lib, nm->nm_name, module);
        (void)netmodule_unload(nm);
        error = ENOENT;
    }
    if(error != 0)
        netdemand_each(module, netdemand_fail1, NULL);
    return (error);
}

/*
 * Have the module started, in the stack's task. From another task the
 * request goes through the control port, by the core's own pointer to it;
 * the message and its reply port are on the caller's stack, the reply
 * port signals SIGF_SINGLE (cleared first), as exec's own short waits do.
 */
int
netdemand_load(const char *module)
{
    struct NetControlModuleMsg mm;
    struct MsgPort reply, *port;
    struct Task *me = FindTask(NULL);

    if(me == AROSTCP_Task)
        return (netdemand_run(module));
    if(module == NULL || *module == '\0')
        return (EINVAL);
    if(strlen(module) >= NCM_MODULE_NAMELEN)
        return (ENOENT);
    /* the stack's task takes the fence to start a module: waiting for it here would stop the stack */
    if(syscall_semaphore.ss_Owner == me) {
        __log(LOG_ERR, "module %s is needed by a call that holds the socket calls' fence: "
              "refused (it would stop the stack).", module);
        return (EDEADLK);
    }

    bzero(&mm, sizeof(mm));
    mm.ncmm_Msg.ncm_Msg.mn_Node.ln_Type = NT_MESSAGE;
    mm.ncmm_Msg.ncm_Msg.mn_ReplyPort = &reply;
    mm.ncmm_Msg.ncm_Msg.mn_Length = sizeof(mm);
    mm.ncmm_Msg.ncm_Command = NCMD_MODULE_DEMAND;
    strcpy(mm.ncmm_Name, module);

    bzero(&reply, sizeof(reply));
    reply.mp_Node.ln_Type = NT_MSGPORT;
    reply.mp_Flags = PA_SIGNAL;
    reply.mp_SigBit = SIGB_SINGLE;
    reply.mp_SigTask = me;
    NEWLIST(&reply.mp_MsgList);
    SetSignal(0, SIGF_SINGLE);

    Forbid();
    if((port = netdemand_port) != NULL)
        PutMsg(port, &mm.ncmm_Msg.ncm_Msg);
    Permit();
    if(port == NULL)
        return (netdemand_ending ? ESHUTDOWN : ENOENT);   /* not up yet, or ending */

    WaitPort(&reply);
    GetMsg(&reply);
    return (mm.ncmm_Msg.ncm_Result == -1 ? ESHUTDOWN : mm.ncmm_Msg.ncm_Result);
}

/*
 * THE IOCTL RANGES.
 */
static struct netioctl_demand *
netioctl_demand_of(int group, int nr)
{
    int i;

    for(i = 0; i < NETIOCTL_DEMANDS; i++)
        if(netioctl_demands[i].nid_group == group &&
                nr >= netioctl_demands[i].nid_first && nr <= netioctl_demands[i].nid_last)
            return (&netioctl_demands[i]);
    return (NULL);
}

struct netdemand *
netioctl_demand(ULONG cmd)
{
    struct netioctl_demand *nid = netioctl_demand_of(IOCGROUP(cmd), cmd & 0xff);

    return (nid != NULL ? &nid->nid_slot : NULL);
}

const struct netioctl *
netioctl_find(ULONG cmd)
{
    int i, group = IOCGROUP(cmd), nr = cmd & 0xff;

    for(i = 0; i < NETIOCTL_MAX; i++)
        if(netioctls[i].nio_handler != NULL && netioctls[i].nio_group == group &&
                nr >= netioctls[i].nio_first && nr <= netioctls[i].nio_last)
            return (&netioctls[i]);
    return (NULL);
}

/* whether a fixed range of the module's name overlaps the range (the slot then follows it) */
static int
netioctl_slot_of(const struct netioctl *nio, const struct netioctl_demand *nid)
{
    return (nio->nio_owner != NULL && nid->nid_group == nio->nio_group &&
            nid->nid_first <= nio->nio_last && nid->nid_last >= nio->nio_first &&
            strcmp(nid->nid_slot.nd_module, nio->nio_owner->nm_name) == 0);
}

int
netioctl_register(int group, int first, int last, netioctl_handler handler,
                  struct netmodule *owner, int flags)
{
    struct netioctl nio;
    int i, slot = -1, slotted = 0;

    if(handler == NULL || group < 0 || group > 255 || first < 0 || last > 255 || first > last)
        return (EINVAL);
    nio.nio_group = group;
    nio.nio_first = first;
    nio.nio_last = last;
    nio.nio_flags = flags;
    nio.nio_handler = handler;
    nio.nio_owner = owner;

    for(i = 0; i < NETIOCTL_MAX; i++) {
        if(netioctls[i].nio_handler == NULL) {
            if(slot < 0)
                slot = i;
        } else if(netioctls[i].nio_group == group && netioctls[i].nio_first <= last &&
                  netioctls[i].nio_last >= first)
            return (EEXIST);
    }
    for(i = 0; i < NETIOCTL_DEMANDS; i++)
        if(netioctl_slot_of(&nio, &netioctl_demands[i]))
            slotted = 1;
    /* without its slot nothing would keep the module from being stopped under the call */
    if((flags & NIOF_UNFENCED) && !slotted)
        return (EINVAL);
    if(slot < 0)
        return (ENOSPC);

    netioctls[slot] = nio;
    /* the range is served: a caller of the fixed range goes straight on */
    for(i = 0; i < NETIOCTL_DEMANDS; i++)
        if(netioctl_slot_of(&nio, &netioctl_demands[i]))
            netioctl_demands[i].nid_slot.nd_ops = &netioctls[slot];
    netioctl_regroup();
    return (0);
}

/*
 * Every range of the handler is given back. A range served with the fence
 * held has nobody inside it now (this runs fenced): its slot is cleared
 * whatever the count says - a caller between its slot and the fence finds
 * nobody serving the number under the fence and goes on as if the range
 * had never been taken. A range served without the fence may have a task
 * inside the handler: then EBUSY, and nothing is changed.
 */
int
netioctl_unregister(netioctl_handler handler)
{
    int i, j, error = 0, found = 0;

    if(handler == NULL)
        return (EINVAL);
    for(i = 0; i < NETIOCTL_MAX && error == 0; i++) {
        if(netioctls[i].nio_handler != handler || !(netioctls[i].nio_flags & NIOF_UNFENCED))
            continue;
        for(j = 0; j < NETIOCTL_DEMANDS && error == 0; j++)
            if(netioctl_slot_of(&netioctls[i], &netioctl_demands[j]) &&
                    (error = netdemand_clear(&netioctl_demands[j].nid_slot)) == 0)
                netioctl_demands[j].nid_slot.nd_ops = &netioctls[i];    /* cleared below */
    }
    if(error != 0)
        return (error);

    for(i = 0; i < NETIOCTL_MAX; i++) {
        if(netioctls[i].nio_handler != handler)
            continue;
        for(j = 0; j < NETIOCTL_DEMANDS; j++)
            if(netioctl_slot_of(&netioctls[i], &netioctl_demands[j]))
                netioctl_demands[j].nid_slot.nd_ops = NULL;
        bzero(&netioctls[i], sizeof(netioctls[i]));
        found = 1;
    }
    if(!found)
        return (ENOENT);
    netioctl_regroup();
    return (0);
}

/*
 * THE SOCKET DOMAINS.
 */
struct netdemand *
netdomain_demand(int family)
{
    int i;

    for(i = 0; i < NETDOMAIN_DEMANDS; i++)
        if(netdomain_demands[i].ndd_family == family)
            return (&netdomain_demands[i].ndd_slot);
    return (NULL);
}
