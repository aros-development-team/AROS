/*
 * Copyright (C) 1993 AmiTCP/IP Group, <amitcp-group@hut.fi>
 *                    Helsinki University of Technology, Finland.
 *                    All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
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
#include <sys/systm.h>
#include <sys/synch.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <stdio.h>
#include <version.h>
/*#include <string.h>*/

#include <signal.h>		/* from the C compiler's includes */

#include <kern/amiga_includes.h>
#include <libraries/miamipanel.h>

#include <proto/dos.h>
//#include <proto/miamipanel.h>

#include <kern/amiga_time.h>
#include <kern/amiga_log.h>
#include <net/pfil.h>

ULONG sana_init(void);
void sana_deinit(void);
BOOL sana_poll(void);

#include <kern/amiga_main_protos.h>
#include <kern/amiga_config.h>
#include <kern/amiga_gui.h>
#include <kern/amiga_netdb.h>
#include <kern/amiga_rc.h>
#include <kern/amiga_log.h>
#include <api/netservice_api.h>
#include <kern/amiga_svcmgr.h>
#include <kern/amiga_netsvc.h>
#include <bsdsocket/socketbasetags.h>   /* SBTC_RECONFIG_* reconfigure tags */
#include <net/netmodule.h>
#include <bsdsocket/netcontrol.h>       /* NetControlMsg / NCMD_* / AROSTCP_CTRLPORT_NAME */
#include <kern/kern_malloc_protos.h>
#include <api/amiga_api.h>
#include <kern/netdiag.h>
#include <kern/netconfig_ops.h>       /* the configuration reader: netconfig_acquire() ... */

/*
 * include prototypes for initialization functions
 */
#include <kern/uipc_domain_protos.h>    /* domaininit() */
#include <protos/net/if_loop_protos.h>

/*
 * The main module of the AMITCP/IP.
 */

/*
 * Global variable so AMITCP/IP task information can be utilized.
 */
struct Task *AROSTCP_Task;

extern struct ExecBase *SysBase;
extern struct Library *MasterSocketBase;
extern WORD nthLibrary;

static ULONG sanamask = 0,
             sig = 0, signalmask = 0, timermask = 0,
             breakmask = 0, controlmask = 0;

static struct MsgPort *controlport = NULL;  /* AROSTCP.ctrl: RELOAD/SHUTDOWN requests */
static BOOL  ctrl_shutdown = FALSE;          /* set by an NCMD_SHUTDOWN control message */
static BOOL  control_poll(void);

/*
 * What the stack's task is doing, readable from outside while it is stuck
 * (kern/netdiag.h). Made public at the start of main() and taken back at
 * the end of deinit_all(), the last thing done before the code can go.
 */
struct netdiag netdiag;
extern struct SignalSemaphore spl_semaphore;
extern spl_t spl_level;

static void
netdiag_start(void)
{
    netdiag.nd_size = sizeof(netdiag);
    netdiag.nd_version = NETDIAG_VERSION;
    netdiag.nd_task = AROSTCP_Task;
    netdiag.nd_spl_semaphore = &spl_semaphore;
    netdiag.nd_syscall_semaphore = &syscall_semaphore;
    netdiag.nd_spl_level = &spl_level;
    netdiag.ml_where = NDW_OUTSIDE;
    netdiag.init_stage = NDI_MAIN;

    /* a port nobody sends to: only its name and its place in the list matter */
    netdiag.nd_port.mp_Node.ln_Type = NT_MSGPORT;
    netdiag.nd_port.mp_Node.ln_Pri = -128;
    netdiag.nd_port.mp_Node.ln_Name = (char *)NETDIAG_PORTNAME;
    netdiag.nd_port.mp_Flags = PA_IGNORE;
    netdiag.nd_port.mp_SigTask = NULL;
    NEWLIST(&netdiag.nd_port.mp_MsgList);
    AddPort(&netdiag.nd_port);
}

static void
netdiag_end(void)
{
    /* once: a panic() runs deinit_all() itself */
    if(netdiag.nd_port.mp_Node.ln_Name != NULL) {
        RemPort(&netdiag.nd_port);
        netdiag.nd_port.mp_Node.ln_Name = NULL;
    }
}

/* the signals just taken, each counted (ml_sig[]): four tests */
static inline void
netdiag_count(ULONG got)
{
    if(got & timermask)
        netdiag.ml_sig[NDS_TIMER]++;
    if(got & sanamask)
        netdiag.ml_sig[NDS_SANA]++;
    if(got & controlmask)
        netdiag.ml_sig[NDS_CONTROL]++;
    if(got & breakmask)
        netdiag.ml_sig[NDS_BREAK]++;
}

UBYTE *taskname = NULL;
/*BPTR db_lock = NULL;*/
ULONG EnableDebug = 0;
BOOL  initialized = FALSE;

STRPTR dhclient_name = NULL;

TEXT interfaces_path[FILENAME_MAX];
TEXT netdb_path[FILENAME_MAX];
TEXT db_path[FILENAME_MAX];
TEXT config_path[FILENAME_MAX];
UBYTE logfiledefname[FILENAME_MAX];

/*
 * DHCP client management (which client, where it lives, how it is launched) is
 * no longer the stack's concern - it belongs entirely to the external dhcp
 * service daemon (services/dhcp).  The stack only launches that service.  The
 * dhclient_name variable is kept only so the legacy sysctl slot still exists;
 * nothing in the stack reads it.
 */

/*
TEXT hequiv_path[FILENAME_MAX];
TEXT inetdconf_path[FILENAME_MAX];
*/
STRPTR version = "$VER: " STACK_RELEASE;

int
main(int argc, char *argv[])
{
    BYTE oldpri;
    STRPTR oldname;
    int retval;
    struct Library *TestSocketBase;

#ifndef __AROS__
    SysBase = *(struct ExecBase **)4;
#else
    D(bug("[AROSTCP](amiga_main.c) main()\n"));
#endif

    TestSocketBase = OpenLibrary("bsdsocket.library", 0);
    if(TestSocketBase) {
        CloseLibrary(TestSocketBase);
        error_request("Another TCP/IP stack is running, please quit it first.");
        return 21;
    }

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) main: Kernel launching ...\n"));
#else
    D(Printf(STACK_NAME " kernel started\n");)
#endif

    /*
     * Disable CTRL-C(D) Break signal.
     */
    signal(SIGINT, SIG_IGN);

    /*
     * Initialize AROSTCP_Task to point the Task structure of this task.
     */
    AROSTCP_Task = FindTask(NULL);
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) main: AROSTCP_Task @ 0x%p\n", AROSTCP_Task));
#endif
    netdiag_start();
    D(Printf("AROSTCP_Task @ 0x%p\n", AROSTCP_Task);)

    /*
     * Get a lock on the 'db' directory so we don't need an assign.
     */
    BPTR  db_path_lock = BNULL;

    char tmpconfigpath[1024];

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) main: Setting environment default Paths ... \n"));
#endif

    db_path_lock = GetProgramDir();
#if defined(__AROS__)
    db_path_lock = ParentDir(db_path_lock);
#endif

    NameFromLock(db_path_lock, interfaces_path, FILENAME_MAX);

#if defined(__AROS__)
    /*
     * ParentDir() above returned a lock we own; release it now that its
     * name has been read (it is otherwise overwritten below and leaked).
     * GetProgramDir()'s lock must NOT be unlocked, so this is only done on
     * the AROS path where ParentDir() actually ran.
     */
    UnLock(db_path_lock);
    db_path_lock = BNULL;
#endif

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) main: Directory tree root: %s\n", interfaces_path));
#endif
    D(Printf("Directory tree root: %s\n", interfaces_path);)
    strncpy(netdb_path, interfaces_path, FILENAME_MAX);
    strncpy(db_path, interfaces_path, FILENAME_MAX);
    strncpy(config_path, interfaces_path, FILENAME_MAX);
    strncpy(logfiledefname, "T:", FILENAME_MAX);             /* NicJA: Default to storing logs in Temp for launching
                                               from read only media                          */
    /*strcpy(hequiv_path, interfaces_path);
      strcpy(inetdconf_path, interfaces_path);*/
    AddPart(interfaces_path, _PATH_DB, FILENAME_MAX);
    AddPart(netdb_path, _PATH_DB, FILENAME_MAX);
    AddPart(db_path, _PATH_DB, FILENAME_MAX);
    AddPart(config_path, _PATH_DB, FILENAME_MAX);
    AddPart(logfiledefname, _PATH_SYSLOG, FILENAME_MAX);
    /* DHCP client path/launch is the dhcp service daemon's concern, not ours. */
    /*AddPart(hequiv_path, _PATH_HEQUIV, FILENAME_MAX);
      AddPart(inetdconf_path, _PATH_INETDCONF, FILENAME_MAX);*/

    /* NicJA : Allow user specified config location (from Env: variable) */
    if(GetVar("AROSTCP/Config", tmpconfigpath, 1024, GVF_GLOBAL_ONLY) != -1) {
        db_path_lock = BNULL;
#if defined(__AROS__)
        D(bug("[AROSTCP](amiga_main.c) main: Env: Var AROSTCP/Config set.\n"));
        D(bug("[AROSTCP](amiga_main.c) main: Attempting to use '%s' for config location..\n", tmpconfigpath));
#endif
        if(db_path_lock = Lock(tmpconfigpath, ACCESS_READ)) {
#if defined(__AROS__)
            D(bug("[AROSTCP](amiga_main.c) main: successfully locked config dir '%s'\n", tmpconfigpath));
#endif
            strncpy(interfaces_path, tmpconfigpath, FILENAME_MAX);
            strncpy(netdb_path, tmpconfigpath, FILENAME_MAX);
            strncpy(db_path, tmpconfigpath, FILENAME_MAX);
            strncpy(config_path, tmpconfigpath, FILENAME_MAX);
            /* Done validating the path; release the lock so it isn't leaked. */
            UnLock(db_path_lock);
            db_path_lock = BNULL;
        }
        /* TODO: NicJA - Attempt to create chosen config location */
        /* TODO: NicJA - and copy defaults if it doesnt currently exist and is possible? */
    }

    AddPart(interfaces_path, _FILE_SANA2CONFIG, FILENAME_MAX);
    AddPart(netdb_path, _FILE_NETDB, FILENAME_MAX);
    AddPart(config_path, _FILE_CONFIG, FILENAME_MAX);

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) main: Log file defaulting to '%s'\n", logfiledefname));
#endif

    /* Save pointer to this tasks old name */
    oldname = AROSTCP_Task->tc_Node.ln_Name;

    if(init_all()) {
#if defined(__AROS__)
        D(bug("[AROSTCP](amiga_main.c) main: preparing AROSTCP_Task\n"));
#endif
        /* Set our priority */
        oldpri = SetTaskPri(AROSTCP_Task, 5);

        /* Set our Task name */
        if(!taskname) {
#ifdef DEBUG
            if(nthLibrary) {
                if(taskname = bsd_malloc(sizeof("bsdsocket.library"), M_CFGVAR, M_WAITOK)) {
                    memcpy(taskname, "bsdsocket.library", sizeof("bsdsocket.library"));
                    taskname[6] = '.';
                    taskname[7] = '0' + nthLibrary;
                }
            } else {
#endif
                taskname = "bsdsocket.library";
#ifdef DEBUG
            }
#endif
        }
        if(taskname) AROSTCP_Task->tc_Node.ln_Name = taskname;

        initialized = TRUE;           /* Global initialization flag */

#ifdef DEBUG
        /* Show our task address */
        printf("%s Task address : %lx\n", taskname, (long) AROSTCP_Task);
#endif

        /* Initialize signal mask for the wait */
        breakmask = SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F;
        signalmask = timermask | breakmask | sanamask | controlmask;

        /*
         * Now when everything else is successfully initialized,
         * let the timeouts roll!
         */
        timer_send();
        netdiag.init_stage = NDI_RUNNING;

        for(;;) {
            ULONG got;

            netdiag.ml_where = NDW_WAIT;
            sig = Wait(signalmask);     /* Sleep until we are signalled. */
            netdiag.ml_iter++;
            netdiag.ml_wokeby = sig;
            netdiag_count(sig);

            do {
                if(sig & sanamask) {
                    netdiag.ml_where = NDW_SANA;
                    if(!sana_poll()) sig &= ~sanamask;
                }

                if(sig & timermask) {
                    netdiag.ml_where = NDW_TIMER;
                    if(!timer_poll()) sig &= ~timermask;
                }

                if(sig & controlmask) {
                    netdiag.ml_where = NDW_CONTROL;
                    if(!control_poll()) sig &= ~controlmask;
                }

                got = SetSignal(0L, signalmask) & signalmask;
                netdiag_count(got);
                sig |= got;
            } while(sig & (~breakmask));

            if((sig & breakmask) || ctrl_shutdown) {
                int i;

                netdiag.ml_where = NDW_STOP;

                ctrl_shutdown = FALSE;      /* consume: one stop attempt per request */

#if defined(__AROS__)
                D(bug("[AROSTCP](amiga_main.c) main: Task received CTRL-C\n"));
#endif
                /* We received CTRL-C
                 * NETTRACE task keeps one base open, it is not counted. */
                api_hide();		          /* hides the API from users */

                /* Tell registered external services to stop and release their
                 * bindings, then stop and WAIT for the service-manager's
                 * launched daemons to exit, so their bases are gone before we
                 * count remaining openers. */
                netservice_signal_external(NSPHASE_STOP);
                svcmgr_stop_all(TRUE);

                api_sendbreaktotasks(); /* send brk to all tasks w/ SBase open */

                /* Try three times with a short delay.  A stack-managed service
                 * was signalled to stop above and closes its own base as it
                 * shuts down, so the open count drops to the NETTRACE baseline
                 * without any special accounting here. */
                for(i = 0; i < 3 && MasterSocketBase->lib_OpenCnt > 1; i++) {
                    Delay(50);		          /* give tasks time to close socket base */
                }

                if(MasterSocketBase->lib_OpenCnt > 1) {
#if defined(__AROS__)
                    D(bug("[AROSTCP](amiga_main.c) main: Got CTRL-C while %ld %s still open.\n",
                          MasterSocketBase->lib_OpenCnt - 1,
                          (MasterSocketBase->lib_OpenCnt == 2) ? "library" : "libraries"));
#endif

                    __log(LOG_ERR, "Got CTRL-C while %d %s still open.\n",
                          (MasterSocketBase->lib_OpenCnt - 1),
                          (MasterSocketBase->lib_OpenCnt == 2) ? "library" : "libraries");

                    api_logopeners(); /* name the task(s) still holding a base */

                    api_show(); /* stopping not successful, show API to users */
                } else {
                    break;
                }
            }
        }
        retval = 0;
    } else
        retval = 20;

    /* free all resources */
    netdiag.ml_where = NDW_OUTSIDE;
    deinit_all();
    initialized = FALSE;

    SetTaskPri(AROSTCP_Task, oldpri);
    AROSTCP_Task->tc_Node.ln_Name = oldname;

    return retval;
}

/*
 * Do all initializations
 */
/*
 * Drain the out-of-band control port.  Mirrors sana_poll/timer_poll: it
 * returns FALSE (GetMsg drains everything, so no persistent pending state).
 * An NCMD_SHUTDOWN sets ctrl_shutdown, routed into the same orderly-stop path
 * as a CTRL-C; an NCMD_RELOAD runs in this (main-task) context - it drives the
 * netservice_stop_all()/reload/netservice_start_all() reconfigure (net_reload)
 * here.
 */
/*
 * A module of the stack queried, started or stopped by name, or loaded
 * from and unloaded with its library file. Runs in the stack's task with
 * the socket calls fenced off, as a reload does; a stop is refused, never
 * forced (net/netmodule.h).
 */
static LONG control_module(struct NetControlModuleMsg *mm)
{
    extern struct SignalSemaphore syscall_semaphore;
    struct netmodule *nm = NULL;
    LONG error = 0;

    if(mm->ncmm_Msg.ncm_Msg.mn_Length < sizeof(*mm))
        return EINVAL;
    mm->ncmm_Name[NCM_MODULE_NAMELEN - 1] = '\0';
    mm->ncmm_Flags = 0;
    mm->ncmm_Users = 0;

    /*
     * A module in a library file of its own, loaded or unloaded: the file
     * is read before the fence is taken, so these two take it themselves
     * (kern/net_modload.c).
     */
    if(mm->ncmm_Msg.ncm_Command == NCMD_MODULE_LOAD) {
        if((error = netmodule_load(mm->ncmm_Name, &nm)) != 0)
            return error;
        strncpy(mm->ncmm_Name, nm->nm_name, NCM_MODULE_NAMELEN - 1);    /* out: the module */
        /* loaded by name: what could not be had before may be asked for on demand again */
        netdemand_forget_failed(nm->nm_name);
    } else if(mm->ncmm_Msg.ncm_Command == NCMD_MODULE_DEMAND) {
        /*
         * Needed by a call of an application (net/netdemand.h): started
         * here, in the stack's task, whoever asked - a module's start may
         * do what only this task may do. Loading reads the disk, so this
         * takes the fence itself, as a load does.
         */
        error = netdemand_run(mm->ncmm_Name);
    } else if(mm->ncmm_Msg.ncm_Command == NCMD_MODULE_UNLOAD) {
        ObtainSemaphore(&syscall_semaphore);
        nm = netmodule_known(mm->ncmm_Name);
        ReleaseSemaphore(&syscall_semaphore);
        /* only this task loads and unloads: nm stays valid without the fence */
        if(nm == NULL)
            return ENOENT;
        if(nm->nm_library == NULL)
            error = EPERM;              /* its code is part of the program */
        else if((error = netmodule_unload(nm)) == 0)
            return 0;                   /* gone, and its descriptor with it */
        /* refused: say below what state it is in */
    }

    ObtainSemaphore(&syscall_semaphore);
    if(nm == NULL && (nm = netmodule_known(mm->ncmm_Name)) == NULL) {
        if(error == 0)
            error = ENOENT;
    } else {
        if(mm->ncmm_Msg.ncm_Command == NCMD_MODULE_START)
            error = netmodule_use(nm);
        else if(mm->ncmm_Msg.ncm_Command == NCMD_MODULE_STOP)
            error = (nm->nm_flags & NMF_STARTED) ? netmodule_unregister(nm) : 0;
        if(nm->nm_flags & NMF_STARTED)
            mm->ncmm_Flags |= NCMMF_STARTED;
        if((nm->nm_flags & NMF_PERMANENT) || nm->nm_stop == NULL)
            mm->ncmm_Flags |= NCMMF_PERMANENT;
        if(nm->nm_flags & NMF_BUILTIN)
            mm->ncmm_Flags |= NCMMF_BUILTIN;
        if(nm->nm_library != NULL)
            mm->ncmm_Flags |= NCMMF_LIBRARY;
        mm->ncmm_Users = netmodule_users(nm);
    }
    ReleaseSemaphore(&syscall_semaphore);
    return error;
}

static BOOL control_poll(void)
{
    struct NetControlMsg *msg;

    if(controlport == NULL)
        return FALSE;

    while((msg = (struct NetControlMsg *)GetMsg(controlport)) != NULL) {
        msg->ncm_Result = 0;
        switch(msg->ncm_Command) {
        case NCMD_SHUTDOWN:
            ctrl_shutdown = TRUE;
            break;
        case NCMD_RELOAD:
            D(bug("[AROSTCP](amiga_main.c) control: RELOAD\n"));
            /* a module that could not be had is looked for again (net/netdemand.h) */
            netdemand_forget_failed(NULL);
            /*
             * The configuration reader first, before net_reload() takes the
             * old configuration down: when it cannot be had the reload is
             * refused and the stack keeps running as it is. Dropped again
             * when the new configuration is in place (kern/netconfig_ops.h).
             */
            if((msg->ncm_Result = netconfig_acquire()) != 0) {
                __log(LOG_ERR, "reload refused: the configuration is kept.");
                break;
            }
            msg->ncm_Result = net_reload();   /* in-place reconfigure (main-task context) */
            netconfig_release();
            break;
        case NCMD_MODULE_QUERY:
        case NCMD_MODULE_START:
        case NCMD_MODULE_STOP:
        case NCMD_MODULE_LOAD:
        case NCMD_MODULE_UNLOAD:
        case NCMD_MODULE_DEMAND:
            msg->ncm_Result = control_module((struct NetControlModuleMsg *)msg);
            break;
        case NCMD_NOP:
        default:
            break;
        }
        ReplyMsg((struct Message *)msg);
    }
    return FALSE;
}

BOOL
init_all(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) init_all()\n"));
#endif
    /*
     * Initialize malloc semaphore
     */
    malloc_init();
    D(Printf("malloc_init() complete\n");)

    /*
     * initialize concurrency control subsystem
     */
    spl_init();
    D(Printf("spl_init() complete\n");)

    /*
     * initialize sleep queues
     */
    sleep_init();
    D(Printf("sleep_init() complete\n");)
    netdiag.init_stage = NDI_LOCKS;

    /*
     * the demand slots and the ioctl ranges of modules loaded on first use
     * (net/netdemand.h): empty, before anything can ask
     */
    netdemand_init();

    /*
     * initialize the network service registry (lives in netservices.library;
     * empty until services register).  The registry data is ready now; the
     * library base that exposes it to external daemons is created in api_init().
     */
    netservice_registry_init();
    D(Printf("netservice_registry_init() complete\n");)

    /*
     * The configuration reader (module "netconfig", kern/netconfig_ops.h),
     * before anything the configuration decides: PROTOCOLS= and the
     * interface lines say what starts. Loading it needs, of what is made
     * further down, only the fence, which the loader takes
     * (kern/net_modload.c), and arostcp.library, which a module library
     * attaches to. Both are made here, earlier than they used to be; both
     * depend on nothing made in between (the fence is a semaphore,
     * InitSemaphore()d again unheld by api_init(); netcore_init() makes and
     * publishes a library of function vectors and is not repeated by its
     * second call below). Without the reader the configuration cannot be
     * read, and the stack does not start (as with PROTOCOLS that select no
     * family). It is dropped again when the start-up is done (below).
     */
    InitSemaphore(&syscall_semaphore);
    if(!netcore_init())
        return FALSE;
    if(netconfig_acquire() != 0) {
        Printf("AROSTCP: cannot read the configuration: "
               NETCONFIG_MODULE NETMODULE_KIND_ATCP " is missing or does not load. "
               "AROSTCP will quit.\n");
        return FALSE;
    }

    /*
     * Read command line arguments and configuration file
     */
    if(!readconfig())
        return FALSE;
    netdiag.init_stage = NDI_CONFIG;
    D(Printf("readconfig() complete\n");)

    /*
     * initialize logging system
     */
    if(!log_init())
        return FALSE;
    netdiag.init_stage = NDI_LOG;
    D(Printf("log_init() complete\n");)

    /*
     * initialize the mbuf subsystem
     */
    if(!mbinit())
        return FALSE;
    netdiag.init_stage = NDI_MBUF;
    D(Printf("mbinit() complete\n");)

    /*
     * initialize timer
     */
    if((timermask = timer_init()) == 0L)
        return FALSE;
    netdiag.init_stage = NDI_TIMER;
    D(Printf("timer_init() complete\n");)

    /*
     * initialize API
     */
    if(!api_init())
        return FALSE;
    netdiag.init_stage = NDI_API;
    D(Printf("api_init() complete\n");)

    /*
     * publish arostcp.library, the core's services for the modules that
     * are library files of their own - before any module is started
     * (published already for the configuration reader, above: this call
     * finds it there and does nothing)
     */
    if(!netcore_init())
        return FALSE;
    netdiag.init_stage = NDI_NETCORE;
    D(Printf("netcore_init() complete\n");)

    /*
     * initialize SANA-II subsystem
     */
    if((sanamask = sana_init()) == 0L)
        return FALSE;
    netdiag.init_stage = NDI_SANA;
    D(Printf("sana_init() complete\n");)

    /*
     * initialize domains (initializes all protocols)
     */
    domaininit();
    netdiag.init_stage = NDI_DOMAINS;
    D(Printf("domaininit() complete\n");)

    /*
     * The protocol modules the configuration selects (PROTOCOLS, default
     * both families) were started by that. A module that is missing or
     * does not start is an error in the log and the stack goes on with
     * the others; with none at all there is nothing a socket could be
     * made of, and the stack does not start.
     */
    if(netmodule_protocols_started() == 0) {
        Printf("AROSTCP: no protocol family could be started (PROTOCOLS=%s). "
               "See the log; AROSTCP will quit.\n",
               netmodule_protocols ? netmodule_protocols
                                   : NETMODULE_PROTOCOLS_DEFAULT);
        return FALSE;
    }

    pfil_init();
    netdiag.init_stage = NDI_PFIL;
    D(Printf("pfil_init() complete\n");)

    /* (packet capture sets itself up when it is first used: net/bpf.c) */

    loconfig();
    netdiag.init_stage = NDI_LOOPBACK;
    D(Printf("loconfig() complete\n");)

    /*
     * Initialize NetDataBase
     */
    if(init_netdb() != 0)
        return FALSE;
    netdiag.init_stage = NDI_NETDB;
    D(Printf("init_netdb() complete\n");)

    /*
     * The optional modules the configuration wants from the start
     * (MODULES=) instead of on their first use. What cannot be had is an
     * error in the log; the stack runs without it.
     */
    netmodule_start_listed();
    netdiag.init_stage = NDI_MODULES;

    /*
     * Create the out-of-band control port (RELOAD / SHUTDOWN requests, and
     * the modules). Non-fatal on failure: the stack simply has no reload
     * channel, and a module that is not running when it is first needed
     * cannot be had. Made BEFORE the API is shown: a call that needs a
     * module started sends its request here (net/netdemand.h), and the
     * first calls come as soon as the API is visible - from NETTRACE among
     * others. Requests wait in the port until the main loop runs.
     */
    if((controlport = CreateMsgPort()) != NULL) {
        controlport->mp_Node.ln_Name = (char *)AROSTCP_CTRLPORT_NAME;
        controlport->mp_Node.ln_Pri  = 0;
        AddPort(controlport);
        controlmask = 1UL << controlport->mp_SigBit;
        netdemand_setport(controlport);
        D(Printf("control port '%s' up\n", AROSTCP_CTRLPORT_NAME);)
    }
    netdiag.init_stage = NDI_CTRLPORT;

    /*
     * Make API visible. Nothing may fail after this point: the error path
     * would tear the stack down under clients that have already opened it,
     * so the NETTRACE check comes first.
     */
    if(!Nettrace_Task || api_show() == FALSE)
        return FALSE;
    netdiag.init_stage = NDI_SHOWN;
    D(Printf("api_show() complete\n");)

    D(Printf("Initialization complete, signalling NETTRACE\n");)
    Signal(Nettrace_Task, SIGBREAKF_CTRL_F);

    rc_start();
    netdiag.init_stage = NDI_RC;
    D(Printf("rc_start() complete, initialization finished\n");)

    /* Register the built-in services now that the subsystems are up. */
    net_services_register();

    /*
     * The service files were the last configuration read by the start
     * (svcmgr_register()): the reader leaves memory until the next reload.
     * Before NDI_DONE, so that "start-up done" means it is gone. (A program
     * that looks at memory as soon as bsdsocket.library appears still sees
     * it: the API is shown, and the control port answers, before this - a
     * control message sent now is answered from the main loop, after it.)
     */
    netconfig_release();
    netdiag.init_stage = NDI_DONE;

#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) init_all: Initialisation successful.\n"));
#endif

    return TRUE;
}

/*
 * clean up everything
 */
void
deinit_all(void)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) deinit_all()\n"));
#endif
    netdiag.init_stage = NDI_STOPPING;
    netdiag.ml_where = NDW_OUTSIDE;

    /*
     * make sure we are out of critical section
     */
    spl0();
    D(Printf("spl0() completed\n");)

    api_hide();			/* hides the API from users */
    D(Printf("api_hide() completed\n");)

    /*
     * Remove the control port so no new RELOAD/SHUTDOWN arrives during
     * teardown; reply any stragglers so senders waiting on us don't block.
     */
    if(controlport) {
        struct NetControlMsg *m;
        netdemand_setport(NULL);    /* no new demand is sent; those sent are replied below */
        RemPort(controlport);
        while((m = (struct NetControlMsg *)GetMsg(controlport)) != NULL) {
            m->ncm_Result = -1;
            ReplyMsg((struct Message *)m);
        }
        DeleteMsgPort(controlport);
        controlport = NULL;
        controlmask = 0;
    }

    /*
     * Deinitialize network database: the interface lines that wait for
     * their device (DEFER) are forgotten - before the modules are stopped,
     * because the configuration reader is kept loaded while one waits.
     * Nothing reads them any more: the control port is gone, and the timer
     * that tries them runs in this task, which is here.
     */
    netdb_deinit();

    /*
     * THE MODULES AT SHUTDOWN, step 1 of 3 (the whole order is described
     * in kern/net_modload.c). No application has a library base open here
     * - the main loop does not leave while one has - so only this task
     * can still call into a module.
     *
     * Every module that lets itself be stopped is stopped now, by its own
     * stop function, while everything those functions use is still there:
     * the interfaces are open, the timers run, the pools exist. The
     * protocol families go first and the transports after them
     * (netmodule_stop_all(): the module that needs another before the one
     * it needs); each takes its addresses, routes and requests off the
     * interfaces, withdraws what it registered and frees what it
     * allocated. Their library files are then unloaded
     * (netmodule_unload_all()).
     *
     * A module whose stop refuses or fails is named in the log and stays
     * as it is: closing the interfaces below may still call it, and it is
     * cut off wholesale in steps 2 and 3, as every protocol module was
     * before they could stop. So does one that cannot be stopped at all.
     *
     * Under the fence, as a stop asked for through the control port is,
     * though nobody is left to fence off. (A stack whose start-up failed
     * before any module was started has nothing to stop, and may not have
     * a fence yet.)
     */
    if(netmodules != NULL) {
        extern struct SignalSemaphore syscall_semaphore;

        ObtainSemaphore(&syscall_semaphore);
        netmodule_stop_all();
        ReleaseSemaphore(&syscall_semaphore);
    }
    netmodule_unload_all();

    /*
     * (Packet capture keeps nothing but memory of the core's pool, which is
     * freed in one piece below.)
     */

    /*
     * Deinitialize network interfaces
     */
    sana_deinit();

    /*
     * Deinitialize timers
     */
    timer_deinit();

    /*
     * THE MODULES AT SHUTDOWN, steps 2 and 3, for the modules that are
     * still started - those that could not be stopped above. Nothing can
     * call a module any more: sana_deinit() has aborted, collected and deleted every
     * SANA-II request (no read completion is delivered), timer_deinit()
     * has deleted the periodic calls, the deadline timer and the
     * protocols' fast and slow timers, and this task polls no queue.
     *
     * 2. netmodule_shutdown_all(): each module that is still started
     *    releases what it holds that is not the core's (nm_shutdown; what
     *    it took from the core's pools and mbufs is freed in one piece
     *    below), and the core forgets every module and every
     *    registration, so that no pointer into a module is left to
     *    follow.
     * 3. netmodule_expunge_all(): every module library is detached,
     *    closed and removed from memory - its static data must not be
     *    there, stale, when the stack is started again - and then
     *    arostcp.library, which each of them held open, is taken back.
     *
     * Before mbdeinit(), log_deinit() and malloc_deinit(): a module's
     * nm_shutdown may still log and free.
     */
    netmodule_shutdown_all();
    netmodule_expunge_all();
    netcore_deinit();

    /*
     * Free all resources allocated by mbufs.
     */
    mbdeinit();
    D(Printf("mbdeinit() completed\n");)

    log_deinit();
    D(Printf("log_deinit() completed\n");)

    /*
     * Tear down the service registry (services own their own storage).
     */
    netservice_registry_deinit();

    /*
     * Free memory pool.
     */
    malloc_deinit();
    D(Printf("malloc_deinit() completed\n");)

    /*
     * Check that there are no libraries open (to our API). We can continue only
     * if all bases are closed.
     */
    api_deinit();  /* NOTICE: this waits until every api user has exited */

    netdiag_end();
}

/*
 * Notification function for taskname
 */
int taskname_changed(void *p, IPTR newname)
{
#if defined(__AROS__)
    D(bug("[AROSTCP](amiga_main.c) taskname_changed()\n"));
#endif

    AROSTCP_Task->tc_Node.ln_Name = (UBYTE *)newname;
    if(initialized)
        printf("New task name '%s'\n", (char *)newname);

    return TRUE;
}
