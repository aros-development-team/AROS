/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.

    netprefs_library.c - In-process registration library for protocol address
    modules.  Follows the same pattern as SysExplorer's sysexp_library.c:
    the main app creates this library via MakeLibrary() and passes its base
    pointer to every loaded module so they can register themselves.
*/

#include <aros/debug.h>

#include <exec/memory.h>
#include <utility/tagitem.h>

#include <proto/netprefs.h>

#include <proto/alib.h>
#include <proto/exec.h>

#include <string.h>

#include "netprefs_intern.h"
#include "protocols.h"

/* -----------------------------------------------------------------------
 * Null function (used for library Open / Close / Expunge / Reserved slots)
 * ----------------------------------------------------------------------- */
AROS_LH0(void, Null,
         struct NetPrefsBase *, NetPrefsBase, 0, LIB)
{
    AROS_LIBFUNC_INIT
    return;
    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
 * RegisterProtoModule — vector 5
 * Called from a module's ModuleInit() to register itself.
 * ----------------------------------------------------------------------- */
AROS_LH2(void, RegisterProtoModule,
         AROS_LHA(struct NetPrefsModule *, Module, A0),
         AROS_LHA(APTR, ModBase, A1),
         struct NetPrefsBase *, NetPrefsBase, 5, NetPrefs)
{
    AROS_LIBFUNC_INIT

    D(bug("[netprefs.library] %s()\n", __func__));

    if (Module)
    {
        struct NetPrefsIntModule *im =
            AllocMem(sizeof(struct NetPrefsIntModule), MEMF_ANY);
        if (im)
        {
            CopyMem(Module, im, sizeof(struct NetPrefsModule));
            im->npim_ModuleBase = ModBase;
            Enqueue(&NetPrefsBase->npb_Modules,
                    &im->npim_Module.npm_Node);
        }
    }

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
 * RegisterBase — vector 6
 * Register a named pointer (e.g. a MUI_CustomClass, a function, etc.)
 * ----------------------------------------------------------------------- */
AROS_LH2(void, RegisterBase,
         AROS_LHA(CONST_STRPTR, BaseID, A0),
         AROS_LHA(APTR, Base, A1),
         struct NetPrefsBase *, NetPrefsBase, 6, NetPrefs)
{
    AROS_LIBFUNC_INIT

    D(bug("[netprefs.library] %s('%s')\n", __func__, BaseID));

    struct NetPrefsIntBase *ib =
        AllocMem(sizeof(struct NetPrefsIntBase), MEMF_ANY);
    if (ib)
    {
        ib->npib_Node.ln_Name = (char *)BaseID;
        ib->npib_Base = Base;
        AddTail(&NetPrefsBase->npb_GenericBases, &ib->npib_Node);
    }

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
 * GetBase — vector 7
 * Retrieve a named pointer previously stored via RegisterBase(), or a
 * module base registered via RegisterProtoModule().
 * ----------------------------------------------------------------------- */
AROS_LH1(APTR, GetBase,
         AROS_LHA(CONST_STRPTR, BaseID, A0),
         struct NetPrefsBase *, NetPrefsBase, 7, NetPrefs)
{
    AROS_LIBFUNC_INIT

    struct NetPrefsIntBase *ib;
    struct NetPrefsIntModule *im;

    D(bug("[netprefs.library] %s('%s')\n", __func__, BaseID));

    ForeachNode(&NetPrefsBase->npb_GenericBases, ib)
    {
        if (!strcmp(ib->npib_Node.ln_Name, BaseID))
            return ib->npib_Base;
    }
    ForeachNode(&NetPrefsBase->npb_Modules, im)
    {
        if (!strcmp(im->npim_Module.npm_Node.ln_Name, BaseID))
            return im->npim_ModuleBase;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
 * RegisterProtoHandler — vector 8
 * Register a protocol-address handler.  Called by modules during their
 * Startup callback.
 * ----------------------------------------------------------------------- */
AROS_LH4(void, RegisterProtoHandler,
         AROS_LHA(CONST_STRPTR, name, A0),
         AROS_LHA(ULONG, family, D0),
         AROS_LHA(struct MUI_CustomClass *, winclass, A1),
         AROS_LHA(NETPREFS_WRITETOKENS, writetokens, A2),
         struct NetPrefsBase *, NetPrefsBase, 8, NetPrefs)
{
    AROS_LIBFUNC_INIT

    D(bug("[netprefs.library] %s('%s', family=%lu)\n",
          __func__, name, (unsigned long)family));

    struct ProtoHandlerNode *ph =
        AllocMem(sizeof(struct ProtoHandlerNode), MEMF_CLEAR);
    if (ph)
    {
        ph->ph_Node.ln_Name = (char *)name;
        ph->ph_Node.ln_Pri  = (BYTE)family;
        ph->ph_Family        = family;
        ph->ph_WinClass      = winclass;
        ph->ph_WriteTokens   = writetokens;
        Enqueue(&NetPrefsBase->npb_ProtoHandlers, &ph->ph_Node);
    }

    AROS_LIBFUNC_EXIT
}

/* -----------------------------------------------------------------------
 * Library function table — built by hand (like sysexp_library.c).
 * ----------------------------------------------------------------------- */
void *NetPrefsLibrary_funcTable[] =
{
    AROS_SLIB_ENTRY(Null, LIB, 0),                     /* Open()    */
    AROS_SLIB_ENTRY(Null, LIB, 0),                     /* Close()   */
    AROS_SLIB_ENTRY(Null, LIB, 0),                     /* Expunge() */
    AROS_SLIB_ENTRY(Null, LIB, 0),                     /* Reserved  */
    AROS_SLIB_ENTRY(RegisterProtoModule, NetPrefs, 5),
    AROS_SLIB_ENTRY(RegisterBase, NetPrefs, 6),
    AROS_SLIB_ENTRY(GetBase, NetPrefs, 7),
    AROS_SLIB_ENTRY(RegisterProtoHandler, NetPrefs, 8),
    (void *)-1
};

/* -----------------------------------------------------------------------
 * netprefs_initlib — called by the main app to create the NetPrefsBase.
 * ----------------------------------------------------------------------- */
void netprefs_initlib(struct NetPrefsBase **basePtr)
{
    struct NetPrefsBase *base;

    D(bug("[NetPrefs] %s()\n", __func__));

    base = (struct NetPrefsBase *)MakeLibrary(
               NetPrefsLibrary_funcTable,
               NULL, NULL,
               sizeof(struct NetPrefsBase),
               BNULL);

    D(bug("[NetPrefs] %s: NetPrefsBase @ %p\n", __func__, base));

    if (base)
    {
        NEWLIST(&base->npb_GenericBases);
        NEWLIST(&base->npb_Modules);
        NEWLIST(&base->npb_ProtoHandlers);
    }

    *basePtr = base;
}

/* Global accessor for prefsdata.c and other non-MUI code */
static struct NetPrefsBase *g_NetPrefsBase = NULL;

void NetPrefs_SetBase(struct NetPrefsBase *base)
{
    g_NetPrefsBase = base;
}

struct NetPrefsBase *NetPrefs_GetBase(void)
{
    return g_NetPrefsBase;
}
