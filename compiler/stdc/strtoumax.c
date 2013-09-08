/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strtoumax().
*/

#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <inttypes.h>

	uintmax_t strtoumax (

/*  SYNOPSIS */
	const char * nptr,
	char      ** endptr,
	int	     base)

/*  FUNCTION
	Convert a string of digits into an integer according to the
	given base. This function is like strtoul() except the fact,
	that it returns a value of type uintmax_t.

    INPUTS
	str - The string which should be converted.
	endptr - If this is non-NULL, then the address of the first
		character after the number in the string is stored
		here.
	base - The base for the number.

    RESULT
	The value of the string. The first character after the number
	is returned in *endptr, if endptr is non-NULL. If no digits can
	be converted, *endptr contains str (if non-NULL) and 0 is
	returned.

    NOTES

    EXAMPLE

    BUGS
        errno is not set as required by C99 standard

    SEE ALSO
        strtoul(), strtoull()

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement errno handling in strtoumax() */
#if defined(AROS_HAVE_LONG_LONG)
    return (uintmax_t) strtoull(nptr, endptr, base);
#else
    return (uintmax_t) strtoul(nptr, endptr, base);
#endif
}
