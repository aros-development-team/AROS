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
        double x, double y)

/*  FUNCTION
        Computes the floating-point remainder of x/y.

    INPUTS
        x - the dividend.
        y - the divisor.

    RESULT
        Returns the value x - n*y, where n is the quotient of x/y truncated
        toward zero. The result has the same sign as x and magnitude less
        than the magnitude of y.

        If y is zero or x is infinite, returns NaN and sets errno to EDOM.

    NOTES
        Differentiates from remainder() by truncation toward zero instead
        of rounding to nearest.

    EXAMPLE
        double r = fmod(5.3, 2.0);  // returns 1.3

    BUGS
        None known.

    SEE ALSO
        remainder(), remquo(), div(), fmodf(), fmodl()

    INTERNALS
        Forwards to __ieee754_fmod() with domain checks and error handling.

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

#if (LDBL_MANT_DIG == 53)
AROS_MAKE_ASM_SYM(typeof(fmodl), sqrtl, AROS_CSYM_FROM_ASM_NAME(fmodl), AROS_CSYM_FROM_ASM_NAME(fmod));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(fmodl));
#endif
