/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
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
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(STRPTR, path, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 103, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    BOOL result = 1;
    struct DosList *dl, *newdl;
    STRPTR s2, pathcopy;
    ULONG namelen;

    newdl = MakeDosEntry(name, DLT_LATE);
    if (newdl == NULL)
	return 0;

    s2 = path;
    while (*s2++)
	;

    namelen = s2-path+1;
    pathcopy = AllocVec(namelen, MEMF_PUBLIC|MEMF_CLEAR);
    if (pathcopy == NULL)
    {
	FreeDosEntry(newdl);
	SetIoErr(ERROR_NO_FREE_STORE);
	return 0;
    }

    CopyMem(path, pathcopy, namelen);
    newdl->dol_misc.dol_assign.dol_AssignName = pathcopy;
    dl = LockDosList(LDF_ALL|LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ALL);
    if (dl == NULL)
	AddDosEntry(newdl);
    else if (dl->dol_Type == DLT_VOLUME || dl->dol_Type == DLT_DEVICE)
    {
	dl = NULL;
	FreeVec(newdl->dol_misc.dol_assign.dol_AssignName);
	FreeDosEntry(newdl);
	SetIoErr(ERROR_OBJECT_EXISTS);
	result = 0;
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

    UnLockDosList(LDF_ALL|LDF_WRITE);
    return result;
    AROS_LIBFUNC_EXIT
} /* AssignLate */
