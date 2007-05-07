/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: dos.library function DupLock()
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
	The new lock or NULL in case of an error. IoErr() will give additional
	information in that case.

    NOTES
	This function is identical to DupLockFromFH().

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

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

*****************************************************************************/
/*AROS alias DupLockFromFH DupLock */
{
    AROS_LIBFUNC_INIT

    BPTR old, new;

    /* Use Lock() to clone the handle. cd to it first. */
    old = CurrentDir(lock);
    new=Lock("",SHARED_LOCK);
    CurrentDir(old);
    return new;
    AROS_LIBFUNC_EXIT
} /* DupLock */
