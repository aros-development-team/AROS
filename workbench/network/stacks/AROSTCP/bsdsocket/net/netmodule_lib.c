/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * The part every module library of the stack has, unchanged: its copy of
 * the core's table, the two vectors the stack calls, and the library rules
 * that keep it in memory while the stack uses it. It is NOT part of the
 * stack program; it is compiled into each module library, which is built
 * by the ordinary AROS build from net/netmodule.conf (genmodule writes the
 * resident structure, InitLib, Open, Close and Expunge).
 *
 * It expects the module to define one thing: `struct netmodule
 * netmodule_this', its descriptor (net/netmodule.h).
 */
#ifndef NETMODULE
#define NETMODULE
#endif

#include <proto/exec.h>
#include <exec/libraries.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <aros/autoinit.h>

#include <net/netcore.h>

struct netcore __netcore;               /* filled by NetModuleAttach() */

extern struct netmodule netmodule_this; /* the module's descriptor */

static struct Library *NetCoreBase;     /* arostcp.library while attached */

/*
 * Two things the link libraries would otherwise add to every module
 * library, about 1.1 KB each time on m68k, for cases a module of the stack
 * cannot be in:
 *
 * SysBase. Taken from libexec.a it comes with a check of exec's version
 * that reports through the error requester below. The stack that loads
 * this library is running on that exec already; the library's own InitLib
 * (genmodule) still refuses an exec that is too old.
 */
struct ExecBase *SysBase;

static int NetModule_SysBase(struct ExecBase *sysBase)
{
    SysBase = sysBase;
    return sysBase != NULL;
}

ADD2INIT(NetModule_SysBase, -128)

/*
 * The requester (or shell line) with which the automatic opening of
 * libraries reports one it could not open - libautoinit's, which brings
 * the format-string walker of libamiga with it. A module opens no library
 * that way (net/netmodule.conf: noautolib; it has the core's table
 * instead), so there is nothing to report; and if one ever did, the open
 * still fails, InitLib fails, and the stack says which module did not
 * load. Defined here, the link takes this one and leaves the other out.
 */
void ___showerror(struct ExecBase *sysBase, const char *format, ...)
{
}

/*
 * The library's InitLib, Open, Close and Expunge bodies. genmodule's code
 * (the generated *_start.c) calls four functions of libautoinit for them;
 * libautoinit has them in one object with the resource and device forms and
 * the automatic opening of libraries, which brings two more objects along -
 * about 1.9 KB in each module library on m68k for code no module of the
 * stack runs. Defined here, the link takes these and none of that.
 *
 * They do what libautoinit's do (compiler/autoinit/functions.c), in the
 * same order, for what a module library has: the symbol sets (INIT, CTORS,
 * INIT_ARRAY, INITLIB, OPENLIB, CLOSELIB, EXPUNGELIB, FINI_ARRAY, DTORS,
 * EXIT). What it cannot have is refused rather than skipped: libraries
 * opened automatically, relative library bases, classes and the oop.library
 * base (net/netmodule.conf: noautolib, no classes) - InitLib fails, and the
 * stack says the module did not load. If libautoinit's object is ever
 * linked in all the same, the link fails on the names defined twice.
 */
static int netmodule_callset(const void * const *set, int dir, int test_fail, void *arg)
{
    int pos, (*func)(void *);

    if(set == NULL)
        return 1;
    ForeachElementInSet(set, dir, pos, func) {
        if(!(*func)(arg) && test_fail)
            return 0;
    }
    return 1;
}

static void netmodule_setsexit(const struct __aros_libinit_sets *s, struct ExecBase *SysBase)
{
    netmodule_callset(s->fini_array, -1, 0, SysBase);
    netmodule_callset(s->dtors, 1, 0, SysBase);
    netmodule_callset(s->exit, -1, 0, SysBase);
}

static void netmodule_freebase(struct Library *lib, struct ExecBase *SysBase)
{
    FreeMem((UBYTE *)lib - lib->lib_NegSize, lib->lib_NegSize + lib->lib_PosSize);
}

void *_set_libinitbase(const struct __aros_libinit_sets *s, struct Library *lib,
                       LONG sysver, struct Library **oopbaseptr, struct ExecBase *SysBase)
{
    int initcalled = 0;

    if(!SysBase || SysBase->LibNode.lib_Version < sysver)
        return NULL;
    if(oopbaseptr != NULL || s->libs != NULL || s->rellibs != NULL ||
            s->classesinit != NULL || s->classesexpunge != NULL) {
        netmodule_freebase(lib, SysBase);
        return NULL;
    }
    if(netmodule_callset(s->init, 1, 1, SysBase)) {
        netmodule_callset(s->ctors, -1, 0, SysBase);
        netmodule_callset(s->init_array, 1, 0, SysBase);
        initcalled = 1;
        if(netmodule_callset(s->initlib, 1, 1, lib))
            return lib;
    }
    if(initcalled)
        netmodule_callset(s->expungelib, -1, 0, lib);
    netmodule_setsexit(s, SysBase);
    netmodule_freebase(lib, SysBase);
    return NULL;
}

void *_set_libopen(const struct __aros_libfuncs_sets *s, struct Library *lib)
{
    if(!netmodule_callset(s->openlib, 1, 1, lib))
        return NULL;
    lib->lib_OpenCnt++;
    lib->lib_Flags &= ~LIBF_DELEXP;
    return lib;
}

/* not 0 when the caller has to expunge the library (a library has no closedev) */
int _set_libclose(const struct __aros_libfuncs_sets *s, struct Library *lib,
                  struct IORequest *ioreq)
{
    lib->lib_OpenCnt--;
    netmodule_callset(s->closelib, -1, 0, lib);
    return lib->lib_OpenCnt == 0 && (lib->lib_Flags & LIBF_DELEXP);
}

BPTR _set_libexpungebase(const struct __aros_libfuncs_sets *s, struct Library *lib,
                         BPTR seglist, struct Library **oopbaseptr, struct ExecBase *SysBase)
{
    if(lib->lib_OpenCnt != 0 || !netmodule_callset(s->init.expungelib, -1, 1, lib)) {
        lib->lib_Flags |= LIBF_DELEXP;
        return BNULL;
    }
    Remove((struct Node *)lib);
    netmodule_setsexit(&s->init, SysBase);
    netmodule_freebase(lib, SysBase);
    return seglist;
}

/*
 * Called by the stack, in its task, after it opened this library: take the
 * core's table and hand the descriptor back. No name is looked up: the
 * table's layout is the list net/netcore.def, the same file the core's
 * table is made from, and the version says how far down that list this
 * module was built to read.
 */
AROS_LH1(LONG, NetModuleAttach,
         AROS_LHA(struct netmodule **, desc, A0),
         struct Library *, NetModuleBase, 5, NetModule)
{
    AROS_LIBFUNC_INIT

    const struct netcore *nc;

    if(NetCoreBase != NULL)
        return NMA_RESULT(NMA_ATTACHED);
    NetCoreBase = OpenLibrary(NETCORE_LIBNAME, NETCORE_LIBVERSION);
    if(NetCoreBase == NULL)
        return NMA_RESULT(NMA_NOCORE);
    nc = NetCoreServices(NETCORE_VERSION);
    if(nc == NULL || nc->nc_version < NETCORE_VERSION ||
            nc->nc_size < sizeof(struct netcore)) {
        CloseLibrary(NetCoreBase);
        NetCoreBase = NULL;
        return NMA_RESULT(NMA_OLDCORE);
    }
    /* our part of the table; a newer core has more at the end */
    CopyMem((APTR)nc, &__netcore, sizeof(struct netcore));
    __netcore.nc_size = sizeof(struct netcore);
    __netcore.nc_version = NETCORE_VERSION;
    *desc = &netmodule_this;
    return NMA_OK;

    AROS_LIBFUNC_EXIT
}

/* Called by the stack once the module is stopped and off the registry. */
AROS_LH0(void, NetModuleDetach,
         struct Library *, NetModuleBase, 6, NetModule)
{
    AROS_LIBFUNC_INIT

    if(NetCoreBase != NULL) {
        CloseLibrary(NetCoreBase);
        NetCoreBase = NULL;
    }

    AROS_LIBFUNC_EXIT
}

/*
 * One opener: the stack. A second opener would keep the library - and its
 * variables, as the last run left them - in memory after the stack let go,
 * and the next stack would attach a module with stale data.
 */
static int NetModule_Open(struct Library *NetModuleBase)
{
    return NetModuleBase->lib_OpenCnt == 0;
}

/* Never out of memory while the core may still hold a pointer into us. */
static int NetModule_Expunge(struct Library *NetModuleBase)
{
    return NetCoreBase == NULL && !(netmodule_this.nm_flags & NMF_STARTED);
}

ADD2OPENLIB(NetModule_Open, 0)
ADD2EXPUNGELIB(NetModule_Expunge, 0)
