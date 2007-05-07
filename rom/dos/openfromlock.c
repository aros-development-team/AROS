/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open a file from a lock
    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, OpenFromLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 63, Dos)

/*  FUNCTION
	Convert a lock into a filehandle. If all went well the lock
	will be gone. In case of an error it must still be freed.

    INPUTS
	lock - Lock to convert.

    RESULT
	New filehandle or 0 in case of an error. IoErr() will give
	additional information in that case.

    NOTES
	Since locks and filehandles in AROS are identical this function
	is just the (useless) identity operator and thus can never fail.
	It's there for compatibility to Amiga OS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Warning: Some very tricky operation ahead ;-). */
    return lock;
    AROS_LIBFUNC_EXIT
} /* OpenFromLock */
