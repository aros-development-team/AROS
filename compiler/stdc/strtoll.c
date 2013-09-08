/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strtoll().
*/

/* This requires the C99 type long long. */

#include <aros/system.h>
#if defined(AROS_HAVE_LONG_LONG)

#include <ctype.h>
#include <errno.h>
#ifndef AROS_NO_LIMITS_H
#	include <limits.h>
#else
#	define LLONG_MAX	0x7fffffffffffffffLL
#	define LLONG_MIN	(-0x7fffffffffffffffLL - 1)
#endif

/*****************************************************************************

    NAME */
#include <stdlib.h>

	long long strtoll (

/*  SYNOPSIS */
	const char * restrict	str,
	char	  ** restrict	endptr,
	int			base)

/*  FUNCTION
	Convert a string of digits into an integer according to the
	given base.

    INPUTS
	str - The string which should be converted. Leading
		whitespace are ignored. The number may be prefixed
		by a '+' or '-'. If base is above 10, then the
		alphabetic characters from 'A' are used to specify
		digits above 9 (ie. 'A' or 'a' is 10, 'B' or 'b' is
		11 and so on until 'Z' or 'z' is 35).
	endptr - If this is non-NULL, then the address of the first
		character after the number in the string is stored
		here.
	base - The base for the number. May be 0 or between 2 and 36,
		including both. 0 means to autodetect the base. strtoul()
		selects the base by inspecting the first characters
		of the string. If they are "0x", then base 16 is
		assumed. If they are "0", then base 8 is assumed. Any
		other digit will assume base 10. This is like in C.

		If you give base 16, then an optional "0x" may
		precede the number in the string.

    RESULT
	The value of the string. The first character after the number
	is returned in *endptr, if endptr is non-NULL. If no digits can
	be converted, *endptr contains str (if non-NULL) and 0 is
	returned.

    NOTES

    EXAMPLE
	// returns 1, ptr points to the 0-Byte
	strtoll ("  \t +0x1", &ptr, 0);

	// Returns 15. ptr points to the a
	strtoll ("017a", &ptr, 0);

	// Returns 215 (5*36 + 35)
	strtoll ("5z", &ptr, 36);

    BUGS

    SEE ALSO
        strtoull()

    INTERNALS

******************************************************************************/
{
    long long	val	= 0;
    char	* ptr;
    char	* copy;

    while (isspace (*str))
	str ++;

    copy = (char *)str;

    if (*str)
    {
	val = strtoull (str, &ptr, base);

	if (endptr)
	{
	    if (ptr == str)
		str = copy;
	    else
		str = ptr;
	}

	/* Remember: strtoull() has already done the sign conversion */
	if (*copy == '-')
	{
	    if ((signed long long)val > 0)
	    {
#ifndef AROSC_ROM
		errno = ERANGE;
#endif
		val = LLONG_MIN;
	    }
	}
	else
	{
	    if ((signed long long)val < 0)
	    {
#ifndef AROSC_ROM
		errno = ERANGE;
#endif
		val = LLONG_MAX;
	    }
	}
    }

    if (endptr)
	*endptr = (char *)str;

    return val;
} /* strtoll */

#endif /* AROS_HAVE_LONG_LONG */
