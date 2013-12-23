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
    
    struct segment *seg;

    D(bug("[Debug] UnregisterModule(0x%p)\n", segList));
    ObtainSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    while (segList)
    {
        ForeachNode(&DBGBASE(DebugBase)->db_Modules, seg)
        {
            if (seg->s_seg == segList)
            {
                module_t *mod = seg->s_mod;

                D(bug("[Debug] Removing segment 0x%p\n", segList));
                Remove((struct Node *)seg);

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

                break;
            }
        }
        /* Advance to next DOS segment */
        segList = *(BPTR *)BADDR(segList);
    }

    ReleaseSemaphore(&DBGBASE(DebugBase)->db_ModSem);

    AROS_LIBFUNC_EXIT
}
