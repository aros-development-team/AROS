/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function log2
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double log2(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the base-2 logarithm of x.

    INPUTS
        x - input value (must be positive).

    RESULT
        Returns log2(x).
        Returns NaN and sets errno to EDOM if x = 0.

    NOTES
        Uses IEEE 754 semantics.

    EXAMPLE
        double val = log2(8.0);  // returns 3.0

    BUGS
        None known.

    SEE ALSO
        log(), log10()

    INTERNALS
        Forwards to __ieee754_log2() with domain and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(log2, x);
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
    return __ieee754_log2(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(log2l), log2l, AROS_CSYM_FROM_ASM_NAME(log2l), AROS_CSYM_FROM_ASM_NAME(log2));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(log2l));
#endif
