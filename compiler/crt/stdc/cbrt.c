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
        x - input value.

    RESULT
        Returns the cube root of x.
        Handles negative inputs properly.

    NOTES
        Returns NaN if x is NaN.

    EXAMPLE
        double root = cbrt(27.0);  // returns 3.0

    BUGS
        None known.

    SEE ALSO
        sqrt(), pow(), cbrtf(), cbrtl()

    INTERNALS
        Forwards to __ieee754_cbrt().

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
