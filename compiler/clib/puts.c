/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Implementation of puts()
    Lang: english
*/
#include <stdio.h>

int puts(const char * str, FILE * fh)
{
    if (fputs (str, fh) == EOF)
	return EOF;

    if (putc ('\n', fh) == EOF)
	return EOF;

    if (fflush (stdout) == EOF)
	return EOF;

    return 1;
} /* puts */

