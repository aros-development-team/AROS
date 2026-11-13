/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function sin
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double sin(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the sine of x (x in radians).

    INPUTS
        x - input angle in radians.

    RESULT
        Returns sin(x).

    NOTES
        Uses range reduction and polynomial approximation internally.

    EXAMPLE
        double val = sin(3.14159 / 2);  // approximately 1.0

    BUGS
        None known.

    SEE ALSO
        cos(), tan(), sinf(), sinl()

    INTERNALS
        Forwards to __ieee754_sin().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(sin, x);
    return __ieee754_sin(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(sinl), sinl, AROS_CSYM_FROM_ASM_NAME(sinl), AROS_CSYM_FROM_ASM_NAME(sin));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(sinl));
#endif
