/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function atol()
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long atol (

/*  SYNOPSIS */
	const char * str)

/*  FUNCTION
	Convert a string of digits into an long integer.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'.

    RESULT
	The value of string str.

    NOTES

    EXAMPLE
	// returns 1
	atol ("  \t +1");

	// returns 1
	atol ("1");

	// returns -1
	atol ("  \n -1");

    BUGS

    SEE ALSO
        atof(), atoi(), strtod(), strtol(), strtoul()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    return strtol (str, (char **)NULL, 10);
} /* atol */
