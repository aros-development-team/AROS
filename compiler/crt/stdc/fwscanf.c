/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function fwscanf().
*/

#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>

        int fwscanf (

/*  SYNOPSIS */
        FILE       * restrict stream,
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Reads formatted wide-character input from the given file stream
        and stores the results into the provided argument locations.

    INPUTS
        stream - Read from this wide-character file stream
        format - How to convert the input into the arguments
        ...    - Write the result in these arguments

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wscanf(), swscanf()

    INTERNALS
        calls vfwscanf().

******************************************************************************/
{
    int result;
    va_list args;

    va_start(args, format);

    result = vfwscanf(stream, format, args);

    va_end(args);

    return result;
}
