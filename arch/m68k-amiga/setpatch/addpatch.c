/*
    Copyright © <year>, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <string.h>

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
#include <proto/setpatch.h>

        AROS_LH1(BOOL, AddPatch,

/*  SYNOPSIS */
        AROS_LHA(struct PatchEntry *, pe, A0),

/*  LOCATION */
        struct SetPatchBase *, SetPatchBase, 5, Setpatch)

/*  FUNCTION

    INPUTS

        pe - Patch entry header

    RESULT

        TRUE  - Patch entry is added to the master patch list
        FALSE - Patch with matching name already in the list 

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BOOL found = FALSE;
    struct PatchEntry *tmp;

    ObtainSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);

    ForeachNode(&SetPatchBase->sp_Patch3.sp_PatchList, tmp) {
        if (strcmp(pe->pe_Name, tmp->pe_Name) == 0) {
            found = TRUE;
            break;
        }
    }

    if (!found)
        AddTail((struct List *)&SetPatchBase->sp_Patch3.sp_PatchList, (struct Node *)pe);

    ReleaseSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);

    return !found;

    AROS_LIBFUNC_EXIT
}

