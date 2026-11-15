/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Modules of the stack whose code is in a library file of its own: loading
 * and unloading (net/netmodule.h, net/netcore.h).
 *
 * A module library is an ordinary AROS library with two vectors,
 * NetModuleAttach() and NetModuleDetach(). Loading is
 *
 *      OpenLibrary()           the file comes into memory; nothing of the
 *                              stack is touched yet, and the socket calls
 *                              are not held up while the disk is read
 *      NetModuleAttach()       the module opens arostcp.library, takes its
 *                              copy of the core's table and hands back its
 *                              `struct netmodule'
 *      netmodule_register()    the modules it needs are started, then it is
 *
 * and unloading the same backwards: netmodule_unregister() (refused while
 * the module is in use - then nothing else happens), NetModuleDetach(),
 * RemLibrary(), CloseLibrary().
 *
 * WHEN THE STACK ENDS every module library leaves memory, whether its
 * module can be stopped or not: a library that stayed would keep its
 * static data as this run left it, and its copy of a table that points
 * into a program that is gone, for the next start of the stack to find.
 * The modules stop by their own stop functions where they can - the
 * protocol families and the transports do. One whose stop refuses or
 * fails, or that has none, is not stopped but made unreachable; the order
 * of all of it is kern/amiga_main.c deinit_all()'s:
 *
 *   1. no application has a library base open (the stack refuses to end
 *      while one has) and the control port is gone;
 *   2. netmodule_stop_all() (kern/net_module.c): every module that lets
 *      itself be stopped is, the one that needs another first - the
 *      families, then the transports - while the stack is whole; what
 *      does not stop is named in the log. netmodule_unload_all(): the
 *      libraries of the stopped modules are unloaded;
 *   3. the interfaces are closed (sana_deinit(): every SANA-II request is
 *      aborted, waited for and deleted, so no read-completion handler of
 *      a module can be called again) and the timers deleted
 *      (timer_deinit(): the periodic calls of netperiodic_add(), the
 *      deadline timer, the protocols' fast and slow timers);
 *   4. netmodule_shutdown_all(): nm_shutdown of each module that is
 *      still started (it did not stop in step 2) -
 *      what it holds outside the core's pools - and the core forgets
 *      every registration (domains, input queues, interface events,
 *      variables, dispatch tables, tunnels, named interfaces);
 *   5. netmodule_expunge_all() below: NetModuleDetach(), RemLibrary(),
 *      CloseLibrary() for every library that is left;
 *   6. arostcp.library is taken back, then the core frees its mbufs and
 *      its memory pool in one piece - with them everything a module
 *      allocated from the core.
 *
 * Every call into a module is made by the stack's own task (timer_poll(),
 * sana_poll(), net_poll(), a control request) or by an application inside
 * a socket call. After step 1 there is no application, and the stack's
 * task is in deinit_all() and polls nothing: from step 4 on nothing calls
 * a module, and step 5 can take its code away.
 *
 * A module cannot leave memory under the stack, for three reasons of which
 * each alone is enough: the registry does not stop a module in use, and
 * nothing is closed before a stop succeeded; the stack holds the library
 * open from load to unload, and exec does not expunge an open library; and
 * the module's own expunge guard (net/netmodule_lib.c) refuses while it is
 * attached or started.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>

#include <exec/libraries.h>
#include <exec/memory.h>
#include <proto/dos.h>

#include <net/netcore.h>

extern struct SignalSemaphore syscall_semaphore;    /* the fence (api/amiga_api.c) */
extern struct Task *AROSTCP_Task;                   /* the stack's own (kern/amiga_main.c) */

/* One loaded module library. */
struct netmodlib {
    struct MinNode    nl_node;
    struct Library   *nl_base;
    struct netmodule *nl_module;
    ULONG             nl_size;      /* of this node, with the name */
    char              nl_file[1];   /* the library's name as exec lists it (no path) */
};

static struct MinList netmodlibs;
static BOOL netmodlibs_ready = FALSE;

static void
netmodlibs_init(void)
{
    if(!netmodlibs_ready) {
        NewList((struct List *)&netmodlibs);
        netmodlibs_ready = TRUE;
    }
}

static struct netmodlib *
netmodlib_find(struct netmodule *nm, const char *file, const char *name)
{
    struct netmodlib *nl;

    if(!netmodlibs_ready)
        return (NULL);
    for(nl = (struct netmodlib *)netmodlibs.mlh_Head; nl->nl_node.mln_Succ;
            nl = (struct netmodlib *)nl->nl_node.mln_Succ)
        if((nm != NULL && nl->nl_module == nm) ||
                (file != NULL && strcmp(nl->nl_file, file) == 0) ||
                (name != NULL && nl->nl_module != NULL &&
                 strcmp(nl->nl_module->nm_name, name) == 0))
            return (nl);
    return (NULL);
}

/*
 * A loaded module by name. The registry asks here (netmodule_known()) for
 * a module that was stopped: it is off the registry's list then, but its
 * library is still loaded and attached, and it can be started again.
 */
struct netmodule *
netmodule_loaded(const char *name)
{
    struct netmodlib *nl = netmodlib_find(NULL, NULL, name);

    return (nl ? nl->nl_module : NULL);
}

struct netmodule *
netmodule_loaded_file(const char *file)
{
    struct netmodlib *nl = netmodlib_find(NULL, file, NULL);

    return (nl ? nl->nl_module : NULL);
}

/*
 * The library is to leave memory now, not when memory next runs short. In
 * this order: while we still hold it open RemLibrary() only marks it
 * (LIBF_DELEXP), and the CloseLibrary() that brings the count to zero
 * expunges it and has its segments unloaded. The other way round the base
 * may already be gone when RemLibrary() is handed it.
 */
static void
netmodlib_close(struct Library *base)
{
    RemLibrary(base);
    CloseLibrary(base);
}

/*
 * Open a module library. A name without a path is looked for in the Libs
 * directory next to the stack's C directory first; then where
 * OpenLibrary() looks for any library (the stack's current and program
 * directories, LIBS:).
 */
static struct Library *
netmodlib_open(const char *libname)
{
    struct Library *base = NULL;

    /*
     * The stack's program directory, not the current directory's: loads
     * run in the stack's task (an application's demand reaches it through
     * the control port, kern/net_demand.c), whose current directory is
     * wherever the stack was started from. A caller that is not a
     * process has no DOS at all, and gets what OpenLibrary() finds.
     */
    if(FilePart((STRPTR)libname) == (STRPTR)libname &&
            FindTask(NULL)->tc_Node.ln_Type == NT_PROCESS) {
        char *path = AllocMem(FILENAME_MAX, MEMF_PUBLIC);
        BPTR prog = (AROSTCP_Task != NULL && AROSTCP_Task->tc_Node.ln_Type == NT_PROCESS)
                    ? ((struct Process *)AROSTCP_Task)->pr_HomeDir : GetProgramDir();
        BPTR home;

        if(path != NULL) {
            if(prog != BNULL && (home = ParentDir(prog)) != BNULL) {
                if(NameFromLock(home, path, FILENAME_MAX) &&
                        AddPart(path, NETMODULE_LIBDIR, FILENAME_MAX) &&
                        AddPart(path, (STRPTR)libname, FILENAME_MAX))
                    base = OpenLibrary(path, 0);
                UnLock(home);
            }
            FreeMem(path, FILENAME_MAX);
        }
    }
    if(base == NULL)
        base = OpenLibrary((STRPTR)libname, 0);
    return (base);
}

static int netmodule_load1(const char *libname, struct netmodule **nmp, int optional,
                           int nostart);

int
netmodule_load(const char *libname, struct netmodule **nmp)
{
    return (netmodule_load1(libname, nmp, 0, 0));
}

int
netmodule_load_optional(const char *libname, struct netmodule **nmp)
{
    return (netmodule_load1(libname, nmp, 1, 0));
}

/*
 * Loaded and attached, not started (net/netmodule.h): the registry reads
 * the descriptor of a module another one needs, and starts it or sends it
 * away. It says in the log what it did; a file that is not there is a
 * notice here - whether that is an error is the registry's to say, which
 * knows who needed it.
 */
int
netmodule_attach(const char *libname, struct netmodule **nmp)
{
    return (netmodule_load1(libname, nmp, 1, 1));
}

static int
netmodule_load1(const char *libname, struct netmodule **nmp, int optional, int nostart)
{
    struct netmodlib *nl;
    struct netmodule *nm = NULL;
    struct Library *base;
    const char *file;
    ULONG size;
    LONG res;
    int error = 0;

    if(nmp != NULL)
        *nmp = NULL;
    if(libname == NULL || *libname == '\0')
        return (EINVAL);
    netmodlibs_init();
    file = (const char *)FilePart((STRPTR)libname);

    if((nl = netmodlib_find(NULL, file, NULL)) != NULL) {
        __log(LOG_ERR, "module library %s is loaded already (module %s).",
              file, nl->nl_module->nm_name);
        return (EEXIST);
    }
    size = sizeof(struct netmodlib) + strlen(file);
    if((nl = AllocMem(size, MEMF_PUBLIC | MEMF_CLEAR)) == NULL)
        return (ENOMEM);
    nl->nl_size = size;
    strcpy(nl->nl_file, file);

    /* outside the fence: this is where the disk is read */
    if((base = netmodlib_open(libname)) == NULL) {
        if(nostart)
            __log(LOG_NOTICE, "module library %s is not installed.", libname);
        else if(optional)
            __log(LOG_NOTICE, "module library %s is not installed: the stack runs "
                  "without it.", libname);
        else
            __log(LOG_ERR, "module library %s: not found, or it could not be opened.",
                  libname);
        FreeMem(nl, size);
        return (ENOENT);
    }

    ObtainSemaphore(&syscall_semaphore);

    res = NetModuleAttach(base, &nm);
    if(res != NMA_OK) {
        switch(NMA_CODE(res)) {
        case NMA_OLDCORE:
            __log(LOG_ERR, "module library %s needs version %ld of the stack's services; "
                  "this stack has version %ld.",
                  file, (long)NMA_WANTS(res), (long)NETCORE_VERSION);
            break;
        case NMA_NOCORE:
            __log(LOG_ERR, "module library %s could not open %s version %ld.",
                  file, NETCORE_LIBNAME, (long)NETCORE_LIBVERSION);
            break;
        case NMA_ATTACHED:
            __log(LOG_ERR, "module library %s is attached to a stack already.", file);
            break;
        default:
            __log(LOG_ERR, "module library %s would not attach (%ld).", file, (long)res);
            break;
        }
        error = EPROTONOSUPPORT;
        goto fail_open;
    }
    /* before anything of the descriptor past its first fields is touched */
    if(nm == NULL || nm->nm_size < NETMODULE_SIZE_V2 || nm->nm_version < 2 ||
            nm->nm_version > NETMODULE_VERSION || nm->nm_name == NULL) {
        __log(LOG_ERR, "module library %s: its module descriptor (version %ld, %ld bytes) "
              "is not the one this stack uses (version %ld, %ld bytes).",
              file, nm ? (long)nm->nm_version : 0L, nm ? (long)nm->nm_size : 0L,
              (long)NETMODULE_VERSION, (long)sizeof(struct netmodule));
        error = EPROTONOSUPPORT;
        goto fail_attached;
    }
    if(netmodule_known(nm->nm_name) != NULL) {
        __log(LOG_ERR, "module library %s: a module named %s is part of the stack already.",
              file, nm->nm_name);
        error = EEXIST;
        goto fail_attached;
    }

    /* from here on it is a module like one built into the program */
    nm->nm_flags &= ~(NMF_STARTED | NMF_STARTING | NMF_BUILTIN);
    nm->nm_library = base;
    nl->nl_base = base;
    nl->nl_module = nm;
    AddTail((struct List *)&netmodlibs, (struct Node *)&nl->nl_node);

    if(nostart) {
        /* known by name from here on; whoever asked starts it or unloads it */
        ReleaseSemaphore(&syscall_semaphore);
        if(nmp != NULL)
            *nmp = nm;
        return (0);
    }

    /* the registry's own messages name a module that is needed and missing */
    if((error = netmodule_register(nm)) != 0) {
        __log(LOG_ERR, "module %s (%s) could not be started (error %ld); it is unloaded again.",
              nm->nm_name, file, (long)error);
        Remove((struct Node *)&nl->nl_node);
        goto fail_attached;
    }
    ReleaseSemaphore(&syscall_semaphore);

    __log(LOG_NOTICE, "module %s loaded from %s.", nm->nm_name, file);
    if(nmp != NULL)
        *nmp = nm;
    return (0);

fail_attached:
    NetModuleDetach(base);
fail_open:
    ReleaseSemaphore(&syscall_semaphore);
    netmodlib_close(base);
    FreeMem(nl, size);
    return (error);
}

int
netmodule_unload(struct netmodule *nm)
{
    struct netmodlib *nl;
    struct Library *base;
    int error;

    if(nm == NULL || (nl = netmodlib_find(nm, NULL, NULL)) == NULL)
        return (EINVAL);
    base = nl->nl_base;

    ObtainSemaphore(&syscall_semaphore);
    /*
     * Stopped first, and only if it lets itself be stopped: in use, the
     * module stays as it is and its library stays open. A module that was
     * stopped before has no users - whatever needs it would have started
     * it.
     */
    if((nm->nm_flags & NMF_STARTED) && (error = netmodule_unregister(nm)) != 0) {
        ReleaseSemaphore(&syscall_semaphore);
        __log(LOG_NOTICE, "module %s is not unloaded: it %s (error %ld).", nm->nm_name,
              error == EBUSY ? "is in use" : "cannot be stopped", (long)error);
        return (error);
    }
    Remove((struct Node *)&nl->nl_node);
    __log(LOG_NOTICE, "module %s unloaded (%s).", nm->nm_name, nl->nl_file);
    nm->nm_library = NULL;
    NetModuleDetach(base);      /* the module closes arostcp.library */
    ReleaseSemaphore(&syscall_semaphore);

    netmodlib_close(base);      /* nm is gone with it */
    FreeMem(nl, nl->nl_size);
    return (0);
}

/*
 * The stack is ending, and is still whole: unload every module library
 * whose module lets itself be stopped. No socket exists any more, so a
 * stop is not refused for that; modules that need one another are
 * unloaded over several rounds, the dependent first.
 *
 * A module that will not stop - it has no nm_stop, or it refuses - stays
 * as it is here: its code may still be called while the interfaces are
 * closed. It leaves memory later, through netmodule_expunge_all(), when
 * nothing can call it any more. (netmodule_stop_all() has stopped what
 * stops before this is called at shutdown, and said in the log what did
 * not: a library whose module is stopped is only detached and closed
 * here.)
 */
void
netmodule_unload_all(void)
{
    struct netmodlib *nl, *prev;
    BOOL progress;

    if(!netmodlibs_ready)
        return;
    do {
        progress = FALSE;
        for(nl = (struct netmodlib *)netmodlibs.mlh_TailPred; nl->nl_node.mln_Pred; nl = prev) {
            prev = (struct netmodlib *)nl->nl_node.mln_Pred;
            /* known not to stop: no line in the log for each at every shutdown */
            if(nl->nl_module->nm_stop == NULL || (nl->nl_module->nm_flags & NMF_PERMANENT))
                continue;
            if(netmodule_unload(nl->nl_module) == 0)
                progress = TRUE;
        }
    } while(progress);
}

/*
 * The stack is ending and netmodule_shutdown_all() has run: no module is
 * registered or started any more and nothing in the core points at one.
 * Every library that is still loaded - those whose module could not be
 * stopped - is detached and removed from memory now. The order no longer
 * matters: no module calls another any more, and detaching only closes
 * arostcp.library.
 *
 * The module's own expunge guard (net/netmodule_lib.c) lets the library
 * go when it is detached and not started; netmodule_shutdown_all() cleared
 * NMF_STARTED, and it is cleared here again for a library that was loaded
 * but never on the registry. The stack is the library's only opener (the
 * glue refuses a second), so the CloseLibrary() below brings the count to
 * zero and LDDemon unloads the segments: the next start of the stack
 * loads the file anew, with the data the file has.
 */
void
netmodule_expunge_all(void)
{
    struct netmodlib *nl;

    if(!netmodlibs_ready)
        return;
    while((nl = (struct netmodlib *)RemTail((struct List *)&netmodlibs)) != NULL) {
        struct Library *base = nl->nl_base;

        nl->nl_module->nm_flags &= ~(NMF_STARTED | NMF_STARTING);
        nl->nl_module->nm_library = NULL;
        NetModuleDetach(base);      /* the module closes arostcp.library */
        netmodlib_close(base);      /* its code and data are gone with this */
        FreeMem(nl, nl->nl_size);
    }
}
