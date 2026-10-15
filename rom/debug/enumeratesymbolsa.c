/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.

    Desc:
 */

#include <libraries/debug.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <aros/debug.h>

#include "debug_intern.h"

static void EnumerateModules(struct Hook * handler, struct Library * DebugBase);

/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH2(void, EnumerateSymbolsA,

/*  SYNOPSIS */
        AROS_LHA(struct Hook *, handler, A0),
        AROS_LHA(struct TagItem *, tags, A1),

/*  LOCATION */
        struct Library *, DebugBase, 8, Debug)

/*  FUNCTION
    Function will call the handler hook for all symbols from kickstart and
    loaded modules.

    The message that is passed to hook contains a pointer to struct SymbolInfo.

    INPUTS
        handler - Hook called once for each symbol.
        tags    - Reserved for future use. Currently ignored.

    RESULT

    NOTES
        In normal task context the module database is held shared while the
        handler hook is called. On guarded supervisor targets a non-blocking
        read guard is held for the callback; if a writer owns the database,
        enumeration is skipped instead of waiting.

        The hook must not call RegisterModule() or UnregisterModule(), or
        otherwise require exclusive access to the module database.

        The SymbolInfo message and the strings it references are borrowed and
        are valid only for the duration of the hook call.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DebugBase *debugBase = DBGBASE(DebugBase);
    BOOL super;
    BOOL resolverlock = FALSE;
    BOOL enumerate = TRUE;

    /* We can be called in supervisor mode. No semaphores in the case! */
    super = KrnIsSuper();
    if (!super)
        ObtainSemaphoreShared(&debugBase->db_ModSem);
    else if (debugBase->db_SymResolverABI >= KRN_SYMRESOLVER_ABI_LEASE)
    {
        if (KrnSpinTryLock(&debugBase->db_ResolverSpin, SPINLOCK_MODE_READ))
            resolverlock = TRUE;
        else
            enumerate = FALSE;
    }

    if (enumerate)
        EnumerateModules(handler, DebugBase);

    if (!super)
        ReleaseSemaphore(&debugBase->db_ModSem);
    else if (resolverlock)
        KrnSpinUnLock(&debugBase->db_ResolverSpin);

    AROS_LIBFUNC_EXIT
}

static inline void callhook(struct Hook * handler, CONST_STRPTR modname, CONST_STRPTR symname,
        APTR start, APTR end)
{
    struct SymbolInfo sinfo = {0};

    sinfo.si_Size           = sizeof(struct SymbolInfo);
    sinfo.si_ModuleName     = modname;
    sinfo.si_SymbolName     = symname;
    sinfo.si_SymbolStart    = start;
    sinfo.si_SymbolEnd      = end;

    CALLHOOKPKT(handler, NULL, &sinfo);
}

static void EnumerateModules(struct Hook * handler, struct Library * DebugBase)
{
    struct DebugBase *debugBase = DBGBASE(DebugBase);
    module_t *mod;

    ForeachNode(&debugBase->db_Modules, mod)
    {
        dbg_sym_t *sym = mod->m_symbols;
        ULONG i;

        D(bug("[Debug] Checking module %s\n", mod->m_name));

        for (i = 0; i < mod->m_symcnt; i++)
        {
            APTR highest = sym[i].s_highest;

            /* Symbols with zero length have zero in s_highest */
            if (!highest)
                highest = sym[i].s_lowest;

            callhook(handler, mod->m_name, sym[i].s_name, sym[i].s_lowest, sym[i].s_highest);
        }
    }
}
