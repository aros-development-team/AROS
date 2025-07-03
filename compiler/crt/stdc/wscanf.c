/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wscanf().
*/

#include <libraries/stdcio.h>
#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <wchar.h>

        int wscanf (

/*  SYNOPSIS */
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Reads formatted wide-character input from the standard input stream
        and stores the results into the provided argument locations.

    INPUTS
        format - How to convert the input into the arguments
        ...    - Write the result in these arguments

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        scanf(), fwscanf()

    INTERNALS
        calls stdcio.library->vfwscanf().

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();
    va_list args;
    int ret;

    va_start(args, format);
    ret = vfwscanf(StdCIOBase->_stdin, format, args);
    va_end(args);

    return ret;
}
