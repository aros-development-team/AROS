/*
    (C) 1995-96 AROS - The Amiga Replacement OS
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

	AROS_LH2(BOOL, AssignAdd,

/*  SYNOPSIS */
	AROS_LHA(STRPTR, name, D1),
	AROS_LHA(BPTR  , lock, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 105, Dos)

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
    struct DosList *dl;
    struct AssignList **al, *newal;

    if (!lock)
	return 0;

    dl = LockDosList(LDF_ASSIGNS|LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ASSIGNS);
    if ((dl == NULL) || (dl->dol_Type != DLT_DIRECTORY))
    {
	UnLockDosList(LDF_ASSIGNS|LDF_WRITE);
	SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	return 0;
    }

    newal = AllocVec(sizeof(struct AssignList), MEMF_PUBLIC|MEMF_CLEAR);
    if (newal == NULL)
    {
	UnLockDosList(LDF_ASSIGNS|LDF_WRITE);
	SetIoErr(ERROR_NO_FREE_STORE);
	return 0;
    }

    newal->al_Lock = lock;
    for (al = &dl->dol_misc.dol_assign.dol_List; *al; al = &((*al)->al_Next));

    *al = newal;
    UnLockDosList(LDF_ASSIGNS|LDF_WRITE);
    return 1;
    AROS_LIBFUNC_EXIT
} /* AssignAdd */
