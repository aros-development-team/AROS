/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function fmod
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double fmod(

/*  SYNOPSIS */
        double x,
        double y)

/*  FUNCTION
        Computes the floating-point remainder of x / y.

    INPUTS
        x - numerator.
        y - denominator (non-zero).

    RESULT
        The remainder r = x - n*y, where n = trunc(x/y).

    NOTES
        Behavior undefined if y == 0.
        Result has same sign as x.

    EXAMPLE
        double r = fmod(5.3, 2.0);  // r = 1.3
        double s = fmod(-5.3, 2.0); // s = -1.3

    BUGS
        None known.

    SEE ALSO
        remainder(), fmodf(), fmodl()

    INTERNALS
        Calls __ieee754_fmod(x, y).
        Uses division and subtraction to compute remainder.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF2(fmod, x, y);

    // Handle special cases: y == 0 or x infinite ? domain error
    if (y == 0.0 || !isfinite(x)) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return __builtin_nan("");  // or (x - x) as NaN
    }

    double r = __ieee754_fmod(x, y);

    // fmod should never overflow, so no ERANGE check here

    return r;
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(fmodl), fmodl, AROS_CSYM_FROM_ASM_NAME(fmodl), AROS_CSYM_FROM_ASM_NAME(fmod));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(fmodl));
#endif
