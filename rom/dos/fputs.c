/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <dos/dosextens.h>
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, FPuts,

/*  SYNOPSIS */
	AROS_LHA(BPTR,         file,   D1),
	AROS_LHA(CONST_STRPTR, string, D2),

/*  LOCATION */

	struct DosLibrary *, DOSBase, 57, Dos)

/*  FUNCTION

    INPUTS
	file   - Filehandle to write to.
	string - String to write.

    RESULT
	0 if all went well or EOF in case of an error.
	IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FGetC(), IoErr()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    while(*string)
	if(FPutC(file,*string++)<0)
	    return EOF;

    return 0;

    AROS_LIBFUNC_EXIT
} /* FPuts */
