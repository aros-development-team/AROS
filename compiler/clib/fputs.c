/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function fputs()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

	int fputs (

/*  SYNOPSIS */
	const char * str,
	FILE	   * fh)

/*  FUNCTION
	Write a string to the specified stream.

    INPUTS
	str - Output this string...
	fh - ...to this stream

    RESULT
	> 0 on success and EOF on error.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	puts(), fputc(), putc()

    INTERNALS

    HISTORY
	10.12.1996 digulla created

******************************************************************************/
{
    while (*str)
    {
		if (putc (*str, fh) == EOF)
	    	return EOF;

		str ++;
    }

    return 1;
} /* fputs */

