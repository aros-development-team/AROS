/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1997/01/27 00:36:20  ldp
    Polish

    Revision 1.1  1996/11/25 15:50:57  aros
    Two new functions

    Desc:
    Lang: english
*/
#include "dos_intern.h"

/*****************************************************************************

    NAME */
#include <proto/dos.h>

	AROS_LH4(LONG, FRead,

/*  SYNOPSIS */
	AROS_LHA(BPTR , fh, D1),
	AROS_LHA(APTR , block, D2),
	AROS_LHA(ULONG, blocklen, D3),
	AROS_LHA(ULONG, number, D4),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 54, Dos)

/*  FUNCTION
	Read a number of blocks from a file.

    INPUTS
	fh - Read from this file
	block - The data is put here
	blocklen - This is the size of a single block
	number - The number of blocks

    RESULT
	The number of blocks written to the file or EOF on error. IoErr()
	gives additional information in case of an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	Open(), FWrite(), FPutc(), Close()

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct DosLibrary *,DOSBase)
    ULONG   read;
    ULONG   len;
    UBYTE * ptr;
    LONG    c;

    ptr = block;
    len = 0;

    for (read=0; read<number; read++)
    {
	for (len=blocklen; len--; )
	{
	    c = FGetC (fh);

	    if (c < 0)
		return EOF;

	    *ptr ++ = c;
	}
    }

    return read;
    AROS_LIBFUNC_EXIT
} /* FRead */
