/* e_lgammaf.c -- float version of e_lgamma.c.
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
static char rcsid[] = "$FreeBSD: src/lib/msun/src/e_lgammaf.c,v 1.8 2008/02/22 02:30:35 das Exp $";
#endif

/* __ieee754_lgammaf(x)
 * Return the logarithm of the Gamma function of x.
 *
 * Method: call __ieee754_lgammaf_r
 */

#include "math.h"
#include "math_private.h"

#include "__stdc_intbase.h"

/****************************************************************************
    NAME
        float lgammaf(

    SYNOPSIS
        float x)

    FUNCTION
        Return the natural logarithm of the absolute value of
        Gamma(x), and store its sign in signgam.

    INPUTS
        x - Float argument.

    RESULT
        An approximation to log(|Gamma(x)|) in float. For finite
        nonpole arguments, signgam is +1 or -1 according to the
        sign of Gamma(x), subject to BUGS.

        Returns zero for x equal to 1 or 2. Both signs of zero and
        negative integers are poles and return positive infinity.
        Either infinity returns positive infinity; NaN returns NaN.

        At negative zero, signgam is -1. At positive zero, negative
        integer poles, either infinity and NaN, it is +1. These
        values do not indicate successful evaluation.

    NOTES
        Include <math.h> to access signgam. On AROS it is a macro;
        do not replace the header with an extern int declaration.

        Read or copy signgam before another call that overwrites
        it. Use lgammaf_r() with your own output storage when the
        sign must remain independent of other calls.

        Use tgammaf() for Gamma(x) itself. The return value here can
        be negative even when signgam is positive.

        The function does not assign errno. Pole arithmetic can
        raise divide-by-zero; other arithmetic can raise overflow
        or inexact. Enabled traps may prevent return.

    BUGS
        Large negative noninteger arguments can produce an incorrect
        Gamma sign. For example, x equal to -2097153.5f can report
        -1 although Gamma(x) is positive.

    SEE ALSO
        lgammaf_r(), lgamma(), lgammal(), tgammaf()

*****************************************************************************/
float
__ieee754_lgammaf(float x)
{
	struct StdCBase *StdCBase = __aros_getbase_StdCBase();
	return __ieee754_lgammaf_r(x,&StdCBase->_signgam);
}
