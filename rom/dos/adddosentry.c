/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1996/10/24 15:50:23  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/10/10 13:18:38  digulla
    Use dl_DevNam instaed of dl_Name (STRPTR and BPTR) (Fleischer)

    Revision 1.3  1996/08/13 13:52:44  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:47  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <clib/utility_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

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
	!=0 if all went well, 0 otherwise.

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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    LONG success=1;
    struct DosList *dl;

    dl=LockDosList(LDF_ALL|LDF_WRITE);
    if(dlist->dol_Type!=DLT_VOLUME)
    {
	dl=FindDosEntry(dl,dlist->dol_DevName,LDF_DEVICES|LDF_ASSIGNS|LDF_WRITE);
	if(dl!=NULL)
	    success=0;
    }
    if(success)
    {
	dlist->dol_Next=DOSBase->dl_DevInfo;
	DOSBase->dl_DevInfo=dlist;
    }
    UnLockDosList(LDF_ALL|LDF_WRITE);

    return success;    
    AROS_LIBFUNC_EXIT
} /* AddDosEntry */
