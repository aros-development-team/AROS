/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/01 17:40:54  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include <clib/exec_protos.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(struct DosList *, LockDosList,

/*  SYNOPSIS */
	__AROS_LA(ULONG, flags, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 109, Dos)

/*  FUNCTION
	Waits until the desired dos lists are free then gets a lock on them.
	A handle is returned that can be used for FindDosEntry().
	Calls to this function nest, i.e. you must call UnLockDosList()
	as often as you called LockDosList(). Always lock all lists
	at once - do not try to get a lock on one of them then on another.

    INPUTS
	flags - what lists to lock

    RESULT
	Handle to the dos list. This is not a direct pointer
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
	ObtainSemaphore(&DOSBase->dl_DosListLock);
    else
    	ObtainSemaphoreShared(&DOSBase->dl_DosListLock);
    return (struct DosList *)&DOSBase->dl_DevInfo;
    __AROS_FUNC_EXIT
} /* LockDosList */
