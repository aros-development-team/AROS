/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.8  2000/11/26 07:52:11  SDuvan
    Layout update

    Revision 1.7  1998/10/20 16:44:28  hkiel
    Amiga Research OS

    Revision 1.6  1997/01/27 00:36:14  ldp
    Polish

    Revision 1.5  1996/12/09 13:53:21  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.4  1996/10/24 15:50:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.3  1996/08/13 13:52:44  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.2  1996/08/01 17:40:47  digulla
    Added standard header for all files

    Desc:
    Lang: English
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(struct DosList *, AttemptLockDosList,

/*  SYNOPSIS */
	AROS_LHA(ULONG, flags, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 111, Dos)

/*  FUNCTION
	Tries to get a lock on some of the dos lists. If all went
	well a handle is returned that can be used for FindDosEntry().
	Don't try to busy wait until the lock can be granted - use
	LockDosList() instead.

    INPUTS
	flags  --  what lists to lock

    RESULT
	Handle to the dos list or NULL. This is not a direct pointer
	to the first list element but to a pseudo element instead.

    NOTES

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

    if(flags & LDF_WRITE)
    {
	if(!AttemptSemaphore(&DOSBase->dl_DosListLock))
	    return NULL;
    }
    else
    {
	if(!AttemptSemaphoreShared(&DOSBase->dl_DosListLock))
	    return NULL;
    }

    return (struct DosList *)&DOSBase->dl_DevInfo;

    AROS_LIBFUNC_EXIT
} /* AttemptLockDosList */
