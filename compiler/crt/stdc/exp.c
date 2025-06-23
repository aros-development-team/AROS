/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function exp
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double exp(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes e raised to the power x.

    INPUTS
        x - exponent.

    RESULT
        Returns e^x.

        Sets errno to ERANGE and raises FE_OVERFLOW on overflow.

    NOTES
        Uses IEEE 754 semantics.

    EXAMPLE
        double val = exp(1.0);  // returns e (~2.71828)

    BUGS
        None known.

    SEE ALSO
        expm1(), log(), expf(), expl()

    INTERNALS
        Forwards to __ieee754_exp() with error handling.

******************************************************************************/
{
    double y = __ieee754_exp(x);
    if (!isfinite(y)) {
        errno = ERANGE;
        feraiseexcept(FE_OVERFLOW);
    }
    return y;
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(expl), expl, AROS_CSYM_FROM_ASM_NAME(expl), AROS_CSYM_FROM_ASM_NAME(exp));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(expl));
#endif
