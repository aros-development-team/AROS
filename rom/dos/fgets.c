/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "dos_intern.h"
#include <aros/debug.h>

/*****************************************************************************

    NAME */
#include <proto/dos.h>

        AROS_LH3(STRPTR, FGets,

/*  SYNOPSIS */
        AROS_LHA(BPTR  , fh, D1),
        AROS_LHA(STRPTR, buf, D2),
        AROS_LHA(ULONG , buflen, D3),

/*  LOCATION */
        struct DosLibrary *, DOSBase, 56, Dos)

/*  FUNCTION
        Read until NEWLINE (\n), EOF is encountered or buflen-1
        characters have been read. If a NEWLINE is read, it will
        be the last character in the buffer. The buffer will always
        be \0-terminated.

    INPUTS
        fh - Read buffered from this filehandle
        buf - Put read chars in this buffer
        buflen - The size of the buffer

    RESULT
        buf or NULL if the first thing read is EOF.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
 
    ULONG len = 0;
    LONG  c;

    buflen--;

    do
    {
        c = FGetC (fh);

        if (c == EOF)
        {
            if (len == 0)
                return NULL;
            else
                break;
        }

        buf[len++] = c;
    }
    while ((len<buflen) && (c != '\n'));

    buf[len] = 0;

    return buf;
    
    AROS_LIBFUNC_EXIT
} /* FGets */
