/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, FWrite,

/*  SYNOPSIS */
	AROS_LHA(BPTR , fh, D1),
	AROS_LHA(CONST APTR , block, D2),
	AROS_LHA(ULONG, blocklen, D3),
	AROS_LHA(ULONG, number, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 55, Dos)

/*  FUNCTION
	Write a number of blocks to a file.

    INPUTS
	fh - Write to this file
	block - The data begins here
	blocklen - This is the size of a single block
	number - The number of blocks

    RESULT
	The number of blocks written to the file or EOF on error. IoErr()
	gives additional information in case of an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	Open(), FRead(), FPutc(), Close()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)

    ULONG   written;
    ULONG   len;
    UBYTE  *ptr;

    ptr = block;
    len = 0;

    SetIoErr(0);

    for(written = 0; written < number; written++)
    {
	for(len = blocklen; len--; )
	{
	    if(FPutC(fh, *ptr++) < 0)
		return EOF;
	}
    }
    
    return written;

    AROS_LIBFUNC_EXIT
} /* FWrite */
