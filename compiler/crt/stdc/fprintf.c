/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    C99 function fprintf().
*/

#include <aros/debug.h>

#include <stdarg.h>

#include "debug.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int fprintf (

/*  SYNOPSIS */
        FILE       * restrict fh,
        const char * restrict format,
        ...)

/*  FUNCTION
        Format a string with the specified arguments and write it to
        the stream.

    INPUTS
        fh - Write to this stream
        format - How to format the arguments
        ... - The additional arguments

    RESULT
        The number of characters written to the stream or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    int     retval;
    va_list args;

    D(bug("[%s] %s(0x%p, 0x%p)\n", STDCNAME, __func__, fh, format));

    va_start (args, format);

    retval = vfprintf (fh, format, args);

    va_end (args);

    D(bug("[%s] %s: returning %x\n", STDCNAME, __func__, retval));

    return retval;
} /* fprintf */
