/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strtod().
*/

#ifndef AROS_NOFPU

#include <ctype.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>
#include <math.h>

	double strtod (

/*  SYNOPSIS */
	const char * str,
	char      ** endptr)

/*  FUNCTION
	Convert a string of digits into a double.

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

    EXAMPLE

    BUGS
        NAN is not handled at the moment

    SEE ALSO
        atof(), atoi(), atol(), strtol(), strtoul()

    INTERNALS

******************************************************************************/
{
    /* Unit tests available in : tests/clib/strtod.c */
    /* FIXME: implement NAN handling */
    double  val = 0, precision;
    int     exp = 0;
    char    c = 0, c2 = 0;
    int     digits = 0;      

    /* assign initial value in case nothing will be found */
    if (endptr)
        *endptr = (char *)str;

    /* skip all leading spaces */
    while (isspace (*str))
        str ++;

    /* start with scanning the floting point number */
    if (*str)
    {
        /* Is there a sign? */
        if (*str == '+' || *str == '-')
            c = *str ++;

        /* scan numbers before the dot */
        while (isdigit(*str))
        {
            digits++;
            val = val * 10 + (*str - '0');
            str ++;
        }

        /* see if there is the dot and there were digits before it or there is 
            at least one digit after it */
        if ((*str == '.') && ((digits > 0) || (isdigit(*(str + 1)))))
        {
            str++;
            /* scan the numbers behind the dot */
            precision = 0.1;
            while (isdigit (*str))
            {
                digits++;
                val += ((*str - '0') * precision) ;
                str ++;
                precision = precision * 0.1;
            }
        }

        /* look for a sequence like "E+10" or "e-22" if there were any digits up to now */
        if ((digits > 0) && (tolower(*str) == 'e'))
        {
            int edigits = 0;
            str++;

            if (*str == '+' || *str == '-')
                c2 = *str ++;

            while (isdigit (*str))
            {
                edigits++;
                exp = exp * 10 + (*str - '0');
                str ++;
            }
            
            if (c2 == '-')
                exp = -exp;
            
            if (edigits == 0) 
            {
                /* there were no digits after 'e' - rollback pointer */
                str--; if (c2 != 0) str--;
            }
            
            val *= pow (10, exp);
        }

        if (c == '-')
            val = -val;

        if ((digits == 0) && (c != 0))
        {
            /* there were no digits but there was sign - rollback pointer */
            str--;
        }
    }

    /* something was found, assign the pointer value */
    if (endptr && digits > 0)
        *endptr = (char *)str;

    return val;
} /* strtod */

#else

void strtod (const char * str,char ** endptr)
{
    return;
}

#endif /* AROS_NOFPU */
