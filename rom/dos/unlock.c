/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/exec.h>
#include <dos/dosextens.h>
#include <dos/filesystem.h>
#include <proto/dos.h>
#include "dos_intern.h"
#include <aros/debug.h>

/*****i***********************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BOOL, UnLock,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 15, Dos)

/*
    FUNCTION
	Free a lock created with Lock().

    INPUTS
	lock -- The lock to free

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return Close(lock);

    AROS_LIBFUNC_EXIT
} /* UnLock */
