/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <dos/dos.h>

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <proto/dos.h>

	AROS_LH1(BPTR, ParentDir,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 35, Dos)

/*  FUNCTION
	Returns a lock to the parent directory of the supplied lock.

    INPUTS
	lock - Lock to get parent directory of.

    RESULT
	Returns a lock to the parent directory or NULL, in which case the 
	supplied lock has no parent directory (because it is the root 
	directory) or an error occured. IoErr() returns 0 in the former case 
	and a different value on error.

    NOTES

    EXAMPLE

    BUGS
	Locks get via ParentDir() are currently never unlocked! Use this 
	function with care.

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BPTR oldlock = CurrentDir(lock), newlock;

    newlock = Lock("/", ACCESS_READ);
    CurrentDir(oldlock);

    return newlock;
    AROS_LIBFUNC_EXIT
} /* ParentDir */
