/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function abs()
    Lang: english
*/
#include <ctype.h>
#include <stdio.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int atoi (

/*  SYNOPSIS */
	const char * str)

/*  FUNCTION
	Convert a string of digits into an integer.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'.

    RESULT
	The absolute value of j.

    NOTES

    EXAMPLE
	// returns 1
	atoi ("  \t +1");

	// returns 1
	atoi ("1");

	// returns -1
	atoi ("  \n -1");

    BUGS

    SEE ALSO
	labs(), fabs()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    return strtol (str, (char **)NULL, 10);
} /* atoi */

