
/* @(#)e_gamma_r.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_gamma_r.c,v 1.7 2005/02/04 18:26:05 das Exp $";
#endif

/* __ieee754_gamma_r(x, signgamp)
 * Reentrant version of the logarithm of the Gamma function
 * with user provide pointer for the sign of Gamma(x).
 *
 * Method: See __ieee754_lgamma_r
 */

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        double gamma_r(

    SYNOPSIS
        double x, int *signgamp)

    FUNCTION
        Return the natural logarithm of the absolute value of
        Gamma(x), and store its sign separately.

    INPUTS
        x - Double argument.
        signgamp - Non-NULL pointer to writable int storage. The
                   function writes this object for every argument,
                   including poles and nonfinite values.

    RESULT
        An approximation to log(|Gamma(x)|) in double. For finite
        nonpole arguments, *signgamp is +1 or -1 according to the
        sign of Gamma(x), subject to BUGS.

        Returns zero for x equal to 1 or 2. Both signs of zero and
        negative integers are poles and return positive infinity.
        Either infinity returns positive infinity; NaN returns NaN.

        At negative zero, *signgamp is -1. At positive zero, negative
        integer poles, either infinity and NaN, it is +1. These
        special-case values do not indicate a valid finite Gamma
        value or successful evaluation.

    NOTES
        Include <math.h>. This is the historical name for lgamma_r().
        It has the same result and sign output.

        Use tgamma() when you need Gamma(x) itself.
        The result can be negative even when *signgamp is positive.

        The signgam variable is not used as implicit output. Passing
        &signgam explicitly still writes that object. Use separate
        output storage for independent concurrent calls.

        The function does not assign errno. Pole arithmetic can
        raise divide-by-zero; other arithmetic can raise overflow
        or inexact. Enabled traps may prevent return.

    BUGS
        Large negative noninteger arguments can produce an incorrect
        Gamma sign. For example, x equal to
        -1125899906842625.5 can report -1 although Gamma(x) is positive.

    SEE ALSO
        lgamma_r(), tgamma(), gammaf_r()

*****************************************************************************/
double
__ieee754_gamma_r(double x, int *signgamp)
{
	return __ieee754_lgamma_r(x,signgamp);
}
