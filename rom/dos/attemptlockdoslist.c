/*
    $Id$
    $Log$
    Revision 1.1  1996/07/28 16:37:22  digulla
    Initial revision

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(struct DosList *, AttemptLockDosList,

/*  SYNOPSIS */
	__AROS_LA(ULONG, flags, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 111, Dos)

/*  FUNCTION
	Tries to get a lock on some of the dos lists. If all went
	well a handle is returned that can be used for FindDosEntry().
	Don't try to busy wait until the lock can be granted - use
	LockDosList() instead.

    INPUTS
	flags - what lists to lock

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
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)
    if(flags&LDF_WRITE)
    {
	if(!AttemptSemaphore(&DOSBase->dl_DosListLock))
	    return NULL;
    }else
    	if(!AttemptSemaphoreShared(&DOSBase->dl_DosListLock))
    	    return NULL;
    return (struct DosList *)&DOSBase->dl_DevInfo;
    __AROS_FUNC_EXIT
} /* AttemptLockDosList */
