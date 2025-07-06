/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function atan
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double atan(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the principal value of the arc tangent of x.

    INPUTS
        x - a double value (any real number)

    RESULT
        The arc tangent of x, in radians, in the range [-p/2, p/2].

    NOTES
        atan(x) is defined for all real numbers.
        Returns NaN if input is NaN.

    EXAMPLE
        double y = atan(0.0);    // y = 0.0
        double z = atan(1.0);    // z = p/4 ˜ 0.78539816339

    BUGS
        None known.

    SEE ALSO
        atanf(), atan2(), tan()

    INTERNALS
        Calls __ieee754_atan(x).
        Uses polynomial approximations with argument reduction.
        Handles special cases for large and small inputs.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(atan, x);
    // entire real line is allowed
    return __ieee754_atan(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(atanl), atanl, AROS_CSYM_FROM_ASM_NAME(atanl), AROS_CSYM_FROM_ASM_NAME(atan));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(atanl));
#endif
