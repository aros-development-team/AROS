/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Calculate square root
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <math.h>

	double sqrt (

/*  SYNOPSIS */
	double val)

/*  FUNCTION
	Compute the square root of val.

    INPUTS
	val - Must be >= 0.0.

    RESULT
	The square root of val.

    NOTES

    EXAMPLE
	double s;
	double val;

	// calculate
	s = sqrt (val);

	if (s*s == val)
	    printf ("%g is the square root of %g\n", s, val);
	else
	    printf ("Error: sqrt(%g) returned %g\n", val, s);

    BUGS

    SEE ALSO
	pow()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    double a, a_;
    int n;

    a=0.0;
    a_=val/2.0;

    for (n=0;n<16 && a!=a_;n++)
    {
	a=a_;
	a_=(a+val/a)/2;
    }

    return a;
} /* sqrt */

