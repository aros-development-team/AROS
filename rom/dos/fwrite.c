/*
    (C) 1995 AROS - The Amiga Research OS
    $Id$
    $Log$
    Revision 1.4  2000/11/23 19:59:37  SDuvan
    Added SetIoErr(0) before operation

    Revision 1.3  1998/10/20 16:44:38  hkiel
    Amiga Research OS

    Revision 1.2  1997/01/27 00:36:21  ldp
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

	AROS_LH4(LONG, FWrite,

/*  SYNOPSIS */
	AROS_LHA(BPTR , fh, D1),
	AROS_LHA(APTR , block, D2),
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

    HISTORY
	29-10-95    digulla automatically created from
			    dos_lib.fd and clib/dos_protos.h

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
