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

	AROS_LH1(BPTR, SelectOutput,

/*  SYNOPSIS */
	AROS_LHA(BPTR, fh, D1),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 50, Dos)

/*  FUNCTION
	Sets the current output stream returned by Output() to a new
	value. Returns the old output stream.

    INPUTS
	fh - New output stream.

    RESULT
	Old output stream handle.

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

    BPTR old;

    /* Get pointer to process structure */
    struct Process *me=(struct Process *)FindTask(NULL);

    /* Nothing spectacular */
    old=me->pr_COS;
    me->pr_COS=fh;
    return old;
    AROS_LIBFUNC_EXIT
} /* SelectOutput */
