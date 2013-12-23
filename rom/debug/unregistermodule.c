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

static BOOL ScanAndRemoveSegment(module_t *mod, BPTR segList);

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

    module_t *mod;
    module_t *modlast = NULL; /* In most (all?) cases, all segments will be within one module */

    D(bug("[Debug] UnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    while (segList)
    {
        if (modlast && ScanAndRemoveSegment(modlast, segList))
        {
            /* Nothing */
        }
        else
        {
            struct Node * tmpnode;

            ForeachNodeSafe(&DBGBASE(DebugBase)->db_LoadedModules, mod, tmpnode)
            {
                if (ScanAndRemoveSegment(mod, segList))
                {
                    modlast = mod;
                    break;
                }
            }
        }

        /* Advance to next DOS segment */
        segList = *(BPTR *)BADDR(segList);
    }

    ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    AROS_LIBFUNC_EXIT
}

static BOOL ScanAndRemoveSegment(module_t *mod, BPTR segList)
{
    struct segment * seg = NULL;
    LONG i, j;

    for (i = 0; i < mod->m_segcnt; i++)
    {
        seg = mod->m_segments[i];

        if (seg->s_seg == segList)
        {
            D(bug("[Debug] Removing segment 0x%p\n", segList));
            /* "Shrink" array of segments so that at any given time the array is valid for
             * binary search
             */
            for (j = i;j < mod->m_segcnt - 1; j++)
                mod->m_segments[j] = mod->m_segments[j + 1];

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
            }

            FreeMem(seg, sizeof(struct segment));

            return TRUE;
        }
    }

    return FALSE;
}
