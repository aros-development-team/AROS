/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, SetVBuf,

/*  SYNOPSIS */
	AROS_LHA(BPTR  , fh, D1),
	AROS_LHA(STRPTR, buff, D2),
	AROS_LHA(LONG  , type, D3),
	AROS_LHA(LONG  , size, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 61, Dos)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

#warning TODO: Write dos/SetVBuf()
    aros_print_not_implemented ("SetVBuf");

    return EOF;
    AROS_LIBFUNC_EXIT
} /* SetVBuf */
