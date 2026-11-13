/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function vwscanf().
*/

#define DEBUG 1
#include <aros/debug.h>

#include <libraries/stdcio.h>
#include <stdarg.h>

#include "debug.h"

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
    struct StdCIOBase *StdCIOBase;

    D(bug("[%s] %s(0x%p, 0x%p)\n", STDCNAME, __func__, format, arg));

    StdCIOBase = __aros_getbase_StdCIOBase();

    D(bug("[%s] %s: StdCIOBase @ 0x%p\n", STDCNAME, __func__, StdCIOBase));

    return vfwscanf(StdCIOBase->_stdin, format, arg);
}
