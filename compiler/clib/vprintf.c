/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of printf()
    Lang: english
*/
#include <stdio.h>
#include <stdarg.h>

int vprintf(const char * format, va_list args)
{
    return vfprintf (stdout, format, args);
} /* vprintf */

