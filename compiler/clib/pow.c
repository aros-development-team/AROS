/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: Compute a power to a number.
    Lang: english
*/
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
#include "__math.h"

/*****************************************************************************

    NAME */
#include <math.h>

	double pow (

/*  SYNOPSIS */
	double base,
	double exp)

/*  FUNCTION
	Compute the p'th power of b.

    INPUTS
	base - The base
	exp - The exponent

    RESULT
	The p'th power of b.

    NOTES

    EXAMPLE
	// returns 8
	pow (2,3);

    BUGS

    SEE ALSO
	sqrt()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    double result;

    result = __ieee754_pow (base, exp);

    if (isnan (exp))
	return result;

    if (isnan (base))
    {
	if (exp == 0.0)
	    return __kernel_standard (base, exp, 42); /* pow(NaN,0.0) */
	else
	    return result;
    }

    if (base == 0.0)
    {
	if (exp == 0.0)
	    return __kernel_standard (base, exp, 20); /* pow(0.0,0.0) */

	if (finite (exp) && exp < 0.0)
	    return __kernel_standard (base, exp, 23); /* pow(0.0,negative) */

	return result;
    }

    if (!finite (result))
    {
	if (finite (base) && finite (exp))
	{
	    if (isnan (result))
		return __kernel_standard (base, exp, 24); /* pow neg**non-int */
	    else
		return __kernel_standard (base, exp, 21); /* pow overflow */
	}
    }

    if (result == 0.0 && finite (base) && finite (exp))
	return __kernel_standard (base, exp, 22); /* pow underflow */

    return result;
} /* pow */

