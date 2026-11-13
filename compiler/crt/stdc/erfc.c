/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: C99 function erfc
*/

#include <math.h>
#include <errno.h>
#include <fenv.h>

#include <math_private.h>

/*****************************************************************************

    NAME */
#include <math.h>

        double erfc(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Computes the complementary error function of x.

    INPUTS
        x - a double value.

    RESULT
        The complementary error function, defined as 1 - erf(x).

    NOTES
        erfc(x) approaches 0 as x ? 8 and 2 as x ? -8.
        Returns NaN if input is NaN.
        Useful for calculating tail probabilities in statistics.

    EXAMPLE
        double y = erfc(0.0);     // y = 1.0
        double z = erfc(1.0);     // z ˜ 0.157299

    BUGS
        Accuracy can degrade for very large negative inputs.

    SEE ALSO
        erf(), erfcf(), erfcl()

    INTERNALS
        Calls __ieee754_erfc(x).
        Uses polynomial or rational approximations with argument reduction.
        Handles underflow and overflow in exponential terms carefully.

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(erfc, x);
    return __ieee754_erfc(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(erfcl), erfcl, AROS_CSYM_FROM_ASM_NAME(erfcl), AROS_CSYM_FROM_ASM_NAME(erfc));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(erfcl));
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */
