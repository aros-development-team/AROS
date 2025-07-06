/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function swscanf().
*/

#define DEBUG 1
#include <aros/debug.h>

#include <wchar.h>

#include "debug.h"

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>

        int swscanf (

/*  SYNOPSIS */
        const wchar_t * restrict ws,
        const wchar_t * restrict format,
        ...)

/*  FUNCTION
        Reads formatted wide-character input from the given wide-character string
        and stores the results into the provided argument locations.

    INPUTS
        ws     - Read from this wide-character string
        format - How to convert the input into the arguments
        ...    - Write the result in these arguments

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        wscanf(), fwscanf()

    INTERNALS
        calls vswscanf().

******************************************************************************/
{
    va_list args;
    int ret;

    D(bug("[%s] %s(0x%p, 0x%p)\n", STDCNAME, __func__, ws, format));

    va_start(args, format);

    ret = vswscanf(ws, format, args);

    va_end(args);

    return ret;
}
