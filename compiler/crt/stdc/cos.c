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
        x - angle in radians.

    RESULT
        Returns the cosine of x.

    NOTES
        Result is in the range [-1, 1].

    EXAMPLE
        double c = cos(0.0);  // returns 1.0

    BUGS
        None known.

    SEE ALSO
        sin(), tan(), cosf(), cosl()

    INTERNALS
        Forwards to __ieee754_cos().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(cos, x);
    return __ieee754_cos(x);
}

#if (LDBL_MANT_DIG == 53)
AROS_MAKE_ASM_SYM(typeof(cosl), cosl, AROS_CSYM_FROM_ASM_NAME(cosl), AROS_CSYM_FROM_ASM_NAME(cos));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cosl));
#endif
