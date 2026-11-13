/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: C99 function tgamma
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double tgamma(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the gamma function G(x).

    INPUTS
        x - input value.

    RESULT
        Returns the gamma function value.
        Domain errors set errno to EDOM.

    NOTES
        Extends the factorial function to real numbers.

    EXAMPLE
        double val = tgamma(5.0);  // returns 24.0 (4!)

    BUGS
        None known.

    SEE ALSO
        lgamma(), erf()

    INTERNALS
        Forwards to __ieee754_tgamma().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(tgamma, x);
    double r = __ieee754_tgamma(x);
    if (isnan(r)) {
        /* Domain error: a negative integer argument (Annex F.10.5.4). */
        errno = EDOM;
        feraiseexcept(FE_INVALID);
    } else if (isinf(r) && isfinite(x)) {
        if (x == 0.0) {
            /* Pole error at zero. */
            errno = ERANGE;
            feraiseexcept(FE_DIVBYZERO);
        } else {
            /* Range error: overflow for large x. */
            errno = ERANGE;
            feraiseexcept(FE_OVERFLOW);
        }
    }
    return r;
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(tgammal), tgammal, AROS_CSYM_FROM_ASM_NAME(tgammal), AROS_CSYM_FROM_ASM_NAME(tgamma));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(tgammal));
#endif
