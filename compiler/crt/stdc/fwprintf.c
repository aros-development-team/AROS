/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdarg.h>

        int fwprintf (

/*  SYNOPSIS */
        struct __sFILE * restrict stream,
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Format a variable list of arguments according to a wide-character
        format string and write the output to the given stream.

    INPUTS
        stream - Output stream.
        format - A wide-character printf() format string.
        ...    - Additional arguments matching the format specifiers.

    RESULT
        The number of wide characters written to the stream, or a negative
        value if an error occurs.

    NOTES
        This function uses wide-character output and formatting, similar to
        printf(), but targeting wchar_t output streams.

    EXAMPLE

    BUGS

    SEE ALSO
        vfwprintf(), wprintf(), vwprintf(), swprintf(), vswprintf()

    INTERNALS

******************************************************************************/
{
    va_list args;
    va_start(args, format);
    int result = vfwprintf(stream, format, args);
    va_end(args);
    return result;
}
