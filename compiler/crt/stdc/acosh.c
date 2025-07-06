/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function acosh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>
/*****************************************************************************

    NAME */
#include <math.h>

        double acosh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the inverse hyperbolic cosine of x.

    INPUTS
        x - a double value where x >= 1.0

    RESULT
        The inverse hyperbolic cosine of x.

    NOTES
        acosh(x) is defined only for x = 1.
        Returns NaN and sets errno to EDOM if x < 1.
        For large x, acosh(x) ˜ ln(2x).

    EXAMPLE
        double y = acosh(1.0);   // y = 0.0
        double z = acosh(2.0);   // z ˜ 1.3169578969

    BUGS
        None known beyond domain restrictions.

    SEE ALSO
        acoshf(), asinh(), cosh()

    INTERNALS
        Calls __ieee754_acosh(x).
        Performs domain checking: if x < 1, returns NaN with errno EDOM.
        Uses logarithmic identity: acosh(x) = ln(x + sqrt(x^2 -1)).

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(acosh, x);
    if (x < 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_acosh(x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(acoshl), acoshl, AROS_CSYM_FROM_ASM_NAME(acoshl), AROS_CSYM_FROM_ASM_NAME(acosh));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(acoshl));
#endif
