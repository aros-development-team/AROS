/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function lgamma
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double lgamma(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the natural logarithm of the absolute value of the gamma function.

    INPUTS
        x - input value.

    RESULT
        Returns ln(|G(x)|).
        Domain errors set errno to EDOM.

    NOTES
        Useful for large arguments to avoid overflow.

    EXAMPLE
        double val = lgamma(5.0);

    BUGS
        None known.

    SEE ALSO
        tgamma(), log()

    INTERNALS
        Forwards to __ieee754_lgamma().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(lgamma, x);
    return __ieee754_lgamma(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(lgammal), lgammal, AROS_CSYM_FROM_ASM_NAME(lgammal), AROS_CSYM_FROM_ASM_NAME(lgamma));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(lgammal));
#endif
