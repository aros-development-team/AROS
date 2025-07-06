/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cosh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double cosh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the hyperbolic cosine of x.

    INPUTS
        x - a double value.

    RESULT
        The hyperbolic cosine of x, defined as (e^x + e^(-x)) / 2.

    NOTES
        cosh(x) = 1 for all real x.
        Returns NaN if input is NaN.
        May overflow for large |x|, resulting in +Inf.

    EXAMPLE
        double y = cosh(0.0);     // y = 1.0
        double z = cosh(1.0);     // z ˜ 1.5430806348

    BUGS
        Overflow for large inputs.

    SEE ALSO
        coshf(), sinh(), tanh()

    INTERNALS
        Calls __ieee754_cosh(x).
        Uses exponential function calls to compute cosh.
        Handles large inputs by scaling to avoid overflow when possible.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(cosh, x);
    double y = __ieee754_cosh(x);
    if (!isfinite(y)) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
    }
    return y;
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(coshl), coshl, AROS_CSYM_FROM_ASM_NAME(coshl), AROS_CSYM_FROM_ASM_NAME(cosh));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(coshl));
#endif
