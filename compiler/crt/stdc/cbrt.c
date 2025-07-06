/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function cbrt
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double cbrt(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the cube root of x.

    INPUTS
        x - a double value (any real number)

    RESULT
        The cube root of x.

    NOTES
        Defined for all real numbers including negative values.
        cbrt(-8.0) returns -2.0.
        Returns NaN if input is NaN.

    EXAMPLE
        double y = cbrt(27.0);    // y = 3.0
        double z = cbrt(-8.0);    // z = -2.0

    BUGS
        None known.

    SEE ALSO
        cbrtf(), pow()

    INTERNALS
        Calls __ieee754_cbrt(x).
        Uses iterative methods to approximate cube root.
        Handles sign and zero carefully to maintain precision.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(cbrt, x);
    double r = __ieee754_cbrt(x);
    // usually no error
    return r;
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(cbrtl), cbrtl, AROS_CSYM_FROM_ASM_NAME(cbrtl), AROS_CSYM_FROM_ASM_NAME(cbrt));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cbrtl));
#endif
