/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

#include <aros/debug.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/lists.h>
#include <proto/exec.h>

#include <string.h>

#include "debug_intern.h"

static module_t * FindModule(BPTR segList, struct Library * DebugBase);
static LONG FindIndex(module_t * mod, BPTR segList);
static VOID RemoveSegmentRange(module_t * mod, LONG firstidx, LONG count);

/*****************************************************************************

    NAME */
#include <proto/debug.h>

        AROS_LH1(void, UnregisterModule,

/*  SYNOPSIS */
        AROS_LHA(BPTR, segList, A0),

/*  LOCATION */
        struct Library *, DebugBase, 6, Debug)

/*  FUNCTION
        Remove previously registered module from the debug information database

    INPUTS
        segList - DOS segment list for the module to remove

    RESULT
        None

    NOTES
        The function correctly supports partial removal of the module
        (when an existing seglist is broken and only a part of the module
        is unloaded).

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    module_t *mod = NULL;
    LONG i = 0, rangestart = -1;

    D(bug("[Debug] UnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    while (segList)
    {
        if (mod == NULL) /* Search for new module */
            mod = FindModule(segList, DebugBase);

        if (mod)
        {
            if (rangestart == -1) /* Search for new index */
                i = rangestart = FindIndex(mod, segList);

            /* Optimization assumes order of segments is similar to order of DOS segments */
            if ((i >= mod->m_segcnt) || (mod->m_segments[i]->s_seg != segList))
            {
                /* Order broken, clear ordered segments */
                RemoveSegmentRange(mod, rangestart, (i - rangestart));

                /* Restart */
                i = rangestart = FindIndex(mod, segList);
            }

            i++;
        }

        /* Advance to next DOS segment */
        segList = *(BPTR *)BADDR(segList);
    }

    if (mod != NULL && rangestart > -1)
        RemoveSegmentRange(mod, rangestart, (i - rangestart));

    ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    AROS_LIBFUNC_EXIT
}

static module_t * FindModule(BPTR segList, struct Library * DebugBase)
{
    module_t *mod;
    LONG i;

    ForeachNode(&DBGBASE(DebugBase)->db_Modules, mod)
    {
       for (i = 0; i < mod->m_segcnt; i++)
       {
           if (mod->m_segments[i]->s_seg == segList)
           {
               return mod;
           }
       }
    }

    return NULL;
}

static LONG FindIndex(module_t * mod, BPTR segList)
{
    LONG i;

    for (i = 0; i < mod->m_segcnt; i++)
    {
        if (mod->m_segments[i]->s_seg == segList)
        {
            return i;
        }
    }

    return -1;
}

static VOID RemoveSegmentRange(module_t * mod, LONG firstidx, LONG count)
{
    struct segment * seg;
    LONG i;

    for (i = 0 ; i < count ; i++)
    {
        seg = mod->m_segments[i + firstidx];

        FreeMem(seg, sizeof(struct segment));

        /* If module's segment count reached 0, remove the whole
           module information */
        if (--mod->m_segcnt == 0)
        {
            D(bug("[Debug] Removing module %s\n", mod->m_name));

            /* Free associated symbols */
            if (mod->m_symbols) {
                D(bug("[Debug] Removing symbol table 0x%p\n", mod->m_symbols));
                FreeVec(mod->m_symbols);
            }

            /* Free associated string tables */
            if (mod->m_str) {
                D(bug("[Debug] Removing symbol name table 0x%p\n", mod->m_str));
                FreeVec(mod->m_str);
            }
            if (mod->m_shstr) {
                D(bug("[Debug] Removing section name table 0x%p\n", mod->m_str));
                FreeVec(mod->m_shstr);
            }

            Remove((struct Node *)mod);
            FreeVec(mod->m_segments);
#if AROS_MODULES_DEBUG
            FreeVec(mod->m_seggdbhlp);
#endif
            /* Free module descriptor at last */
            FreeVec(mod);

            return;
        }
    }

    /* "Shrink" array of segments so that at any given time the array is valid for
     * binary search
     */
    for (i = firstidx;i < mod->m_segcnt; i++)
        mod->m_segments[i] = mod->m_segments[i + count];

}
