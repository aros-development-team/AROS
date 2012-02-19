/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function atof().
*/

#ifndef AROS_NOFPU

/*****************************************************************************

    NAME */
#include <stdlib.h>

	double atof (

/*  SYNOPSIS */
	const char * str)

/*  FUNCTION
	Convert a string of digits into a double.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'.

    RESULT
	The value of string str.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        atoi(), atol(), strtod(), strtol(), strtoul()

    INTERNALS

******************************************************************************/
{
    return strtod (str, (char **)NULL);
} /* atof */

#else

void atof(const char * str)
{
	return;
}

#endif /* AROS_NOFPU */
