/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cos
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double cos(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the cosine of x (x in radians).

    INPUTS
        x - a double value representing an angle in radians.

    RESULT
        The cosine of the angle x.

    NOTES
        The function is periodic with period 2p.
        Returns NaN if input is NaN.
        Uses argument reduction to improve precision for large inputs.

    EXAMPLE
        double y = cos(0.0);          // y = 1.0
        double z = cos(M_PI / 2.0);   // z ˜ 0.0

    BUGS
        Slight loss of precision for very large inputs.

    SEE ALSO
        cosf(), sin(), tan()

    INTERNALS
        Calls __ieee754_cos(x).
        Performs argument reduction modulo 2p.
        Uses polynomial approximations (Taylor or Chebyshev series).

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(cos, x);
    return __ieee754_cos(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(cosl), cosl, AROS_CSYM_FROM_ASM_NAME(cosl), AROS_CSYM_FROM_ASM_NAME(cos));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cosl));
#endif
