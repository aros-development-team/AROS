/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef NET_NETDEMAND_H
#define NET_NETDEMAND_H

/*
 * MODULES LOADED ON FIRST USE (kern/net_demand.c).
 *
 * An optional part of the stack - the resolver, the administration calls,
 * the packet services - is a module whose library is read from disk the
 * first time an application asks for something of it, not when the stack
 * starts. The application does not notice: the vector it calls stays in
 * the core as a stub that finds the module's table of functions through a
 * demand slot, starts the module (loading its library) when it is not
 * running, and calls through the table:
 *
 *      if((ops = netdemand_enter(&resolver_demand)) == NULL)
 *          return (what the call answers when it cannot do its work);
 *      result = ops->ro_gethostbyname(name, libPtr);
 *      netdemand_leave(&resolver_demand);
 *
 * THE ONE RULE EVERY STUB OBEYS: netdemand_enter() is called BEFORE the
 * socket calls are fenced (ObtainSyscallSemaphore()), never inside. A
 * module is started in the stack's own task, which takes the fence to do
 * it; a caller that held the fence while it waited for that would stop
 * the stack. (netdemand_load() refuses with EDEADLK rather than wait.)
 *
 * The slot. While the module runs nd_ops is its table: the module puts it
 * there with netdemand_set() in its nm_start and takes it away in its
 * nm_stop. nd_busy counts the tasks that are inside the module through the
 * slot now - possibly asleep in a socket call of the core - and a stop
 * that finds one is refused (EBUSY), so the module's code cannot leave
 * memory under a caller. The two sides need no lock: a caller counts up
 * and then reads the table; a stop clears the table and then reads the
 * count. Whichever comes second sees the other.
 *
 * nd_failed: the module could not be had (its library is not installed,
 * would not load or start, or its test switch ENV:AROSTCP/Without_<name>
 * is set). It is not tried again - no disk access for every call - until
 * the configuration is reloaded (NCMD_RELOAD) or the module is loaded by
 * name (NCMD_MODULE_LOAD). The log says so once.
 *
 * A slot is static data of the core: the stub file of the part that is
 * loaded on demand defines it and puts it into the symbol set with
 * NETDEMAND_SLOT(), so that netdemand_set() finds it by the module's name.
 * The slots of the core's own delegation tables (ioctl numbers, socket
 * domains) are kept in kern/net_demand.c.
 *
 * Cost when the module runs: two counts and a test around an indirect
 * call. Nothing that runs for each packet enters a slot.
 */

#include <exec/types.h>
#include <sys/types.h>             /* caddr_t */

struct netdemand {
    const char           *nd_module;    /* "resolver": resolver.atcp */
    const void *volatile  nd_ops;       /* the module's table; NULL while it is not started */
    volatile LONG         nd_busy;      /* tasks inside the module through this slot now */
    UBYTE                 nd_failed;    /* it could not be had: not tried again until told */
};

#ifndef NETMODULE
#include <aros/symbolsets.h>
/* next to the definition of a slot in a stub file of the core */
#define NETDEMAND_SLOT(slot)    ADD2SET(slot, NETDEMANDS, 0)
#endif

#ifdef KERNEL
struct socket;
struct netmodule;
struct MsgPort;

/*
 * The module's table, starting the module first if need be; NULL when it
 * cannot be had (then the stub answers what the call answers when it
 * cannot do its work). A table that was handed out must be given back with
 * netdemand_leave() when the call is done with it. Any task; never with
 * the socket calls fenced.
 */
const void *netdemand_enter(struct netdemand *nd);
/* the same; 0 and the table in *opsp, or why not (an errno) */
int         netdemand_enter_error(struct netdemand *nd, const void **opsp);
void        netdemand_leave(struct netdemand *nd);

/*
 * The module puts its table into its slots (every slot of the core with
 * its name) in nm_start, and takes it away with NULL in nm_stop - where
 * EBUSY means a task is inside it: the stop is to be refused, and nothing
 * was changed. 0: done (also when the core has no slot of that name).
 * In the stack's task, fenced, as starts and stops are.
 */
int         netdemand_set(const char *module, const void *ops);

/*
 * Start the module `module' if it is known (part of the stack program, or
 * loaded and stopped), else load <module>.atcp and start it. 0
 * also when it runs already; else an errno: ENOENT it is not installed, its
 * test switch is set, or it could not be had before (nd_failed); what its
 * start failed with (ENXIO: something it needs is left out by the
 * configuration); ESHUTDOWN the stack is ending; EDEADLK the caller holds
 * the fence. From any task: one that is not the stack's own sends
 * NCMD_MODULE_DEMAND to the control port and waits, so that the work is
 * done in the stack's task, as every start of a module is.
 */
int         netdemand_load(const char *module);

/*
 * The stack's side (kern/amiga_main.c). netdemand_init(): at start, before
 * anything can ask. netdemand_setport(): the control port, as it is made
 * and before it is taken away (NULL). netdemand_run(): the work of
 * NCMD_MODULE_DEMAND, in the stack's task, the fence not held.
 * netdemand_forget_failed(): a reload (NULL: every module) or an explicit
 * load of `module' clears nd_failed.
 */
void        netdemand_init(void);
void        netdemand_forget_all(void);     /* at shutdown: nothing points into a library */
void        netdemand_setport(struct MsgPort *port);
int         netdemand_run(const char *module);
void        netdemand_forget_failed(const char *module);

/*
 * IOCTL NUMBERS SERVED BY A MODULE. A module takes a range of numbers of
 * one group (IOCGROUP(): the letter of _IOW('r', 60, ...)) when it starts
 * and gives it back when it stops; IoctlSocket() hands those numbers to
 * its handler instead of its own switch. The core keeps a small table of
 * them (NETIOCTL_MAX), and beside it a fixed one of the ranges whose
 * module it starts - and loads - when a number of them is asked for while
 * nobody serves it (kern/net_demand.c: the filter and NAT numbers).
 *
 * The handler gets the socket, the request and its data, with the socket
 * calls fenced and after the generic checks of IoctlSocket(); it answers 0
 * or an errno, or NETIOCTL_PASS for a number it does not know - the core
 * then does with it what it did before the range was taken. With
 * NIOF_UNFENCED it is called without the fence and without the socket
 * (NULL): for a request that may wait. Such a range must be one of the
 * fixed table's, of the module's name - its slot is what keeps the module
 * from being stopped under a caller (netioctl_unregister() is then refused
 * with EBUSY while one is inside).
 *
 * netioctl_register(): EINVAL (no handler, a bad range, NIOF_UNFENCED
 * without its slot), EEXIST (a number of the range is taken), ENOSPC.
 * netioctl_unregister(): every range of that handler; ENOENT, EBUSY.
 * Both in the stack's task, fenced, as starts and stops are.
 */
#define NETIOCTL_MAX    4
#define NETIOCTL_PASS   (-1)
#define NIOF_UNFENCED   0x01

typedef int (*netioctl_handler)(struct socket *so, ULONG cmd, caddr_t data);

int     netioctl_register(int group, int first, int last, netioctl_handler handler,
                          struct netmodule *owner, int flags);
int     netioctl_unregister(netioctl_handler handler);

/*
 * IoctlSocket()'s side. NETIOCTL_GROUP(cmd): one bit tested, whether any
 * range of the request's group is taken or startable - the only cost for
 * every other request. netioctl_demand(): before the fence, the slot of the
 * fixed range the number is in, or NULL. netioctl_find(): with the fence
 * held, who serves the number now, or NULL.
 */
struct netioctl {
    UBYTE               nio_group, nio_first, nio_last, nio_flags;
    netioctl_handler    nio_handler;
    struct netmodule   *nio_owner;
};

extern UBYTE netioctl_groups[32];
#define NETIOCTL_GROUP(cmd) \
    (netioctl_groups[((cmd) >> 11) & 0x1f] & (1 << (((cmd) >> 8) & 7)))

struct netdemand      *netioctl_demand(ULONG cmd);
const struct netioctl *netioctl_find(ULONG cmd);

/*
 * SOCKET DOMAINS ON DEMAND. socket() of a family no running module has
 * attached a domain for: the module that would, or NULL (a fixed table in
 * kern/net_demand.c: PF_ROUTE, the administration module). Asked only on
 * that failure path.
 */
struct netdemand *netdomain_demand(int family);
#endif /* KERNEL */

#endif /* !NET_NETDEMAND_H */
