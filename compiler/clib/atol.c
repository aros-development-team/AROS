/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function atol().
*/

#include <limits.h>
#include <ctype.h>

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

******************************************************************************/
{
    unsigned long   val   = 0;
    int             digit;
    char            c = 0;
    unsigned long   cutoff;
    int             cutlim;
    int		    any;

    while (isspace (*str))
        str ++;

    if (*str)
    {
        if (*str == '+' || *str == '-')
            c = *str ++; 

        /*
            Conversion loop, from FreeBSD's src/lib/libc/stdlib/strtoul.c

            The previous AROS loop was
            a) inefficient - it did a division each time around.
            b) buggy - it returned the wrong value in endptr on overflow.
        */
        cutoff = (unsigned long)ULONG_MAX / (unsigned long)10;
        cutlim = (unsigned long)ULONG_MAX % (unsigned long)10;
        val = 0;
        any = 0;

        while (*str)
        {
            digit = *str;

            if (!isdigit(digit))
                break;

            digit -= '0';

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
                val = (val * 10) + digit;
            }

            str++;
        }

        /* Range overflow */
        if (any < 0)
        {
            val = ULONG_MAX;
        }

        if (c == '-')
            val = -val;
    }

    return val;
} /* atol */
