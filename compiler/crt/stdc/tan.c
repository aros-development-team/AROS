/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function tan
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double tan(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the tangent of x (x in radians).

    INPUTS
        x - input angle in radians.

    RESULT
        Returns tan(x).

    NOTES
        Uses range reduction and polynomial approximation internally.
        Returns NaN and sets errno to EDOM if x is an odd multiple of p/2.

    EXAMPLE
        double val = tan(3.14159 / 4);  // approximately 1.0

    BUGS
        None known.

    SEE ALSO
        sin(), cos(), tanh()

    INTERNALS
        Forwards to __ieee754_tan() with domain and error handling.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(tan, x);
    return __ieee754_tan(x);
}

#if (LDBL_MANT_DIG == DBL_MANT_DIG)
AROS_MAKE_ASM_SYM(typeof(tanl), tanl, AROS_CSYM_FROM_ASM_NAME(tanl), AROS_CSYM_FROM_ASM_NAME(tan));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(tanl));
#endif
