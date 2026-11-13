/* e_gammaf_r.c -- float version of e_gamma_r.c.
 * Conversion to float by Ian Lance Taylor, Cygnus Support, ian@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_gammaf_r.c,v 1.7 2002/05/28 18:15:03 alfred Exp $";
#endif

/* __ieee754_gammaf_r(x, signgamp)
 * Reentrant version of the logarithm of the Gamma function
 * with user provide pointer for the sign of Gamma(x).
 *
 * Method: See __ieee754_lgammaf_r
 */

#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME
        float gammaf_r(

    SYNOPSIS
        float x, int *signgamp)

    FUNCTION
        Return the natural logarithm of the absolute value of
        Gamma(x), and store its sign separately.

    INPUTS
        x - Float argument.
        signgamp - Non-NULL pointer to writable int storage. The
                   function writes this object for every argument,
                   including poles and nonfinite values.

    RESULT
        An approximation to log(|Gamma(x)|) in float. For finite
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
        Include <math.h>. This is the historical name for lgammaf_r().
        It has the same result and sign output.

        Use tgammaf() when you need Gamma(x) itself.
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
        -2097153.5f can report -1 although Gamma(x) is positive.

    SEE ALSO
        lgammaf_r(), tgammaf(), gamma_r()

*****************************************************************************/
float
__ieee754_gammaf_r(float x, int *signgamp)
{
	return __ieee754_lgammaf_r(x,signgamp);
}
