/*
    (C) 1995-98 AROS - The Amiga Replacement OS
    $Id$

    Desc: ANSI C function strtod()
    Lang: english
*/
#include <ctype.h>
#include <errno.h>
#include <limits.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

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

    SEE ALSO
        atof(), atoi(), atol(), strtol(), strtoul()

    INTERNALS

    HISTORY
	12.06.1996 hkiel created

******************************************************************************/
{
    double        val = 0;
    int           tail, exp = 0;
    char          c = 0, ce = 0;

    while (isspace (*str))
	str ++;

    if (*str)
    {
	if (*str == '+' || *str == '-')
	    c = *str ++;

	while (isdigit(*str))
	{
	    val = val * 10 + (*str - '0');
	    str ++;
	}

	if(*str == '.')
	{
	    tail = -1;
	    while (isdigit (*str))
	    {
		val += (*str - '0') * pow (10, tail);
		str ++;
		tail --;
	    }
	}

	if(tolower(*str) == 'e')
	{
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

