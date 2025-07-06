/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function wscanf().
*/

#define DEBUG 1
#include <aros/debug.h>

#include <libraries/stdcio.h>
#include <stdarg.h>

#include "debug.h"

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
    struct StdCIOBase *StdCIOBase;
    va_list args;
    int ret;

    D(bug("[%s] %s(0x%p)\n", STDCNAME, __func__, format));

    StdCIOBase = __aros_getbase_StdCIOBase();

    D(bug("[%s] %s: StdCIOBase @ 0x%p\n", STDCNAME, __func__, StdCIOBase));

    va_start(args, format);

    ret = vfwscanf(StdCIOBase->_stdin, format, args);

    va_end(args);

    return ret;
}
