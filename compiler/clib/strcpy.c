/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function strcpy()
    Lang: english
*/
#include <string.h>

char * strcpy (char * dest, const char * src)
{
    char * ptr = dest;

    while ((*ptr = *src))
    {
	ptr ++;
	src ++;
    }

    return dest;
}
