/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Service manager - launches external service daemons configured under
 * db/services.d/ (one file per service) and owns their process lifecycle.
 * See amiga_svcmgr.h.
 */

#include <conf.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <libraries/netservice.h>
#include <api/netservice_api.h>
#include <kern/amiga_svcmgr.h>
#include <kern/amiga_log.h>
#include <kern/netconfig_ops.h>

/* A launched daemon.  The seglist is freed by DOS on exit (NP_FreeSeglist
 * defaults TRUE), so we track only the process and an exit flag the NP_ExitCode
 * trampoline sets from the daemon's context as it dies. */
struct SvcProc {
    struct Node      sp_Node;       /* ln_Name = sp_NameBuf */
    struct Process  *sp_Proc;
    ULONG            sp_StopSig;
    ULONG            sp_Policy;      /* NSRP_SIGNAL (survive reload) / NSRP_RESTART */
    volatile LONG    sp_Exited;
    char             sp_NameBuf[32];
};

#define SVC_STOP_WAIT_STEP   5      /* ticks between exit polls */
#define SVC_STOP_WAIT_MAX  250      /* ~5s cap waiting for a daemon to exit */

static struct List SvcList;
static BOOL        SvcMgrReady = FALSE;

/* -------------------------------------------------------------------------
 * Process lifecycle
 * ------------------------------------------------------------------------- */

/* NP_ExitCode trampoline - runs in the daemon's context as it exits. */
static void svc_exited(IPTR rc, IPTR data)
{
    struct SvcProc *sp = (struct SvcProc *)data;
    (void)rc;
    if(sp)
        sp->sp_Exited = TRUE;
}

struct SvcProc *svcmgr_find(CONST_STRPTR name)
{
    return (struct SvcProc *)FindName(&SvcList, name);
}

LONG svcmgr_launch(CONST_STRPTR name, CONST_STRPTR path, ULONG stopsig,
                   ULONG policy)
{
    BPTR seg;
    struct SvcProc *sp;
    struct Process *proc;
    int i;

    seg = LoadSeg((STRPTR)path);
    if(seg == BNULL) {
        __log(LOG_ERR, "servicemgr: cannot load service '%s' from '%s'", name, path);
        return -1;
    }

    sp = AllocMem(sizeof(*sp), MEMF_PUBLIC | MEMF_CLEAR);
    if(sp == NULL) {
        UnLoadSeg(seg);
        return -1;
    }
    for(i = 0; i < (int)sizeof(sp->sp_NameBuf) - 1 && name[i]; i++)
        sp->sp_NameBuf[i] = name[i];
    sp->sp_NameBuf[i]  = '\0';
    sp->sp_Node.ln_Name = sp->sp_NameBuf;
    sp->sp_StopSig      = stopsig ? stopsig : SIGBREAKF_CTRL_C;
    sp->sp_Policy       = policy;
    sp->sp_Exited       = FALSE;

    /* NP_FreeSeglist defaults TRUE: DOS unloads seg when the daemon exits. */
    proc = CreateNewProcTags(NP_Seglist,    (IPTR)seg,
                             NP_Name,       (IPTR)sp->sp_NameBuf,
                             NP_Cli,        TRUE,
                             NP_ConsoleTask, (IPTR)NULL,
                             NP_ExitCode,   (IPTR)svc_exited,
                             NP_ExitData,   (IPTR)sp,
                             TAG_DONE);
    if(proc == NULL) {
        UnLoadSeg(seg);
        FreeMem(sp, sizeof(*sp));
        __log(LOG_ERR, "servicemgr: cannot start service '%s'", name);
        return -1;
    }

    sp->sp_Proc = proc;
    AddTail(&SvcList, &sp->sp_Node);
    __log(LOG_NOTICE, "servicemgr: launched service '%s' (%s)", name, path);
    return 0;
}

void svcmgr_stop_one(struct SvcProc *sp, BOOL force)
{
    int waited;

    (void)force;
    D(bug("[AROSTCP](amiga_svcmgr.c) stopping '%s'\n", sp->sp_NameBuf));

    /* Ask the daemon to stop; it unregisters, closes its bases and exits. */
    Signal((struct Task *)sp->sp_Proc, sp->sp_StopSig);

    for(waited = 0; !sp->sp_Exited && waited < SVC_STOP_WAIT_MAX;
            waited += SVC_STOP_WAIT_STEP)
        Delay(SVC_STOP_WAIT_STEP);

    if(!sp->sp_Exited) {
        /* Daemon ignored the stop signal.  Leave the record linked (the exit
         * trampoline still references it) rather than freeing under it. */
        __log(LOG_ERR, "servicemgr: service '%s' did not exit on stop", sp->sp_NameBuf);
        return;
    }

    Remove(&sp->sp_Node);
    FreeMem(sp, sizeof(*sp));
}

/* -------------------------------------------------------------------------
 * Per-service config (db/services.d/<name>)
 * ------------------------------------------------------------------------- */

/*
 * The service files are read by the configuration reader (module
 * "netconfig", kern/netconfig_module.c), which the stack's task has loaded
 * whenever this runs: at start-up (net_services_register()) and in a
 * reload (the "servicemgr" service's start). It brings each service into
 * line with its file through svcmgr_find(), svcmgr_launch() and
 * svcmgr_stop_one() above; the records and the processes stay here.
 */
void svcmgr_launch_all(void)
{
    const struct netconfig_ops *ops;
    int error;

    if(!SvcMgrReady)
        return;
    if((ops = netconfig_enter(&error)) == NULL) {
        __log(LOG_ERR, "servicemgr: db/services.d is not read: the configuration "
              "reader is not available (error %ld)", (long)error);
        return;
    }
    ops->nco_services_scan();
    netconfig_leave();
}

void svcmgr_stop_all(BOOL force)
{
    struct Node *node, *next;

    if(!SvcMgrReady)
        return;

    D(bug("[AROSTCP](amiga_svcmgr.c) svcmgr_stop_all(force=%ld)\n", (long)force));
    for(node = SvcList.lh_Head; (next = node->ln_Succ); node = next) {
        struct SvcProc *sp = (struct SvcProc *)node;

        /* A reload (force==FALSE) only kills RESTART-policy daemons; SIGNAL and
         * IGNORE daemons stay running (SIGNAL ones are told to quiesce/resume
         * through netservices).  A shutdown (force==TRUE) stops everything. */
        if(!force && sp->sp_Policy != NSRP_RESTART)
            continue;
        svcmgr_stop_one(sp, force);
    }
}

/* -------------------------------------------------------------------------
 * Netservice integration ("servicemgr")
 * ------------------------------------------------------------------------- */

static LONG svcmgr_svc_start(struct NetService *s)
{
    (void)s;
    svcmgr_launch_all();
    return 0;
}

static LONG svcmgr_svc_stop(struct NetService *s, BOOL force)
{
    (void)s;
    svcmgr_stop_all(force);
    return 0;
}

static const struct NetServiceOps svcmgr_ops = { svcmgr_svc_start, svcmgr_svc_stop };
static struct NetService svc_svcmgr;

void svcmgr_register(void)
{
    NewList(&SvcList);
    SvcMgrReady = TRUE;

    svc_svcmgr.ns_Node.ln_Name = (char *)"servicemgr";
    svc_svcmgr.ns_Node.ln_Pri  = NSPRI_SVCMGR;
    svc_svcmgr.ns_Ops          = &svcmgr_ops;
    netservice_register(&svc_svcmgr);
    svc_svcmgr.ns_State = NSSTATE_RUNNING;   /* boot brings daemons up below */

    /* Boot-time launch (net_services_register runs after api_show(), so
     * bsdsocket/netservices are already visible for the daemons to open). */
    svcmgr_launch_all();
}
