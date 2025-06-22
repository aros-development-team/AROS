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
        x - input value.

    RESULT
        Returns (e^x + e^(-x)) / 2.

    NOTES
        Result is always >= 1 for real x.

    EXAMPLE
        double ch = cosh(0.0);  // returns 1.0

    BUGS
        None known.

    SEE ALSO
        sinh(), tanh(), coshf(), coshl()

    INTERNALS
        Forwards to __ieee754_cosh().

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
