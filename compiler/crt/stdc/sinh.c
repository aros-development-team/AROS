/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function sinh
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double sinh(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the hyperbolic sine of x.

    INPUTS
        x - input value.

    RESULT
        Returns sinh(x) = (e^x - e^{-x}) / 2.

    NOTES
        Uses exponentials internally for calculation.

    EXAMPLE
        double val = sinh(1.0);

    BUGS
        None known.

    SEE ALSO
        cosh(), tanh()

    INTERNALS
        Forwards to __ieee754_sinh().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(sinh, x);
    return __ieee754_sinh(x);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(sinhl), sinhl, AROS_CSYM_FROM_ASM_NAME(sinhl), AROS_CSYM_FROM_ASM_NAME(sinh));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(sinhl));
#endif
