/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Create a late-binding (deferred) assign.
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH2(BOOL, AssignLate,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(CONST_STRPTR, path, D2),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 103, Dos)

/*  FUNCTION
        Create an assign for the given name, which will be resolved upon the
        first reference to it. If this succeeds (i.e. the path exists and
        can be locked) it will be turned into an AssignLock() type assign.
        This way you can create assigns to unmounted volumes which will only
        be requested when accessed.

    INPUTS
        name  --  NULL terminated name of the assign.
        path  --  NULL terminated path to be resolved on the first reference.

    RESULT
        != 0 success, 0 on failure. IoErr() gives additional information
        in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Lock(), AssignAdd(), AssignPath(), AssignLock()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return assign_path(name, path, DLT_LATE, DOSBase);

    AROS_LIBFUNC_EXIT
} /* AssignLate */

/* Replace the assign called 'name' with 'newdl' and free the old entry with
 * its locks. Fails with ERROR_OBJECT_EXISTS, freeing 'newdl', if 'name' is a
 * device or a volume. */
static BOOL assign_install(CONST_STRPTR name, struct DosList *newdl, struct DosLibrary *DOSBase)
{
    BOOL success = DOSTRUE;
    struct DosList *dl;

    dl = LockDosList(LDF_ALL | LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ALL);

    if (dl == NULL)
    {
        AddDosEntry(newdl);
    }
    else if (dl->dol_Type == DLT_DEVICE || dl->dol_Type == DLT_VOLUME)
    {
        dl = NULL;
        FreeVec(newdl->dol_misc.dol_assign.dol_AssignName);
        FreeDosEntry(newdl);
        SetIoErr(ERROR_OBJECT_EXISTS);
        success = DOSFALSE;
    }
    else
    {
        RemDosEntry(dl);

        AddDosEntry(newdl);
    }

    if (dl != NULL)
    {
        UnLock(dl->dol_Lock);

        if (dl->dol_misc.dol_assign.dol_List != NULL)
        {
            struct AssignList *al, *oal;

            for (al = dl->dol_misc.dol_assign.dol_List; al; )
            {
                UnLock(al->al_Lock);
                oal = al;
                al = al->al_Next;
                FreeVec(oal);
            }
        }

        FreeVec(dl->dol_misc.dol_assign.dol_AssignName);
        FreeDosEntry(dl);
    }

    UnLockDosList(LDF_ALL | LDF_WRITE);

    return success;
}

/* The body of AssignLate() and AssignPath(): an assign that keeps a path */
BOOL assign_path(CONST_STRPTR name, CONST_STRPTR path, LONG type, struct DosLibrary *DOSBase)
{
    struct DosList *newdl;
    CONST_STRPTR    s2;
    STRPTR  pathcopy;
    ULONG   namelen;

    newdl = MakeDosEntry(name, type);

    if (newdl == NULL)
        return DOSFALSE;

    s2 = path;

    while(*s2++)
        ;

    namelen = s2 - path + 1;
    pathcopy = AllocVec(namelen, MEMF_PUBLIC | MEMF_CLEAR);

    if(pathcopy == NULL)
    {
        FreeDosEntry(newdl);
        SetIoErr(ERROR_NO_FREE_STORE);

        return DOSFALSE;
    }

    CopyMem(path, pathcopy, namelen);
    newdl->dol_misc.dol_assign.dol_AssignName = pathcopy;

    return assign_install(name, newdl, DOSBase);
}
