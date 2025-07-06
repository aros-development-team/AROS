/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    C99 function vfwscanf().
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/dos.h>

#include "__stdio.h"

#include "debug.h"

/*****************************************************************************

    NAME */
#include <wchar.h>
#include <stdio.h>

        int vfwscanf (

/*  SYNOPSIS */
        FILE       * restrict stream,
        const wchar_t * restrict format,
        va_list      arg)

/*  FUNCTION
        Reads formatted wide-character input from the given file stream,
        parsing according to the given format string, and stores the results
        into the locations described by the argument list.

    INPUTS
        stream - Read from this wide-character file stream
        format - How to convert the input into the arguments
        arg    - List of argument pointers to receive results

    RESULT
        The number of converted arguments.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        vwscanf(), vswscanf()

    INTERNALS
        Calls __vwscanf() using fgetwc() and ungetwc() for character
        input and push-back.

******************************************************************************/
{
    D(bug("[%s] %s(0x%p, 0x%p)\n", STDCNAME, __func__, stream, format));

    Flush (stream->fh);

    return __vwscanf(stream, (wint_t (*)(void *))fgetwc, (int (*)(wint_t,  void *))ungetwc, format, arg);
}
