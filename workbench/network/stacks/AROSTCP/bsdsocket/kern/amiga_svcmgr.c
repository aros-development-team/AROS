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

extern TEXT db_path[];

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

static struct SvcProc *svc_find(CONST_STRPTR name)
{
    return (struct SvcProc *)FindName(&SvcList, name);
}

static LONG svc_launch(CONST_STRPTR name, CONST_STRPTR path, ULONG stopsig,
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

static void svc_stop_one(struct SvcProc *sp, BOOL force)
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

/* Case-insensitive match of a NUL-terminated key against a literal. */
static BOOL svc_keyeq(const char *k, const char *lit)
{
    while(*k && *lit) {
        char a = *k, b = *lit;
        if(a >= 'a' && a <= 'z') a -= 32;
        if(b >= 'a' && b <= 'z') b -= 32;
        if(a != b)
            return FALSE;
        k++; lit++;
    }
    return *k == '\0' && *lit == '\0';
}

/* Split "KEY = VALUE" in place, trimming surrounding whitespace.  Returns FALSE
 * for blank / comment (# or ;) / non-assignment lines. */
static BOOL svc_cfg_kv(char *line, char **key, char **val)
{
    char *p = line, *eq, *e;

    while(*p == ' ' || *p == '\t')
        p++;
    if(*p == '\0' || *p == '\n' || *p == '\r' || *p == '#' || *p == ';')
        return FALSE;

    eq = p;
    while(*eq && *eq != '=' && *eq != '\n')
        eq++;
    if(*eq != '=')
        return FALSE;

    *key = p;
    e = eq;                             /* terminate + rtrim the key */
    *eq = '\0';
    while(e > p && (e[-1] == ' ' || e[-1] == '\t'))
        *--e = '\0';

    p = eq + 1;                         /* value: ltrim then rtrim */
    while(*p == ' ' || *p == '\t')
        p++;
    *val = p;
    e = p;
    while(*e && *e != '\n' && *e != '\r')
        e++;
    while(e > p && (e[-1] == ' ' || e[-1] == '\t'))
        e--;
    *e = '\0';
    return TRUE;
}

/* Interpret a config value as a boolean (dflt when empty).  False = no / false
 * / 0 / off; anything else is true. */
static BOOL svc_cfg_bool(const char *v, BOOL dflt)
{
    char c;

    if(v == NULL || v[0] == '\0')
        return dflt;
    c = v[0];
    if(c == 'n' || c == 'N' || c == 'f' || c == 'F' || c == '0')
        return FALSE;
    if((c == 'o' || c == 'O') && (v[1] == 'f' || v[1] == 'F'))   /* "off" */
        return FALSE;
    return TRUE;
}

/* Read one service config file (the current dir is db/services.d) and bring the
 * service into line with it.  The service name is the file name; the file holds
 * its management config:
 *   Path=<executable>              (required)
 *   Order=<n>                      (bring-up priority; higher launches first)
 *   StopSig=<bit>                  (signal bit to stop it; default CTRL-C = 12)
 *   Policy=signal|restart|ignore   (reload policy; default signal)
 *   Enabled=yes|no                 (default yes)
 * An enabled service not yet running is launched; a disabled service that IS
 * running is stopped.  So a service is turned off by setting Enabled=no (it
 * takes effect at the next boot or reload) WITHOUT deleting its script. */
static void svc_apply_cfg(CONST_STRPTR name)
{
    BPTR  fh;
    char  line[256];
    char  path[256];
    ULONG stopsig = 0, policy = NSRP_SIGNAL;
    BOOL  havepath = FALSE, enabled = TRUE;
    struct SvcProc *existing;

    if(name[0] == '\0' || name[0] == '.')   /* skip "", hidden, "." / ".." */
        return;

    fh = Open((STRPTR)name, MODE_OLDFILE);   /* relative to services.d */
    if(fh == BNULL)
        return;

    while(FGets(fh, (STRPTR)line, sizeof(line))) {
        char *key = NULL, *val = NULL;

        if(!svc_cfg_kv(line, &key, &val))
            continue;

        if(svc_keyeq(key, "Path")) {
            int i;
            for(i = 0; i < (int)sizeof(path) - 1 && val[i]; i++)
                path[i] = val[i];
            path[i]  = '\0';
            havepath = (i > 0);
        } else if(svc_keyeq(key, "StopSig")) {
            LONG n = 0;
            const char *v = val;
            while(*v >= '0' && *v <= '9')
                n = n * 10 + (*v++ - '0');
            if(n >= 0 && n < 32)
                stopsig = 1UL << n;
        } else if(svc_keyeq(key, "Policy")) {
            if(val[0] == 'r' || val[0] == 'R')
                policy = NSRP_RESTART;
            else if(val[0] == 'i' || val[0] == 'I')
                policy = NSRP_IGNORE;
            else
                policy = NSRP_SIGNAL;
        } else if(svc_keyeq(key, "Enabled")) {
            enabled = svc_cfg_bool(val, TRUE);
        }
        /* Order= sequences the scan (svcmgr_launch_all), not this function;
         * Name= and ConfigTool= are UI-side keys (Network prefs). */
    }
    Close(fh);

    existing = svc_find(name);
    if(enabled) {
        if(existing)                        /* already running */
            return;
        if(havepath)
            svc_launch(name, path, stopsig, policy);
        else
            __log(LOG_ERR, "servicemgr: service '%s' has no Path", name);
    } else if(existing) {
        /* Disabled but running: keep the script, stop the service. */
        __log(LOG_NOTICE, "servicemgr: service '%s' disabled, stopping", name);
        svc_stop_one(existing, FALSE);
    }
}

/* Read just a service file's Order= (launch priority, higher first; 0 when
 * absent).  The current dir is db/services.d. */
static LONG svc_read_order(CONST_STRPTR name)
{
    BPTR fh;
    char line[256];
    LONG order = 0;

    fh = Open((STRPTR)name, MODE_OLDFILE);
    if(fh == BNULL)
        return 0;
    while(FGets(fh, (STRPTR)line, sizeof(line))) {
        char *key = NULL, *val = NULL;

        if(!svc_cfg_kv(line, &key, &val))
            continue;
        if(svc_keyeq(key, "Order")) {
            const char *v = val;
            LONG n = 0;

            while(*v >= '0' && *v <= '9')
                n = n * 10 + (*v++ - '0');
            order = n;
        }
    }
    Close(fh);
    return order;
}

#define SVC_MAXSCAN 32

struct SvcScanEnt {
    char name[108];                 /* fib_FileName size */
    LONG order;
};

void svcmgr_launch_all(void)
{
    BPTR dblock, svclock, old;
    struct FileInfoBlock *fib;
    struct SvcScanEnt *found;
    int nfound = 0, i, j;

    if(!SvcMgrReady)
        return;

    dblock = Lock(db_path, ACCESS_READ);
    if(dblock == BNULL)
        return;
    old = CurrentDir(dblock);
    /* One file per service under db/services.d/.  (db/services itself is the
     * IANA port-name database read by getservbyname, hence the ".d" dir name
     * to avoid colliding with it.) */
    svclock = Lock("services.d", ACCESS_READ);
    if(svclock == BNULL) {
        CurrentDir(old);
        UnLock(dblock);
        return;             /* no services.d -> no external service daemons */
    }

    found = AllocVec(sizeof(struct SvcScanEnt) * SVC_MAXSCAN,
                     MEMF_PUBLIC | MEMF_CLEAR);
    fib = AllocDosObject(DOS_FIB, NULL);
    if(fib != NULL && Examine(svclock, fib)) {
        CurrentDir(svclock);    /* so Open(name) resolves inside services.d */
        D(bug("[AROSTCP](amiga_svcmgr.c) svcmgr_launch_all(): scanning db/services.d\n"));
        while(ExNext(svclock, fib)) {
            if(fib->fib_DirEntryType > 0)       /* skip subdirectories */
                continue;
            if(found != NULL && nfound < SVC_MAXSCAN) {
                int k;

                /* collect first: launches are sequenced by Order= below */
                for(k = 0; k < (int)sizeof(found[0].name) - 1 && fib->fib_FileName[k]; k++)
                    found[nfound].name[k] = fib->fib_FileName[k];
                found[nfound].name[k] = '\0';
                found[nfound].order = svc_read_order(found[nfound].name);
                nfound++;
            } else
                svc_apply_cfg(fib->fib_FileName);   /* overflow: scan order */
        }
        /* Launch highest Order first; insertion sort keeps equal orders in
         * scan order. */
        for(i = 1; i < nfound; i++) {
            struct SvcScanEnt tmp = found[i];

            for(j = i; j > 0 && found[j - 1].order < tmp.order; j--)
                found[j] = found[j - 1];
            found[j] = tmp;
        }
        for(i = 0; i < nfound; i++)
            svc_apply_cfg(found[i].name);
    }
    if(fib != NULL)
        FreeDosObject(DOS_FIB, fib);
    FreeVec(found);
    CurrentDir(old);
    UnLock(svclock);
    UnLock(dblock);
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
        svc_stop_one(sp, force);
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
