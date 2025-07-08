/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdarg.h>

        int swprintf (

/*  SYNOPSIS */
        wchar_t * restrict str,
        size_t   maxsize,
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Format a variable list of arguments according to a wide-character
        format string and store the result in the specified buffer.

    INPUTS
        s      - Destination buffer for the formatted wide-character string.
        maxsize - Maximum number of wide characters to store in the buffer,
                  including the null terminator.
        format - A wide-character printf() format string.
        ...    - Additional arguments matching the format specifiers.

    RESULT
        The number of wide characters written to the buffer, excluding the
        terminating null character. A negative value is returned if an error
        occurs.

    NOTES
        The result is always null-terminated unless maxsize is zero.

    EXAMPLE

    BUGS

    SEE ALSO
        vswprintf(), fwprintf(), vfwprintf(), wprintf(), vwprintf()

    INTERNALS

******************************************************************************/
{
    va_list args;
    va_start(args, format);
    int result = vswprintf(str, maxsize, format, args);
    va_end(args);
    return result;
}
