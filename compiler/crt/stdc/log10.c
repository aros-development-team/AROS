/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function log10
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double log10(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the base-10 logarithm of x.

    INPUTS
        x - input value (must be positive).

    RESULT
        Returns log10(x).
        Returns NaN and sets errno to EDOM if x = 0.

    NOTES
        Uses IEEE 754 semantics.

    EXAMPLE
        double val = log10(100.0);  // returns 2.0

    BUGS
        None known.

    SEE ALSO
        log(), log1p(), exp10()

    INTERNALS
        Forwards to __ieee754_log10() with domain and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(log10, x);
    if (x < 0.0) {
        STDC_SETMATHERRNO(EDOM)
        STDC_RAISEMATHEXCPT(FE_INVALID)
        return NAN;
    }
    if (x == 0.0) {
        STDC_SETMATHERRNO(ERANGE)
        STDC_RAISEMATHEXCPT(FE_DIVBYZERO)
        return -INFINITY;
    }
    return __ieee754_log10(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(log10l), log10l, AROS_CSYM_FROM_ASM_NAME(log10l), AROS_CSYM_FROM_ASM_NAME(log10));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(log10l));
#endif
