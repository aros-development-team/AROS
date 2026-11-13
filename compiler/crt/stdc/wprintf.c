/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <stdarg.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>

        int wprintf (

/*  SYNOPSIS */
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Format a variable list of arguments according to a wide-character
        format string and print the output to the standard output stream.

    INPUTS
        format - A wide-character printf() format string.
        ...    - Additional arguments matching the format specifiers.

    RESULT
        The number of wide characters written to the standard output, or a
        negative value if an error occurs.

    NOTES
        Equivalent to fwprintf() with stdout as the output stream.

    EXAMPLE

    BUGS

    SEE ALSO
        vwprintf(), fwprintf(), vfwprintf(), swprintf(), vswprintf()

    INTERNALS

******************************************************************************/
{
    va_list args;
    va_start(args, format);
    int result = vwprintf(format, args);
    va_end(args);
    return result;
}
