/*
    (C) 1995 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  1999/05/10 19:10:21  hkiel
    Always return the number of (complete) blocks read.

    Revision 1.3  1998/10/20 16:44:38  hkiel
    Amiga Research OS

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
	The number of blocks read from the file or 0 on EOF.
	This function may return less than the requested number of blocks
	IoErr() gives additional information in case of an error.

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
		goto finish;

	    *ptr ++ = c;
	}
    }
finish:
    if( read==0 && len==blocklen )
    {
        return EOF;
    }
    return read;
    AROS_LIBFUNC_EXIT
} /* FRead */
