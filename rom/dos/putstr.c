/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Lang: english
*/
#include <proto/exec.h>
#include <dos/dosextens.h>

#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH1(LONG, PutStr,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR, string, D1),

/*  LOCATION */

	struct DosLibrary *, DOSBase, 158, Dos)

/*  FUNCTION
        This routine writes an unformatted string to the default output.  No 
        newline is appended to the string and any error is returned.  This
        routine is buffered.

    INPUTS
        str   - Null-terminated string to be written to default output

    RESULT
        error - 0 for success, -1 for any error.

    SEE ALSO
        FGetC(), IoErr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    BPTR file=((struct Process *)FindTask(NULL))->pr_COS;

    return(FPuts(file, string));
    
    AROS_LIBFUNC_EXIT
} /* PutStr */
