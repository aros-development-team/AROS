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

        AROS_LH1(struct PatchEntry *, FindPatch,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, A0),

/*  LOCATION */
        struct SetPatchBase *, SetPatchBase, 7, Setpatch)

/*  FUNCTION

    INPUTS

        name - Name of the patch to search for

    RESULT

        Pointer to the patch entry if found, or NULL

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PatchEntry *pe, *peret = NULL;

    ObtainSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);
    ForeachNode(&SetPatchBase->sp_Patch3.sp_PatchList, pe) {
        if (strcmp(pe->pe_Name,name) == 0) {
            peret = pe;
            break;
        }
    }
    ReleaseSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);

    return peret;

    AROS_LIBFUNC_EXIT
}

