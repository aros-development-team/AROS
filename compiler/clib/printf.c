/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of printf()
    Lang: english
*/
#include <stdio.h>
#include <stdarg.h>

int printf(const char * format, ...)
{
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfprintf (stdout, format, args);

    va_end (args);

    fflush (stdout);

    return retval;
} /* printf */

