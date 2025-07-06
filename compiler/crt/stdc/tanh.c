/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function tanh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double tanh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the hyperbolic tangent of x.

    INPUTS
        x - input value.

    RESULT
        Returns tanh(x) = sinh(x) / cosh(x).

    NOTES
        Uses exponentials internally for calculation.

    EXAMPLE
        double val = tanh(1.0);

    BUGS
        None known.

    SEE ALSO
        sinh(), cosh()

    INTERNALS
        Forwards to __ieee754_tanh().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(tanh, x);
    return __ieee754_tanh(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(tanhl), tanhl, AROS_CSYM_FROM_ASM_NAME(tanhl), AROS_CSYM_FROM_ASM_NAME(tanh));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(tanhl));
#endif
