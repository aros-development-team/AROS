/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
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
    loaded modules that match the given search criteria.

    The message that is passed to hook contains a pointer to struct SymbolInfo.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL super;

    /* We can be called in supervisor mode. No semaphores in the case! */
    super = KrnIsSuper();
    if (!super)
        ObtainSemaphoreShared(&DBGBASE(DebugBase)->db_ModSem);

    EnumerateModules(handler, DebugBase);

    if (!super)
        ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

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
    module_t *mod;

    ForeachNode(&DBGBASE(DebugBase)->db_Modules, mod)
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
