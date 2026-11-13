/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The core's side of the configuration readers (kern/netconfig_ops.h):
 * the demand slot of module "netconfig", having it loaded for a read and
 * dropped after, the functions the core always called by these names, and
 * the interface lines marked DEFER that wait for their device.
 *
 * WHO READS WHEN, AND IN WHICH TASK
 *
 *  - start-up, the stack's task: kern/amiga_main.c init_all() acquires the
 *    reader before readconfig() and releases it after the last read of
 *    the start (the service files, net_services_register()). Between the
 *    two, readconfig(), init_netdb() and svcmgr_launch_all() find it
 *    running and call through its table.
 *  - NCMD_RELOAD, the stack's task (control_poll()): acquired before
 *    net_reload() takes the old configuration down - so that a reader
 *    that cannot be had refuses the reload with nothing changed - and
 *    released after. netdb_reload() and svcmgr_launch_all() run inside
 *    net_reload(), fenced.
 *  - an ARexx command, NETTRACE's task (kern/amiga_rexx.c): ADD, RESET and
 *    SET WITH=file enter the slot - the stack's task loads the reader for
 *    them (NCMD_MODULE_DEMAND) - and on leaving ask the stack's task to
 *    drop it again (a message to the control port).
 *  - a deferred interface line, the stack's task in timer_poll(), at
 *    splsoftclock: only if the reader runs - it is kept while a line waits
 *    - and never loaded or unloaded there: loading takes the socket calls'
 *    fence, which an application takes before the spl level, so taking it
 *    with the level raised could deadlock. When the last line is applied
 *    the timer asks for the reader to be dropped, as NETTRACE does.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>
#include <aros/atomic.h>
#include <aros/symbolsets.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <proto/dos.h>

#include <net/if.h>
#include <protos/net/if_protos.h>
#include <net/netmodule.h>
#include <net/netdemand.h>
#include <bsdsocket/netcontrol.h>
#include <kern/amiga_config.h>
#include <kern/amiga_netdb.h>
#include <kern/netconfig_ops.h>

extern struct SignalSemaphore syscall_semaphore;    /* the fence (api/amiga_api.c) */
extern struct Task *AROSTCP_Task;                   /* the stack's own (kern/amiga_main.c) */

UBYTE ERR_NOCONFIG[] = "Configuration reader not available";

/* The demand slot: the reader's table while the module runs. */
static struct netdemand netconfig_demand = { NETCONFIG_MODULE };
NETDEMAND_SLOT(netconfig_demand);

/*
 * The modules of the stack program (kern/net_module.c). The reader is
 * needed before netmodule_start_builtin() makes them known by name, so it
 * is looked for here, once.
 */
DECLARESET(NETMODULES)

static BOOL netconfig_looked = FALSE;
static BOOL netconfig_inprogram = FALSE;    /* built in: stopped after a read, not unloaded */

static void
netconfig_know_builtin(void)
{
    struct netmodule *nm;
    long pos;

    if(netconfig_looked)
        return;
    netconfig_looked = TRUE;
    ForeachElementInSet(SETNAME(NETMODULES), 1, pos, nm)
        if(strcmp(nm->nm_name, NETCONFIG_MODULE) == 0) {
            netmodule_know(nm);
            netconfig_inprogram = TRUE;
        }
}

/*
 * The reader running, before a read in the stack's task. A module of the
 * program is started; otherwise its library is loaded and started
 * (kern/net_modload.c: the disk is read outside the fence). The test
 * switch ENV:AROSTCP/Without_netconfig keeps it out either way, as it
 * keeps any module out (netdemand_run()): the tests prove the "not
 * installed" answer with it.
 */
int
netconfig_acquire(void)
{
    struct netmodule *nm;
    char lib[48];
    int error = 0;

    netconfig_know_builtin();
    if(netmodule_leftout(NETCONFIG_MODULE, sizeof(NETCONFIG_MODULE) - 1)) {
        __log(LOG_ERR, "module %s is left out (AROSTCP/Without_%s is set).",
              NETCONFIG_MODULE, NETCONFIG_MODULE);
        return (ENOENT);
    }

    /* only this task starts, stops, loads and unloads: nm stays valid without the fence */
    ObtainSemaphore(&syscall_semaphore);
    if((nm = netmodule_known(NETCONFIG_MODULE)) != NULL)
        error = netmodule_use(nm);
    ReleaseSemaphore(&syscall_semaphore);

    if(nm == NULL) {
        if(!netmodule_libname(NETCONFIG_MODULE, sizeof(NETCONFIG_MODULE) - 1, NETMODULE_KIND_ATCP,
                              lib, sizeof(lib)))
            return (ENOENT);
        /* the loader says in the log what is wrong with the file */
        if((error = netmodule_load(lib, &nm)) == 0 && strcmp(nm->nm_name, NETCONFIG_MODULE) != 0) {
            __log(LOG_ERR, "%s holds module %s, not %s.", lib, nm->nm_name, NETCONFIG_MODULE);
            (void)netmodule_unload(nm);
            error = ENOENT;
        }
    }
    if(error == 0)
        netdemand_forget_failed(NETCONFIG_MODULE);
    else
        __log(LOG_ERR, "module %s could not be started (error %ld): the configuration "
              "cannot be read.", NETCONFIG_MODULE, (long)error);
    return (error);
}

/*
 * MODULES= names the reader (MODULES=NETCONFIG): it stays, as every module
 * that item names does - for a machine that would rather keep the memory
 * taken than read the disk at a reload. (The item's own parser in
 * kern/net_module.c is that file's; the same reading of the list.)
 */
static int
netconfig_listed(void)
{
    const char *p = netmodule_modules, *n;

    if(p == NULL)
        return 0;
    while(*p) {
        while(*p == ',' || *p == ' ' || *p == '\t')
            p++;
        for(n = NETCONFIG_MODULE; *n && *p && (*p | 0x20) == *n; p++, n++)
            ;
        if(*n == '\0' && (*p == '\0' || *p == ',' || *p == ' ' || *p == '\t'))
            return 1;
        while(*p && *p != ',')
            p++;
    }
    return 0;
}

/*
 * The read is done: the reader leaves memory (a module of the program is
 * stopped). Not while a deferred interface line waits for it, not when
 * MODULES= names it, and not while a task is inside it - NETTRACE in an
 * ARexx command; that task asks for it to be dropped when it leaves.
 */
void
netconfig_release(void)
{
    struct netmodule *nm;
    char lib[48];
    int pending, error = 0;

    if(netconfig_listed())
        return;
    if((pending = netdb_defer_pending()) != 0) {
        __log(LOG_NOTICE, "module %s stays: %ld DEFER interface(s) wait.",
              NETCONFIG_MODULE, (long)pending);
        return;
    }
    ObtainSemaphore(&syscall_semaphore);
    /* by its name, in any state; else its library file in the loader's list */
    if((nm = netmodule_known(NETCONFIG_MODULE)) == NULL &&
            netmodule_libname(NETCONFIG_MODULE, sizeof(NETCONFIG_MODULE) - 1, NETMODULE_KIND_ATCP,
                              lib, sizeof(lib)))
        nm = netmodule_loaded_file(FilePart(lib));
    if(nm != NULL && nm->nm_library == NULL && (nm->nm_flags & NMF_STARTED))
        error = netmodule_unregister(nm);   /* part of the program: stopped */
    ReleaseSemaphore(&syscall_semaphore);
    if(nm != NULL && nm->nm_library != NULL)
        error = netmodule_unload(nm);
    if(error == EBUSY)
        __log(LOG_NOTICE, "module %s: a task is inside it; it is dropped when that task "
              "leaves.", NETCONFIG_MODULE);
}

/*
 * Ask the stack's task to drop the reader, from a task that must not do
 * it itself.
 *
 * Another task (NETTRACE after an ARexx command): it asks and WAITS for
 * the answer, as netdemand_load() does - its message and reply port on its
 * own stack. Each task that leaves the reader with nobody else inside asks
 * once, so the last one out always has it dropped, and nothing is left
 * waiting in the port for some later message. (A single message posted
 * without waiting could be found still in flight - being refused because
 * this same task had entered again - and then not be posted again.) It
 * holds nothing the stack's task takes: not the fence (checked), not NDB
 * (the reader's calls take it and give it back), no socket.
 *
 * The stack's own timer (spl raised, see the head of this file) must not
 * wait for its own task: it posts the one static message, without a reply
 * port (the stack's ReplyMsg() then marks it free again), served by the
 * main loop when the timer is done. Only the timer posts it.
 *
 * STOP for a module of the program, which cannot be unloaded. The control
 * port is found by its name: the core keeps its pointer in
 * kern/net_demand.c, which hands it to no one.
 */
static struct NetControlModuleMsg netconfig_dropmsg;

static void
netconfig_drop_msg(struct NetControlModuleMsg *mm, struct MsgPort *reply)
{
    bzero(mm, sizeof(*mm));
    mm->ncmm_Msg.ncm_Msg.mn_Node.ln_Type = NT_MESSAGE;
    mm->ncmm_Msg.ncm_Msg.mn_ReplyPort = reply;
    mm->ncmm_Msg.ncm_Msg.mn_Length = sizeof(*mm);
    mm->ncmm_Msg.ncm_Command = netconfig_inprogram ? NCMD_MODULE_STOP : NCMD_MODULE_UNLOAD;
    strcpy(mm->ncmm_Name, NETCONFIG_MODULE);
}

static void
netconfig_drop_later(void)
{
    struct NetControlModuleMsg mm;
    struct MsgPort reply, *port;
    struct Task *me = FindTask(NULL);

    if(netdb_defer_pending() || netconfig_listed())
        return;

    if(me == AROSTCP_Task) {
        Forbid();
        if(netconfig_dropmsg.ncmm_Msg.ncm_Msg.mn_Node.ln_Type != NT_MESSAGE &&
                (port = FindPort((STRPTR)AROSTCP_CTRLPORT_NAME)) != NULL) {
            netconfig_drop_msg(&netconfig_dropmsg, NULL);
            PutMsg(port, &netconfig_dropmsg.ncmm_Msg.ncm_Msg);
        }
        Permit();
        return;
    }

    /* another task is inside: it asks when it leaves */
    if(netconfig_demand.nd_busy != 0 || syscall_semaphore.ss_Owner == me)
        return;
    netconfig_drop_msg(&mm, &reply);
    bzero(&reply, sizeof(reply));
    reply.mp_Node.ln_Type = NT_MSGPORT;
    reply.mp_Flags = PA_SIGNAL;
    reply.mp_SigBit = SIGB_SINGLE;
    reply.mp_SigTask = me;
    NEWLIST(&reply.mp_MsgList);
    SetSignal(0, SIGF_SINGLE);

    Forbid();
    if((port = FindPort((STRPTR)AROSTCP_CTRLPORT_NAME)) != NULL)
        PutMsg(port, &mm.ncmm_Msg.ncm_Msg);
    Permit();
    if(port != NULL) {
        WaitPort(&reply);
        GetMsg(&reply);
    }
}

/*
 * One call through the reader's table. From a task that is not the
 * stack's, the slot has the stack's task load the reader (net/netdemand.h)
 * and leaving asks for it to be dropped again. In the stack's task the
 * reads are bracketed by netconfig_acquire() and netconfig_release(), and
 * leaving drops nothing.
 */
const struct netconfig_ops *
netconfig_enter(int *errorp)
{
    const void *ops = NULL;
    int error;

    error = netdemand_enter_error(&netconfig_demand, &ops);
    if(errorp != NULL)
        *errorp = error;
    return (error == 0 ? (const struct netconfig_ops *)ops : NULL);
}

void
netconfig_leave(void)
{
    netdemand_leave(&netconfig_demand);
    if(FindTask(NULL) != AROSTCP_Task)
        netconfig_drop_later();
}

/*
 * The table if the reader runs now, entered (netconfig_leave() after) -
 * and nothing loaded when it does not: for the timer. Counted first, read
 * after, as netdemand_enter_error() does, so that a stop cannot miss it.
 */
const struct netconfig_ops *
netconfig_running(void)
{
    const void *ops;

    AROS_ATOMIC_INC(netconfig_demand.nd_busy);
    if((ops = netconfig_demand.nd_ops) == NULL)
        AROS_ATOMIC_DEC(netconfig_demand.nd_busy);
    return ((const struct netconfig_ops *)ops);
}

/*
 * An error text of the reader into the caller's reply buffer (the core's
 * own errors are written there too, by csprintf() and Fault()): the
 * reader's constants leave memory with it, and NETTRACE reads the text
 * after the reader may have been dropped. Only for a call that failed:
 * after one that did not, *errstrp need not point anywhere.
 */
void
netconfig_keep_error(UBYTE **errstrp, struct CSource *res)
{
    UBYTE *s = *errstrp;
    size_t n;

    if(s == NULL || res == NULL || res->CS_Buffer == NULL || res->CS_Length < 2 ||
            s == res->CS_Buffer)
        return;
    n = strnlen((const char *)s, res->CS_Length - 1);
    memmove(res->CS_Buffer, s, n);
    res->CS_Buffer[n] = '\0';
    *errstrp = res->CS_Buffer;
}

/* ---- the functions the core always called by these names ---------------- */

/*
 * The command line and db/general.config, at start-up (kern/amiga_main.c,
 * after netconfig_acquire()).
 */
BOOL
readconfig(void)
{
    const struct netconfig_ops *ops;
    BOOL ok;
    int error;

    /* (kern/amiga_main.c has it running, and says so when it cannot) */
    if((ops = netconfig_enter(&error)) == NULL)
        return FALSE;
    ok = ops->nco_readconfig();
    netconfig_leave();
    return ok;
}

/*
 * SET WITH=file: a file of general.config's syntax. In a configuration
 * file (the reader runs) and from ARexx (NETTRACE). The item of the
 * command line is the core's to read, as the rest of SET's syntax is.
 */
LONG
read_sets(struct CSource *args, UBYTE **errstrp, struct CSource *res)
{
    const struct netconfig_ops *ops;
    UBYTE *buf = res->CS_Buffer;
    LONG retval;

    if(ReadItem(buf, res->CS_Length, args) < 0) {
        *errstrp = ERR_SYNTAX;
        return RETURN_WARN;
    }
    if((ops = netconfig_enter(NULL)) == NULL) {
        *errstrp = ERR_NOCONFIG;
        return RETURN_ERROR;
    }
    if((retval = ops->nco_parsefile(buf, errstrp, res)) != RETURN_OK)
        netconfig_keep_error(errstrp, res);
    netconfig_leave();
    return retval;
}

/* init_netdb()'s read of db/netdb (the rest of init_netdb() is the core's) */
LONG
netconfig_netdb_init(void)
{
    const struct netconfig_ops *ops;
    LONG retval;
    int error;

    if((ops = netconfig_enter(&error)) == NULL) {
        __log(LOG_ERR, "netdb: %s (error %ld).", ERR_NOCONFIG, (long)error);
        return RETURN_FAIL;
    }
    retval = ops->nco_netdb_init();
    netconfig_leave();
    return retval;
}

/* The "interfaces" service of a reload (kern/amiga_netsvc.c), fenced. */
LONG
netdb_reload(void)
{
    const struct netconfig_ops *ops;
    LONG retval;
    int error;

    if((ops = netconfig_enter(&error)) == NULL) {
        __log(LOG_ERR, "netdb reload: %s (error %ld).", ERR_NOCONFIG, (long)error);
        return RETURN_FAIL;
    }
    retval = ops->nco_netdb_reload();
    netconfig_leave();
    return retval;
}

/* ARexx ADD (kern/amiga_config.c, rexx_parse_funs[]) */
LONG
do_netdb(struct CSource *csarg, UBYTE **errstrp, struct CSource *res)
{
    const struct netconfig_ops *ops;
    LONG retval;

    if((ops = netconfig_enter(NULL)) == NULL) {
        *errstrp = ERR_NOCONFIG;
        return RETURN_ERROR;
    }
    if((retval = ops->nco_netdb_add(csarg, errstrp, res)) != RETURN_OK)
        netconfig_keep_error(errstrp, res);
    netconfig_leave();
    return retval;
}

/* ARexx RESET */
LONG
reset_netdb(struct CSource *cs, UBYTE **errstrp, struct CSource *res)
{
    const struct netconfig_ops *ops;
    LONG retval;

    if((ops = netconfig_enter(NULL)) == NULL) {
        *errstrp = ERR_NOCONFIG;
        return RETURN_ERROR;
    }
    if((retval = ops->nco_netdb_reset(cs, errstrp, res)) != RETURN_OK)
        netconfig_keep_error(errstrp, res);
    netconfig_leave();
    return retval;
}

/* ---- interface lines marked DEFER ---------------------------------------- */

/*
 * Interfaces marked DEFER whose device did not open: hardware that turns up
 * later (a hot-plugged adapter, a wireless or Bluetooth link). Their
 * interface line is kept here, in the core, and tried again from
 * netdb_defer_timer() until the device opens; from then on the line is
 * applied exactly as at startup, by the reader, which stays loaded while
 * a line waits.
 */
struct deferred_if {
    struct MinNode di_Node;
    char           di_Name[IFNAMSIZ];
    ULONG          di_Length;
    char           di_Line[1];      /* the interface line, '\n' terminated */
};

static struct MinList deferred_ifs;
static BOOL deferred_ready = FALSE;

/*
 * The list is changed by the reader in the task that reads - the stack's,
 * or NETTRACE in an ARexx ADD, holding NDB - and walked by the stack's
 * task, which may not hold NDB then (a reload forgets the lines before it
 * reads; a count): each change and each count is made in one piece.
 */

/* name NULL: every line (a reload reads them anew; the stack ends) */
void
netdb_defer_forget(const char *name)
{
    struct deferred_if *di, *next;

    if(!deferred_ready)
        return;
    Forbid();
    for(di = (struct deferred_if *)deferred_ifs.mlh_Head;
            (next = (struct deferred_if *)di->di_Node.mln_Succ); di = next) {
        if(name == NULL || strncmp(di->di_Name, name, IFNAMSIZ) == 0) {
            Remove((struct Node *)di);
            FreeVec(di);
        }
    }
    Permit();
}

/* the reader keeps a line (it replaces one of the same interface); FALSE: no memory */
int
netdb_defer_add(const char *name, const char *line, ULONG length)
{
    struct deferred_if *di;

    if(!(di = AllocVec(sizeof(*di) + length + 1, MEMF_PUBLIC | MEMF_CLEAR)))
        return FALSE;
    strncpy(di->di_Name, name, IFNAMSIZ - 1);
    memcpy(di->di_Line, line, length);
    if(length == 0 || di->di_Line[length - 1] != '\n')
        di->di_Line[length++] = '\n';   /* ReadArgs() depends on it */
    di->di_Length = length;

    Forbid();
    if(!deferred_ready) {
        NewList((struct List *)&deferred_ifs);
        deferred_ready = TRUE;
    }
    netdb_defer_forget(name);       /* (Forbid() nests) */
    AddTail((struct List *)&deferred_ifs, (struct Node *)di);
    Permit();
    return TRUE;
}

/* how many lines wait: the reader is not dropped while one does */
int
netdb_defer_pending(void)
{
    struct MinNode *n;
    int count = 0;

    if(!deferred_ready)
        return 0;
    Forbid();
    for(n = deferred_ifs.mlh_Head; n->mln_Succ; n = n->mln_Succ)
        count++;
    Permit();
    return count;
}

/*
 * Periodic (timer framework, kern/amiga_time.c): try the deferred
 * interfaces again. A line whose device now opens goes through the
 * reader's interface parser as it would have at startup, so its
 * addresses, DHCP and routes are set up the same way. The reader is not
 * loaded here (see the head of this file): it runs while a line waits.
 */
void
netdb_defer_timer(void)
{
    const struct netconfig_ops *ops;
    struct deferred_if *di, *next;
    UBYTE result[REPLYBUFLEN + 1];
    struct CSource res;
    UBYTE *errstr;

    if(!deferred_ready || NDB == NULL || IsListEmpty((struct List *)&deferred_ifs))
        return;
    if((ops = netconfig_running()) == NULL)
        return;
    /* never wait for the database here: whoever holds it may be waiting on us */
    if(ATTEMPT_NDB(NDB)) {
        for(di = (struct deferred_if *)deferred_ifs.mlh_Head;
                (next = (struct deferred_if *)di->di_Node.mln_Succ); di = next) {
            res.CS_Buffer = result;
            res.CS_Length = sizeof(result);
            res.CS_CurChr = 0;
            errstr = NULL;

            if(ops->nco_defer_retry(di->di_Line, di->di_Length, &errstr, &res) != 0)
                continue;   /* its device still does not open */
            /* it opened (or the line turned out unusable): either way done */
            if(ifunit(di->di_Name))
                __log(LOG_NOTICE, "%s: device available, interface started.\n", di->di_Name);
            else
                __log(LOG_ERR, "%s: deferred interface dropped (%s).\n", di->di_Name,
                      errstr ? (char *)errstr : "not created");
            Forbid();
            Remove((struct Node *)di);
            Permit();
            FreeVec(di);
        }
        UNLOCK_NDB(NDB);
    }
    netconfig_leave();
    /* the last line is applied: the reader can go (the stack's task, outside the timer) */
    if(IsListEmpty((struct List *)&deferred_ifs))
        netconfig_drop_later();
}
