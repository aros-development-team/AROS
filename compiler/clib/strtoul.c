/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI C function strtoul()
    Lang: english
*/

#include <ctype.h>
#include <errno.h>
#ifndef AROS_NO_LIMITS_H
#	include <limits.h>
#else
#	define ULONG_MAX	4294967295UL
#endif

/*****************************************************************************

    NAME */
#include <stdlib.h>

	unsigned long strtoul (

/*  SYNOPSIS */
	const char * str,
	char      ** endptr,
	int          base)

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
	// Returns 1, ptr points to the 0-Byte
	strtoul ("  \t +0x1", &ptr, 0);

	// Returns 15. ptr points to the a
	strtoul ("017a", &ptr, 0);

	// Returns 215 (5*36 + 35)
	strtoul ("5z", &ptr, 36);

    BUGS

    SEE ALSO
        atoi(), atol(), strtod(), strtol(), strtoul()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    GETUSER;

    unsigned long val   = 0;
    int           digit;
    char          c = 0;

    if (base < 0 || base == 1 || base > 36)
    {
	errno = EINVAL;

	if (endptr)
	    *endptr = (char *)str;

	return 0;
    }

    while (isspace (*str))
	str ++;

    if (*str)
    {
	if (*str == '+' || *str == '-')
	    c = *str ++;

	/* Assume base ? */
	if (!base)
	{
	    if (*str == '0') /* Base 8 or 16 */
	    {
		if (tolower(str[1]) == 'x')
		    base = 16;
		else
		    base = 8;
	    }
	    else /* Any other digit: Base 10 (decimal) */
		base = 10;
	}

	while (*str)
	{
	    if (isalnum (*str))
	    {
		digit = tolower (*str) - '0';

		if (digit > 9)
		    digit -= 'a' - '0' - 10;

		if (digit >= base)
		    break;

		if (val > ((ULONG_MAX - digit) / base)
		    || (val * base) > (ULONG_MAX - digit)
		)
		{
		    errno = ERANGE;
		    val = ULONG_MAX;
		    break;
		}

		val = (val * base) + digit;
	    }
	    else
		break;

	    str ++;
	}

	if (c == '-')
	    val = -val;
    }

    if (endptr)
	*endptr = (char *)str;

    return val;
} /* strtoul */

