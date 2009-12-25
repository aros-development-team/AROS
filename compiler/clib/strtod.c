/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function strtod().
*/

#ifndef AROS_NOFPU

#include <ctype.h>
#include <errno.h>
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
#warning TODO: implement NAN handling
    double        val = 0, precision;
    int           exp = 0;
    char          c = 0, c2 = 0;

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
	    val = val * 10 + (*str - '0');
	    str ++;
	}

        /* see if there is the dot */
	if(*str == '.')
	{
            str++;
	    /* scan the numbers behind the dot */
            precision = 0.1;
	    while (isdigit (*str))
	    {
		val += ((*str - '0') * precision) ;
		str ++;
		precision = precision * 0.1;
	    }
	}

        /* look for a sequence like "E+10" or "e-22" */
	if(tolower(*str) == 'e')
	{
            str++;
        
	    if (*str == '+' || *str == '-')
	        c2 = *str ++;

	    while (isdigit (*str))
	    {
		exp = exp * 10 + (*str - '0');
		str ++;
	    }
	    if (c2 == '-')
	        exp = -exp;
	    val *= pow (10, exp);
	}

	if (c == '-')
	    val = -val;
    }

    if (endptr)
	*endptr = (char *)str;

    return val;
} /* strtod */

#else

void strtod (const char * str,char ** endptr)
{
	return;
}

#endif /* AROS_NOFPU */
