/*
    Copyright © 2015, The AROS Development Team. All rights reserved.
    $Id:$

    C99 function strtold().
*/

#ifndef AROS_NOFPU

#include <ctype.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>
#include <math.h>

	long double strtold (

/*  SYNOPSIS */
	const char * str,
	char      ** endptr)

/*  FUNCTION
	Convert a string of digits into a long double.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'. An 'e' or 'E' introduces the exponent.
		Komma is only allowed before exponent.
	endptr - If this is non-NULL, then the address of the first
		character after the number in the string is stored
		here.

    RESULT
	The value of the string. The first character after the number
	is returned in *endptr, if endptr is non-NULL. If no digits can
	be converted, *endptr contains str (if non-NULL) and 0 is
	returned.

    NOTES
	We make the compiler do an internal conversion from a double
	to a long double. Because of this we lose a some precision, but for 
	now it works.

    EXAMPLE

    BUGS
        NAN is not handled at the moment

    SEE ALSO
        strtod()

    INTERNALS

******************************************************************************/
{
    return (long double)strtod(str, endptr);
} /* strtold */

#else

void strtold (const char * str,char ** endptr)
{
    return;
}

#endif /* AROS_NOFPU */
