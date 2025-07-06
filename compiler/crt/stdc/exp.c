/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function exp
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double exp(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the exponential function of x, e raised to the power x.

    INPUTS
        x - a double value.

    RESULT
        The value of e^x.

    NOTES
        Returns +Inf if x is large and positive (overflow).
        Returns 0 if x is large and negative (underflow).
        Returns NaN if input is NaN.

    EXAMPLE
        double y = exp(0.0);     // y = 1.0
        double z = exp(1.0);     // z ˜ 2.718281828

    BUGS
        Precision may degrade for very large magnitude inputs.

    SEE ALSO
        expf(), expm1()

    INTERNALS
        Calls __ieee754_exp(x).
        Uses range reduction and polynomial approximation.
        Carefully handles overflow and underflow conditions.

******************************************************************************/
{
    double y = __ieee754_exp(x);
    if (!isfinite(y)) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
    }
    return y;
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(expl), expl, AROS_CSYM_FROM_ASM_NAME(expl), AROS_CSYM_FROM_ASM_NAME(exp));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(expl));
#endif
