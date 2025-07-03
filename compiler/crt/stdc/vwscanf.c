/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function vwscanf().
*/

#include <libraries/stdcio.h>
#include <wchar.h>
#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>

        int vwscanf (

/*  SYNOPSIS */
        const wchar_t * restrict format,
        va_list      arg)

/*  FUNCTION
        Reads formatted wide-character input from the standard input stream
        and stores the results into the locations described by the argument list.

    INPUTS
        format - How to convert the input into the arguments
        arg    - List of argument pointers to receive results

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        vfwscanf(), vswscanf()

    INTERNALS
        calls stdcio.library->vfwscanf().

******************************************************************************/
{
    struct StdCIOBase *StdCIOBase = __aros_getbase_StdCIOBase();

    return vfwscanf(StdCIOBase->_stdin, format, arg);
}
