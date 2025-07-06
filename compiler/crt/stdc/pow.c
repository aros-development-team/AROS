/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function pow
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double pow(

/*  SYNOPSIS */
        double x, double y)

/*  FUNCTION
        Computes x raised to the power of y (x^y).

    INPUTS
        x - the base.
        y - the exponent.

    RESULT
        Returns x raised to y. If the result overflows, sets errno to ERANGE.
        If x < 0 and y is not an integer, result is NaN and errno is set to EDOM.

    NOTES
        Domain and range errors are handled per IEEE 754.

    EXAMPLE
        double r = pow(2.0, 3.0);  // returns 8.0

    BUGS
        Some edge cases may differ slightly from hardware FPU behavior.

    SEE ALSO
        exp(), log(), powf(), powl()

    INTERNALS
        Forwards to __ieee754_pow() and checks domain/overflow.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF2(pow, x, y);

    // Invalid domain: negative base with non-integer exponent
    if (x < 0.0 && floor(y) != y) {
        STDC_SETMATHERRNO(EDOM)
        STDC_RAISEMATHEXCPT(FE_INVALID)
        return NAN;
    }

    double r = __ieee754_pow(x, y);
    if (!isfinite(r)) {
        STDC_SETMATHERRNO(ERANGE)
        STDC_RAISEMATHEXCPT(FE_OVERFLOW)
    }
    return r;
}

#if	(LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(powl), powl, AROS_CSYM_FROM_ASM_NAME(powl), AROS_CSYM_FROM_ASM_NAME(pow));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(powl));
#endif
