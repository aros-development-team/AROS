/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.
*/

#include <libraries/stdcio.h>

#include <stdarg.h>
#include <wchar.h>

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>
#include <stdarg.h>

        int vwprintf (

/*  SYNOPSIS */
        const wchar_t * restrict format,
        va_list         args)

/*  FUNCTION
        Format a va_list of arguments according to a wide-character
        format string and print the output to the standard output stream.

    INPUTS
        format - A wide-character printf() format string.
        arg    - A va_list containing the arguments for the format string.

    RESULT
        The number of wide characters written to the standard output, or a
        negative value if an error occurs.

    NOTES
        Equivalent to vfwprintf() with stdout as the output stream.

    EXAMPLE

    BUGS

    SEE ALSO
        wprintf(), fwprintf(), vfwprintf(), swprintf(), vswprintf()

    INTERNALS

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return vfwprintf(StdCIOBase->_stdout, format, args);
}
