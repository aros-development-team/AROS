/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * arostcp.library: what the core offers a module of the stack whose code
 * is in a library file of its own (net/netcore.h).
 *
 * The library is made by the stack program with MakeLibrary() and
 * published with AddLibrary(), the way api/amiga_api.c makes
 * bsdsocket.library, miami.library and netservices.library. It has the
 * four standard vectors and one of its own, NetCoreServices(), which hands
 * out the table of the core's functions and variables. The table is
 * constant data: every address in it is resolved when the program is
 * linked, and no name is looked up when a module is loaded.
 *
 * A module holds the library open while it is attached to the core, so the
 * open count is the number of attached modules. The base is taken out of
 * the system and freed only by the stack itself, at shutdown, and only
 * when that count is zero: a module's copy of the table points into this
 * program.
 */

#include <conf.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <kern/amiga_includes.h>

#include <exec/libraries.h>
#include <aros/libcall.h>

#include <version.h>
#include <net/netcore.h>

/*
 * The table. `&name' is checked against the member's type, which comes
 * from the same declaration: a name that does not exist, or a function
 * whose type differs from its prototype, does not compile.
 */
static const struct netcore netcore_services = {
    sizeof(struct netcore),
    NETCORE_VERSION,
#define NC(name)    &name,
#include <net/netcore.def>
#undef NC
};

static struct Library *MasterNetCoreBase = NULL;
static BOOL netcore_closing = FALSE;    /* the stack itself asks for the expunge */

AROS_LH1(struct Library *, Open,
         AROS_LHA(ULONG, version, D0),
         struct Library *, NetCoreBase, 1, NetCore)
{
    AROS_LIBFUNC_INIT

    if(netcore_closing)
        return NULL;            /* the stack is ending: nothing may attach any more */
    NetCoreBase->lib_OpenCnt++;
    NetCoreBase->lib_Flags &= ~LIBF_DELEXP;
    return NetCoreBase;

    AROS_LIBFUNC_EXIT
}

AROS_LH0(BPTR, Close, struct Library *, NetCoreBase, 2, NetCore)
{
    AROS_LIBFUNC_INIT

    if(NetCoreBase->lib_OpenCnt > 0)
        NetCoreBase->lib_OpenCnt--;
    return BNULL;

    AROS_LIBFUNC_EXIT
}

/*
 * Called under Forbid(), by exec when memory is short and by
 * netcore_deinit(). Refused - only marked - unless it is the stack that
 * asks and no module holds the library.
 */
AROS_LH0(BPTR, Expunge, struct Library *, NetCoreBase, 3, NetCore)
{
    AROS_LIBFUNC_INIT

    if(!netcore_closing || NetCoreBase->lib_OpenCnt != 0) {
        NetCoreBase->lib_Flags |= LIBF_DELEXP;
        return BNULL;
    }
    Remove(&NetCoreBase->lib_Node);
    FreeMem((UBYTE *)NetCoreBase - NetCoreBase->lib_NegSize,
            NetCoreBase->lib_NegSize + NetCoreBase->lib_PosSize);
    MasterNetCoreBase = NULL;
    return BNULL;               /* no segment list: the code is the stack program */

    AROS_LIBFUNC_EXIT
}

AROS_LH0(APTR, Reserved, struct Library *, NetCoreBase, 4, NetCore)
{
    AROS_LIBFUNC_INIT
    return NULL;
    AROS_LIBFUNC_EXIT
}

/*
 * The table, for a module built against version `version' of it or an
 * older one. The table only grows at the end, so a module built for an
 * older version finds everything it knows where it expects it.
 */
AROS_LH1(const struct netcore *, NetCoreServices,
         AROS_LHA(ULONG, version, D0),
         struct Library *, NetCoreBase, 5, NetCore)
{
    AROS_LIBFUNC_INIT

    return (version <= NETCORE_VERSION) ? &netcore_services : NULL;

    AROS_LIBFUNC_EXIT
}

typedef VOID (* f_void)();

static const f_void NetCore_FuncTable[] = {
#ifdef __MORPHOS__
    FUNCARRAY_32BIT_NATIVE,
#endif
    (f_void)AROS_SLIB_ENTRY(Open,            NetCore, 1),
    (f_void)AROS_SLIB_ENTRY(Close,           NetCore, 2),
    (f_void)AROS_SLIB_ENTRY(Expunge,         NetCore, 3),
    (f_void)AROS_SLIB_ENTRY(Reserved,        NetCore, 4),
    (f_void)AROS_SLIB_ENTRY(NetCoreServices, NetCore, 5),
    (f_void) - 1
};

/*
 * Make the library and publish it. Unlike bsdsocket.library it is visible
 * from here on, not only when the stack is up: the modules loaded while
 * the stack starts open it.
 */
int
netcore_init(void)
{
    struct Library *lib;
    struct Node *other;

    if(MasterNetCoreBase != NULL)
        return (1);

    /* a second stack: the name is taken, and modules could not tell the two apart */
    Forbid();
    other = FindName(&SysBase->LibList, NETCORE_LIBNAME);
    Permit();
    if(other != NULL) {
        __log(LOG_ERR, "%s is already there: is another copy of the stack running?",
              NETCORE_LIBNAME);
        return (0);
    }

    lib = MakeLibrary((APTR)NetCore_FuncTable, NULL, NULL, sizeof(struct Library), BNULL);
    if(lib == NULL)
        return (0);
    lib->lib_Node.ln_Type = NT_LIBRARY;
    lib->lib_Node.ln_Name = (APTR)NETCORE_LIBNAME;
    lib->lib_Flags = (LIBF_SUMUSED | LIBF_CHANGED);
    lib->lib_Version = NETCORE_LIBVERSION;
    lib->lib_Revision = NETCORE_LIBREVISION;
    lib->lib_IdString = (APTR)RELEASESTRING NETCORE_LIBNAME " "
                        STR(NETCORE_LIBVERSION) "." STR(NETCORE_LIBREVISION) " (" DATE ")";

    netcore_closing = FALSE;
    MasterNetCoreBase = lib;
    AddLibrary(lib);
    return (1);
}

/*
 * Take the library back. The module libraries have been unloaded, or cut
 * off, before (netmodule_unload_all()), so nothing of the stack holds it.
 * Should somebody else have it open, its memory is not freed under them;
 * but it leaves the system's list, because its vectors are about to go
 * with the program, and exec calls the Expunge vector of every library on
 * that list when memory runs short.
 */
void
netcore_deinit(void)
{
    struct Library *lib = MasterNetCoreBase;

    if(lib == NULL)
        return;
    netcore_closing = TRUE;
    RemLibrary(lib);            /* our Expunge vector, under exec's Forbid() */
    if(MasterNetCoreBase != NULL) {
        Forbid();
        Remove(&lib->lib_Node);
        Permit();
        MasterNetCoreBase = NULL;
        __log(LOG_ERR, "%s is still open %ld time(s) as the stack ends; its base is not freed.",
              NETCORE_LIBNAME, (long)lib->lib_OpenCnt);
    }
}
