/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>

#include <proto/exec.h>

#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(BOOL, AssignAddToList,

/*  SYNOPSIS */
        AROS_LHA(CONST_STRPTR, name, D1),
        AROS_LHA(BPTR, lock, D2),
        AROS_LHA(ULONG, position, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 226, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct DosList     *dl;
    struct AssignList **al, *newal;
    int cnt = 0;
    BOOL retval = DOSTRUE;

    D(bug("[DOS] %s('%s', 0x%p, %d)\n", __func__, name, lock, position);)

    if(lock == BNULL)
        return DOSFALSE;

    dl = LockDosList(LDF_ASSIGNS | LDF_WRITE);
    dl = FindDosEntry(dl, name, LDF_ASSIGNS);

    if (dl != NULL)
    {
	D(bug("[DOS] %s: dl = 0x%p, type = %08x\n", __func__, dl, dl->dol_Type);)
	if  (!(dl->dol_Type == DLT_VOLUME || dl->dol_Type == DLT_DEVICE))
	{    
	    newal = AllocVec(sizeof(struct AssignList), MEMF_PUBLIC | MEMF_CLEAR);
	    if(newal != NULL)
	    {
		for(al = &dl->dol_misc.dol_assign.dol_List; *al && (cnt < position + 1); al = &((*al)->al_Next), cnt++);
		if (cnt == 0)
		{
		    char lnTmp[128];

		    D(bug("[DOS] %s: replacing top level lock 0x%p\n", __func__, dl->dol_Lock);)
		    if ((newal->al_Lock = dl->dol_Lock) == BNULL)
		    {
			newal->al_Lock = Lock(dl->dol_misc.dol_assign.dol_AssignName, SHARED_LOCK);
		    }
		    dl->dol_Lock = lock;
		    if (NameFromLock(lock, lnTmp, sizeof(lnTmp)))
		    {
			STRPTR s2, oldin = dl->dol_misc.dol_assign.dol_AssignName;

			s2 = (STRPTR)AllocVec(strlen(lnTmp) + 1, MEMF_PUBLIC | MEMF_CLEAR);
			if (s2 != NULL)
			{
			    Strlcpy(s2, lnTmp, strlen(lnTmp) + 1);
			    dl->dol_misc.dol_assign.dol_AssignName = s2;
			    FreeVec(oldin);
			}
			else
			{
			    SetIoErr(ERROR_NO_FREE_STORE);
			}
		    }
		}
		else
		{
		    D(bug("[DOS] %s: inseting @ %d\n", __func__, cnt);)
		    newal->al_Lock = lock;
		}

		if (*al)
		    newal->al_Next = *al;
		*al = newal;
	    }
	    else
	    {
		SetIoErr(ERROR_NO_FREE_STORE);
		retval = DOSFALSE;
	    }
	}
	else
	{
	    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	    retval = DOSFALSE;
	}
    }
    else
    {
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	retval = DOSFALSE;
    }
    UnLockDosList(LDF_ASSIGNS | LDF_WRITE);

    return retval;

    AROS_LIBFUNC_EXIT
} /* AssignAddToList */
