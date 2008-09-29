/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: (Obsolete) Open a library.
    Lang: english
*/
#include "exec_intern.h"
#include <aros/libcall.h>
#include <exec/libraries.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */

	AROS_LH1(struct Library *, OldOpenLibrary,

/*  SYNOPSIS */
	AROS_LHA(UBYTE *, libName, A1),

/*  LOCATION */
	struct ExecBase *, SysBase, 68, Exec)

/*  FUNCTION
	This is the same function as OpenLibrary(), only that it uses 0 as
	version number. This function is obsolete. Don't use it.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	OpenLibrary()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    return OpenLibrary(libName,0);
    AROS_LIBFUNC_EXIT
} /* OldOpenLibrary */
