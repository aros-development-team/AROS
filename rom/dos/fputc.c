/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <exec/memory.h>
#include <proto/exec.h>

#include <dos/stdio.h>
#include <dos/dosextens.h>

#include "dos_intern.h"

#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH2(LONG, FPutC,

/*  SYNOPSIS */
	AROS_LHA(BPTR, file,      D1),
	AROS_LHA(LONG, character, D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 52, Dos)

/*  FUNCTION

    INPUTS
	file	  - Filehandle to write to.
	character - Character to write.

    RESULT
	The character written or EOF in case of an error.
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

    return (1 == FWriteChars(file, &character, 1))
        ? character
        : EOF;

    AROS_LIBFUNC_EXIT
} /* FPutC */
