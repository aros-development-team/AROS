/*
    Copyright © <year>, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include LC_LIBDEFS_FILE

/*****************************************************************************

    NAME */
#include <proto/setpatch.h>

        AROS_LH1(VOID, RemPatch,

/*  SYNOPSIS */
        AROS_LHA(struct PatchEntry *, pe, A0),

/*  LOCATION */
        struct SetPatchBase *, SetPatchBase, 6, Setpatch)

/*  FUNCTION

    INPUTS

        Patch entry to remove (from FindPatch() or AddPatch())

    RESULT

        Patch entry is removed from the master patch list

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct PatchEntry *pematch;

    ObtainSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);
    ForeachNode(&SetPatchBase->sp_Patch3.sp_PatchList, pematch) {
        if (pe == pematch) {
            Remove((struct Node *)pe);
            break;
        }
    }
    ReleaseSemaphore(&SetPatchBase->sp_Patch3.sp_Semaphore);

    AROS_LIBFUNC_EXIT
}

