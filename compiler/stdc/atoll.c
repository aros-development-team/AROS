/*
    Copyright © 1995-2018, The AROS Development Team. All rights reserved.
    $Id$

    C99 function atoll().
*/

#include <aros/system.h>
#if defined(AROS_HAVE_LONG_LONG)
/*****************************************************************************

    NAME */
#include <stdlib.h>

	long long atoll (

/*  SYNOPSIS */
	const char * str)

/*  FUNCTION
	Convert a string of digits into an long long integer.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'.

    RESULT
	The value of string str.

    NOTES

    EXAMPLE
	// returns 1
	atoll ("  \t +1");

	// returns 1
	atoll ("1");

	// returns -1
	atoll ("  \n -1");

    BUGS

    SEE ALSO
        atof(), atoi(), atol(), strtod(), strtol(), strtoul()

    INTERNALS

******************************************************************************/
{
    return strtoll(str, (char **)NULL, 10);
} /* atoll */

#endif /* AROS_HAVE_LONG_LONG */
