/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a directory to an assign.
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, AssignAdd,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(BPTR  , lock, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 105, Dos)

/*  FUNCTION
        Create a multi-directory assign, or adds to it if it already was one.
        Do not use or free the lock after calling this function - it becomes
        the assign and will be freed by the system when the assign is removed.

    INPUTS
        name - NULL terminated name of the assign.
        lock - Lock on the assigned directory.

    RESULT
        != 0 success, 0 on failure. IoErr() gives additional information
        in that case. The lock is not freed on failure.

    NOTES
        This will only work with an assign created with AssignLock() or
        a resolved AssignLate() assign.

    EXAMPLE

    BUGS

    SEE ALSO
        Lock(), AssignLock(), AssignPath(), AssignLate(), DupLock(),
        RemAssignList()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DosList     *dl;
    struct AssignList **al, *newal;

    if(lock == BNULL)
        return DOSFALSE;

    dl = LockDosList(LDF_ASSIGNS | LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ASSIGNS);

    if((dl == NULL) || (dl->dol_Type != DLT_DIRECTORY))
    {
        UnLockDosList(LDF_ASSIGNS | LDF_WRITE);
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);

        return DOSFALSE;
    }
    
    newal = AllocVec(sizeof(struct AssignList), MEMF_PUBLIC | MEMF_CLEAR);

    if(newal == NULL)
    {
        UnLockDosList(LDF_ASSIGNS | LDF_WRITE);
        SetIoErr(ERROR_NO_FREE_STORE);

        return DOSFALSE;
    }
    
    newal->al_Lock = lock;

    for(al = &dl->dol_misc.dol_assign.dol_List; *al; al = &((*al)->al_Next));

    *al = newal;
    UnLockDosList(LDF_ASSIGNS | LDF_WRITE);

    return DOSTRUE;

    AROS_LIBFUNC_EXIT
} /* AssignAdd */
