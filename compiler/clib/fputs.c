/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of fputs()
    Lang: english
*/
#include <stdio.h>
#include <stdarg.h>

int fputs(const char * str, FILE * fh)
{
    while (*str)
    {
	if (putc (*str, fh) == EOF)
	    return EOF;

	str ++;
    }

    return 1;
} /* fputs */

