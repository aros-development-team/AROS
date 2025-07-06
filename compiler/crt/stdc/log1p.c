/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function log1p
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double log1p(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the natural logarithm of (1 + x), i.e. ln(1 + x), with high
        accuracy even for values of x near zero.

    INPUTS
        x - input value.

    RESULT
        Returns ln(1 + x).

        If x == -1.0, returns -INFINITY and sets errno to ERANGE.
        If x < -1.0, returns NaN and sets errno to EDOM.

    NOTES
        This function is more accurate than using log(1 + x) directly for
        small x, due to reduced cancellation error.

        The result is well-defined for all x > -1. If x < -1, the result is
        undefined (logarithm of a negative number).

    EXAMPLE
        double val = log1p(1e-9);   // accurate result close to 1e-9
        double val2 = log1p(-1.0);  // returns -INFINITY, sets errno = ERANGE

    BUGS
        None known.

    SEE ALSO
        log(), expm1(), log10(), log2()

    INTERNALS
        Forwards to __ieee754_log1p(), with domain checks and proper error
        handling for x = -1. NaN and Inf are forwarded automatically.

******************************************************************************/
 {
    FORWARD_IF_NAN_OR_INF(log1p, x);

    if (x == -1.0) {
        errno = ERANGE;
        feraiseexcept(FE_DIVBYZERO);
        return -INFINITY;
    }

    if (x < -1.0) {
        errno = EDOM;
        feraiseexcept(FE_INVALID);
        return NAN;
    }

    return __ieee754_log1p(x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(log1pl), log1pl, AROS_CSYM_FROM_ASM_NAME(log1pl), AROS_CSYM_FROM_ASM_NAME(log1p));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(log1pl));
#endif
