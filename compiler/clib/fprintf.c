/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of fprintf()
    Lang: english
*/
#include <stdio.h>
#include <stdarg.h>

int fprintf(FILE * fh, const char * format, ...)
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfprintf (fh, format, args);

    va_end (args);

    return retval;
} /* fprintf */

