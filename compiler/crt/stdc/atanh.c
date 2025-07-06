/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atanh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double atanh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the inverse hyperbolic tangent of x.

    INPUTS
        x - a double value.

    RESULT
        The inverse hyperbolic tangent of x.

    NOTES
        Domain is (-1, 1); outside this range returns NaN.
        Returns ±Inf if x is ±1.
        Returns NaN if input is NaN.
        The function satisfies atanh(x) = 0.5 * ln((1+x)/(1-x)).

    EXAMPLE
        double y = atanh(0.5);   // y ˜ 0.5493061443

    BUGS
        May lose precision near domain boundaries.

    SEE ALSO
        atanhf(), tanh()

    INTERNALS
        Calls __ieee754_atanh(x).
        Performs domain checks.
        Uses log and division operations to compute value.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(atanh, x);

    double absx = fabs(x);
    if (absx > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    } else if (absx == 1.0) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
        return copysign(INFINITY, x);
    }

    return __ieee754_atanh(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(atanhl), atanhl, AROS_CSYM_FROM_ASM_NAME(atanhl), AROS_CSYM_FROM_ASM_NAME(atanh));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(atanhl));
#endif
