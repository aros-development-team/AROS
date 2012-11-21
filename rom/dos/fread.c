/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

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
        This function may return fewer than the requested number of blocks.
        IoErr() gives additional information in case of an error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        Open(), FWrite(), FPutc(), Close()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG   read;
    ULONG   len;
    UBYTE  *ptr;
    LONG    c;

    ptr = block;
    len = 0;

    SetIoErr(0);

    for(read = 0; read < number; read++)
    {
        for(len = blocklen; len--; )
        {
            c = FGetC(fh);

            if(c < 0)
                goto finish;

            *ptr ++ = c;
        }
    }
finish:
    if(read == 0 && len == blocklen)
    {
        return EOF;
    }

    return read;

    AROS_LIBFUNC_EXIT
} /* FRead */
