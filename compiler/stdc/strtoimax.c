/*
    Copyright © 2008, The AROS Development Team. All rights reserved.
    $Id$

    POSIX function strtoimax().
*/

#include <stdlib.h>

/*****************************************************************************

    NAME */
#include <inttypes.h>

	intmax_t strtoimax (

/*  SYNOPSIS */
	const char * nptr,
	char      ** endptr,
	int	     base)

/*  FUNCTION
	Convert a string of digits into an integer according to the
	given base. This function is like strtol() except the fact,
	that it returns a value of type intmax_t.

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
        strtol(), strtoll()

    INTERNALS

******************************************************************************/
{
    /* TODO: Implement errno handling in strtoimax() */
#if defined(AROS_HAVE_LONG_LONG)
    return (intmax_t) strtoll(nptr, endptr, base);
#else
    return (intmax_t) strtol(nptr, endptr, base);
#endif    
}
