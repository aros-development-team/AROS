/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Create an assign.
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/filesystem.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(BOOL, AssignLock,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, name, D1),
	AROS_LHA(BPTR,   lock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 102, Dos)

/*  FUNCTION
	Create an assign from a given name to a lock. Replaces any older
	assignments from that name, 0 cancels the assign completely. Do not
	use or free the lock after calling this function - it becomes
	the assign and will be freed by the system if the assign is removed.

    INPUTS
	name -- NUL terminated name of the assign.
	lock -- Lock to assigned directory.

    RESULT
	!= 0 success, 0 on failure. IoErr() gives additional information
	in that case. The lock is not freed on failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    BOOL success = DOSTRUE;

    struct DosList    *dl, *newdl = NULL;
    struct FileHandle *fh = (struct FileHandle *)BADDR(lock);

    if (lock != NULL)
    {
	newdl = MakeDosEntry(name, DLT_DIRECTORY);

	if (newdl == NULL)
	{
	    return DOSFALSE;
	}

	newdl->dol_Ext.dol_AROS.dol_Unit   = fh->fh_Unit;
	newdl->dol_Ext.dol_AROS.dol_Device = fh->fh_Device;
	newdl->dol_Lock                    = lock;
    }

    dl = LockDosList(LDF_ALL | LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ALL);

    if (dl == NULL)
    {
	AddDosEntry(newdl);
    }
    else if (dl->dol_Type == DLT_DEVICE || dl->dol_Type == DLT_VOLUME)
    {
	dl = NULL;
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
	if (dl->dol_Lock)
	{
	    UnLock(dl->dol_Lock);
	}

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
    
    AROS_LIBFUNC_EXIT
} /* AssignLock */
