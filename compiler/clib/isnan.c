/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Check is a double is invalid.
    Lang: english
*/

/* @(#)s_isnan.c 1.3 95/01/18 */
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

	int isnan (

/*  SYNOPSIS */
	double val)

/*  FUNCTION
	Check if val is a valid double.

    INPUTS
	val - Check this.

    RESULT
	TRUE is val is not a number (NaN) or FALSE if it is
	a valid number.

    NOTES

    EXAMPLE
	// If this wouldn't crash, then it would print
	// "a doesn't contain a valid number".
	a = 1.0;
	a /= 0.0;

	if (isnan(a))
	    printf ("a doesn't contain a valid number\n");
	else
	    printf ("a contains a valid number\n");

    BUGS

    SEE ALSO
	abs(), fabs()

    INTERNALS

    HISTORY
	12.12.1996 digulla created

******************************************************************************/
{
    int hx,lx;

    hx = (__HI(val)&0x7fffffff);
    lx = __LO(val);
    hx |= (unsigned)(lx|(-lx))>>31;
    hx = 0x7ff00000 - hx;

    return ((unsigned)(hx))>>31;
} /* isnan */
