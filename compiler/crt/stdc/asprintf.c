/*
    Copyright (C) 2023-2025, The AROS Development Team. All rights reserved.

    GNU extension asprintf().
*/


#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <libraries/stdcio.h>

#include <aros/debug.h>
#if DEBUG
#include <aros/libcall.h>
#endif

#include "debug.h"

/*****************************************************************************

    NAME */
#include <stdio.h>
#include <stdarg.h>

        int asprintf (

/*  SYNOPSIS */
        char **restrict str, const char *restrict format, ...)

/*  FUNCTION
        Analog of sprintf, except that sotrage is allocated for a string
        large enough to hold the output including the terminating null
        byte

    INPUTS
        str - Where to store the pointer for the allocated string.
        format - A printf() format string.
        ... - Arguments for the format string

    RESULT
        The number of characters written, or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        vasprintf(), sprintf(), vsprintf()

    INTERNALS

******************************************************************************/
{
    va_list args;
    int size = 0;

    va_start(args, format);
    size = vasprintf(str, format, args);
    va_end(args);

    return size;
}
