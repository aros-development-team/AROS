/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function strlen()
    Lang: english
*/
#include <string.h>
#include <exec/types.h>

size_t strlen (const char * ptr)
{
    const char * start = ptr;

    while (*ptr) ptr ++;

    return (IPTR)ptr - (IPTR)start;
}

