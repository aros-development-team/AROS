/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>

#include "dos_intern.h"

#include <aros/debug.h>


/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, FPuts,
/*		FPuts -- Writes a string the the specified output (buffered) */

/*  SYNOPSIS */
	AROS_LHA(BPTR,         file,   D1),
	AROS_LHA(CONST_STRPTR, string, D2),

/*  LOCATION */

	struct DosLibrary *, DOSBase, 57, Dos)

/*  FUNCTION
    This routine writes an unformatted string to the filehandle.  No
    newline is appended to the string.  This routine is buffered.

    INPUTS
	file   - Filehandle to write to.
	string - String to write.

    RESULT
	0 if all went well or EOF in case of an error.
	IoErr() gives additional information in that case.

    SEE ALSO
	FGetC(), IoErr()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    ASSERT_VALID_PTR(file);
    ASSERT_VALID_PTR(string);

        ULONG
    len = strlen(string);

    return(FWriteChars(file, string, len) == len
        ? 0
        : -1);

    AROS_LIBFUNC_EXIT
} /* FPuts */
