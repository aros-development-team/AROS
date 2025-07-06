/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function acos
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double acos(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the principal value of the arc cosine of x.

    INPUTS
        x - a double value in the domain [-1.0, 1.0]

    RESULT
        The arc cosine of x, in radians, in the range [0, p].

    NOTES
        acos(x) returns NaN if x is outside the domain [-1,1].

    EXAMPLE
        double y = acos(0.0);   // y = p/2 ˜ 1.57079632679
        double z = acos(1.0);   // z = 0.0

    BUGS
        None known beyond domain restrictions.

    SEE ALSO
        acosf(), asin(), atan()

    INTERNALS
        Calls __ieee754_acos(x).
        Performs domain checking: if |x| > 1, sets errno to EDOM and returns NaN.
        Uses polynomial/rational approximations with argument reduction for accuracy.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(acos, x);
    if (x < -1.0 || x > 1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }
    return __ieee754_acos(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(acosl), acosl, AROS_CSYM_FROM_ASM_NAME(acosl), AROS_CSYM_FROM_ASM_NAME(acos));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(acosl));
#endif
