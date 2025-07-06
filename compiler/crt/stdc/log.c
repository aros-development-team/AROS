/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function log
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double log(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the natural logarithm (base e) of x.

    INPUTS
        x - input value (must be positive).

    RESULT
        Returns ln(x).
        Returns NaN and sets errno to EDOM if x = 0.

    NOTES
        Uses IEEE 754 semantics for special values.

    EXAMPLE
        double val = log(2.718281828);  // approximately 1.0

    BUGS
        None known.

    SEE ALSO
        log10(), exp()

    INTERNALS
        Forwards to __ieee754_log() with domain and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(log, x);
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
    return __ieee754_log(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(logl), logl, AROS_CSYM_FROM_ASM_NAME(logl), AROS_CSYM_FROM_ASM_NAME(log));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(logl));
#endif
