/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.

    GNU function vasprintf().
*/
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
#include <stdlib.h>

        int vasprintf (

/*  SYNOPSIS */
        char **restrict str, const char *restrict format, va_list args)

/*  FUNCTION
        Analog of vsprintf, except that sotrage is allocated for a string
        large enough to hold the output including the terminating null
        byte

    INPUTS
        str - Where to store the pointer for the allocated string.
        format - A printf() format string.
        args - A list of arguments for the format string.

    RESULT
        The number of characters written, or EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        asprintf(), sprintf(), vsprintf()

    INTERNALS

******************************************************************************/
{
    va_list argtmp;
    int size = 0;


    va_copy(argtmp, args);
    size = vsnprintf(NULL, 0, format, argtmp);
    va_end(argtmp);

    if (size < 0)
    {
        return -1;
    }

    *str = (char *) malloc(size + 1);
    if (!*str)
    {
        return -1;
    }

    size = vsprintf(*str, format, args);

    return size;
}
