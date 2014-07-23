/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
        Write a character to a file handle. 

        The write is buffered.

        If the file handle is an interactive stream,
        the buffer is automatically flushed on a linefeed,
        carriage return or ASCII NUL.

    INPUTS
        file      - Filehandle to write to.
        character - Character to write.

    RESULT
        The character written or EOF in case of an error.
        IoErr() gives additional information in that case.

    NOTES
        You should use Flush() when switching between
        buffered and unbuffered IO.

    EXAMPLE

    BUGS

    SEE ALSO
        FGetC(), IoErr(), Flush(), FWrite()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    BYTE c = character;
    return (1 == FWriteChars(file, &c, 1, DOSBase))
        ? character
        : EOF;

    AROS_LIBFUNC_EXIT
} /* FPutC */
