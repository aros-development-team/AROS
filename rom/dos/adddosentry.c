/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <proto/utility.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, AddDosEntry,

/*  SYNOPSIS */
	AROS_LHA(struct DosList *, dlist, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 113, Dos)

/*  FUNCTION
	Adds a given dos list entry to the dos list. Automatically
	locks the list for writing. There may be not more than one device
	or assign node of the same name. There are no restrictions on
	volume nodes.

    INPUTS
	dlist - pointer to dos list entry.

    RESULT
	!= 0 if all went well, 0 otherwise.

    NOTES
	Since anybody who wants to use a device or volume node in the
	dos list has to lock the list, filesystems may be called with
	the dos list locked. So if you want to add a dos list entry
	out of a filesystem don't just wait on the lock but serve all
	incoming requests until the dos list is free instead.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    LONG            success = 1;
    struct DosList *dl;

    if (dlist == NULL) return success;

    dl = LockDosList(LDF_ALL | LDF_WRITE);

    if(dlist->dol_Type != DLT_VOLUME)
    {
	while(TRUE)
	{
	    dl = dl->dol_Next;

	    if(dl == NULL)
		break;

	    if(dl->dol_Type != DLT_VOLUME &&
	       !Stricmp(dl->dol_DevName, dlist->dol_DevName))
	    {
		success = 0;
		break;
	    }
	}
    }

    if(success)
    {
	dlist->dol_Next = DOSBase->dl_DevInfo;
	DOSBase->dl_DevInfo = dlist;
    }

    UnLockDosList(LDF_ALL | LDF_WRITE);

    return success;    

    AROS_LIBFUNC_EXIT
} /* AddDosEntry */
