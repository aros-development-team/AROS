/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    C99 function strtoull().
*/

/* This function requires the use of the long long type. */
#include <aros/system.h>
#if defined(AROS_HAVE_LONG_LONG)

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#ifndef AROS_NO_LIMITS_H
#	include <limits.h>
#else
#	define ULLONG_MAX	0xffffffffffffffffULL
#endif

/*****************************************************************************

    NAME */
#include <stdlib.h>

	unsigned long long strtoull (

/*  SYNOPSIS */
	const char * restrict	str,
	char      ** restrict	endptr,
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
		including both. 0 means to autodetect the base. strtoull()
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
	strtoull ("  \t +0x1", &ptr, 0);

	// Returns 15. ptr points to the a
	strtoull ("017a", &ptr, 0);

	// Returns 215 (5*36 + 35)
	strtoull ("5z", &ptr, 36);

    BUGS

    SEE ALSO
        strtoll(), strtoull()

    INTERNALS

******************************************************************************/
{
    unsigned long long	val = 0;
    int			digit;
    char		c = 0;
    unsigned long long	cutoff;
    int			cutlim;
    int			any;

    if (base < 0 || base == 1 || base > 36)
    {
#ifndef STDC_STATIC
        errno = EINVAL;
#endif
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
        if (base == 0 || base == 16)
        {
            if (*str == '0') /* Base 8 or 16 */
            {
                str++;
                if (*str == 'x' || *str == 'X')
                {
                    base = 16;
                    str++;
                }
                else if(base == 0)
                    base = 8;
            }
            else if(base == 0) /* Any other digit: Base 10 (decimal) */
                base = 10;
        }

        /*
            Conversion loop, from FreeBSD's src/lib/libc/stdlib/strtoull.c

            The previous AROS loop was
            a) inefficient - it did a division each time around.
            b) buggy - it returned the wrong value in endptr on overflow.
        */
        cutoff = (unsigned long long)ULLONG_MAX / (unsigned long long)base;
        cutlim = (unsigned long long)ULLONG_MAX % (unsigned long long)base;
        val = 0;
        any = 0;

        while (*str)
        {
            digit = *str;

            if (!isascii(digit))
                break;

            if (isdigit(digit))
                digit -= '0';
            else if (isalpha(digit))
                digit -= isupper(digit) ? 'A' - 10 : 'a' - 10;
            else
                break;

            if (digit >= base)
                break;

            /*
                any < 0 when we have overflowed. We still need to find the
                end of the subject sequence
            */
            if (any < 0 || val > cutoff || (val == cutoff && digit > cutlim))
            {
                any = -1;
            }
            else
            {
                any = 1;
                val = (val * base) + digit;
            }

            str++;
        }

        /* Range overflow */
        if (any < 0)
        {
            val = ULLONG_MAX;
#ifndef STDC_STATIC
            errno = ERANGE;
#endif
        }

        if (c == '-')
            val = -val;
    }

    if (endptr) 
        *endptr = (char *)str;

    return val;
} /* strtoull */

#endif /* AROS_HAVE_LONG_LONG */
