/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/exec.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(BPTR, CurrentDir,

/*  SYNOPSIS */
	AROS_LHA(BPTR, lock, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 21, Dos)

/*  FUNCTION
	Sets a new directory as the current directory. Returns the old one.
	0 is valid in both cases and represents the boot filesystem.

    INPUTS
	lock - Lock for the new current directory.

    RESULT
	Old current directory.

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

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);
    BPTR old;

    /* Nothing spectacular */
    old=me->pr_CurrentDir;
    me->pr_CurrentDir=lock;
    return old;
    AROS_LIBFUNC_EXIT
} /* CurrentDir */
