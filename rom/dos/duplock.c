/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/08/12 14:20:37  digulla
    Added aliases

    Revision 1.2  1996/08/01 17:40:49  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <clib/exec_protos.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
	#include <clib/dos_protos.h>

	__AROS_LH1(BPTR, DupLock,

/*  SYNOPSIS */
	__AROS_LA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 16, Dos)

/*  FUNCTION
	Clone a lock on a file or directory. This will only work on shared
	locks.

    INPUTS
	lock - Old lock.

    RESULT
	The new lock or 0 in case of an error. IoErr() will give additional
	information in that case.

    NOTES
	This function is identical to DupLockFromFH().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/

/*****************************************************************************

    NAME
	#include <clib/dos_protos.h>

	__AROS_LH1(BPTR, DupLockFromFH,

    SYNOPSIS
	__AROS_LA(BPTR, fh, D1),

    LOCATION
	struct DosLibrary *, DOSBase, 62, Dos)

    FUNCTION
	Try to get a lock on the object selected by the filehandle.

    INPUTS
	fh - filehandle.

    RESULT
	The new lock or 0 in case of an error. IoErr() will give additional
	information in that case.

    NOTES
	This function is identical to DupLock().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
/*AROS alias DupLockFromFH DupLock */
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct DosLibrary *,DOSBase)

    BPTR old, new;
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Use Lock() to clone the handle. cd to it first. */
    old=me->pr_CurrentDir;
    me->pr_CurrentDir=lock;
    new=Lock("",SHARED_LOCK);
    me->pr_CurrentDir=old;
    return new;
    __AROS_FUNC_EXIT
} /* DupLock */
