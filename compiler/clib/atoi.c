/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function atoi()
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
	The value of string str.

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
        atof(), atol(), strtod(), strtol(), strtoul()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    return strtol (str, (char **)NULL, 10);
} /* atoi */

