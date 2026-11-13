/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdarg.h>

        int vfwprintf (

/*  SYNOPSIS */
        struct __sFILE * restrict stream,
        const wchar_t * restrict format,
        va_list         arg)

/*  FUNCTION
        Format a va_list of arguments according to a wide-character
        format string and write the output to the given stream.

    INPUTS
        stream - Output stream.
        format - A wide-character printf() format string.
        arg    - A va_list containing the arguments for the format string.

    RESULT
        The number of wide characters written to the stream, or a negative
        value if an error occurs.

    NOTES
        This is the va_list version of fwprintf(), similar to vprintf().

    EXAMPLE

    BUGS

    SEE ALSO
        fwprintf(), wprintf(), vwprintf(), swprintf(), vswprintf()

    INTERNALS
        Calls __vwformat() with a file-backed putwc() handler.

******************************************************************************/
{
    return __vwformat(stream, (unsigned int (*)(unsigned int,  void *))fputwc, format, arg);
}
