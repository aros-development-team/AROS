/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.7  1997/01/27 00:36:17  ldp
    Polish

    Revision 1.6  1996/12/09 13:53:24  aros
    Added empty templates for all missing functions

    Moved #include's into first column

    Revision 1.5  1996/10/24 15:50:26  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.4  1996/08/13 13:52:54  digulla
    Replaced <dos/dosextens.h> by "dos_intern.h" or added "dos_intern.h"
    Replaced AROS_LA by AROS_LHA

    Revision 1.3  1996/08/12 14:20:37  digulla
    Added aliases

    Revision 1.2  1996/08/01 17:40:49  digulla
    Added standard header for all files

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, DupLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

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

	AROS_LH1(BPTR, DupLockFromFH,

    SYNOPSIS
	AROS_LHA(BPTR, fh, D1),

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    BPTR old, new;
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Use Lock() to clone the handle. cd to it first. */
    old=me->pr_CurrentDir;
    me->pr_CurrentDir=lock;
    new=Lock("",SHARED_LOCK);
    me->pr_CurrentDir=old;
    return new;
    AROS_LIBFUNC_EXIT
} /* DupLock */
