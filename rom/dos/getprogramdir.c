/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetProgramDir() - Get the lock for PROGDIR:
    Lang: english
*/
#include "dos_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH0(BPTR, GetProgramDir,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct DosLibrary *, DOSBase, 100, Dos)

/*  FUNCTION
	This function will return the shared lock on the directory that
	the current process was loaded from. You can use this to help
	you find data files which were supplied with your program.

	A NULL return is possible, which means that you may be running
	from the Resident list.

	You should NOT under any circumstance UnLock() this lock.

    INPUTS

    RESULT
	A shared lock on the directory the program was started from.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    return ((struct Process *)FindTask(NULL))->pr_HomeDir;

    AROS_LIBFUNC_EXIT
} /* GetProgramDir */
