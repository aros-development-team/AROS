/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function asin
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double asin(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the principal value of the arc sine of x.

    INPUTS
        x - a double value in the domain [-1.0, 1.0]

    RESULT
        The arc sine of x, in radians, in the range [-p/2, p/2].

    NOTES
        Returns NaN if |x| > 1 and sets errno to EDOM.
        Accurate for inputs close to domain boundaries.

    EXAMPLE
        double y = asin(0.0);   // y = 0.0
        double z = asin(1.0);   // z = p/2 ˜ 1.57079632679

    BUGS
        None known beyond domain checks.

    SEE ALSO
        asinf(), acos(), atan()

    INTERNALS
        Calls __ieee754_asin(x).
        Performs domain checking: if |x| > 1, returns NaN with errno EDOM.
        Uses polynomial approximations with argument reduction.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(asin, x);
    if (x < -1.0 || x > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_asin(x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(asinl), asinl, AROS_CSYM_FROM_ASM_NAME(asinl), AROS_CSYM_FROM_ASM_NAME(asin));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(asinl));
#endif
