/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/debug.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <dos/stdio.h>
#include <dos/dosextens.h>

#include "dos_intern.h"


/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH1(LONG, FGetC,

/*  SYNOPSIS */
        AROS_LHA(BPTR, file, D1),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 51, Dos)

/*  FUNCTION
        Get a character from a buffered file. Buffered I/O is more efficient
        for small amounts of data but less for big chunks. You have to
        use Flush() between buffered and non-buffered I/O or you'll
        clutter your I/O stream.

    INPUTS
        file   - filehandle

    RESULT
        The character read or EOF if the file ended or an error happened.
        IoErr() gives additional information in that case.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        IoErr(), Flush()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    UBYTE c;
    LONG res;

    res = vbuf_fetch(file, &c, 1, DOSBase);

    if (res < 0)
        return EOF;
    else
        return c;

    AROS_LIBFUNC_EXIT
} /* FGetC */
