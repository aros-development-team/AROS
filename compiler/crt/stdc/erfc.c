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
        x - input value.

    RESULT
        Returns 1 - erf(x).

    NOTES
        Useful for calculations involving tail probabilities.

    EXAMPLE
        double val = erfc(1.0);

    BUGS
        None known.

    SEE ALSO
        erf(), erfcf(), erfcl()

    INTERNALS
        Forwards to __ieee754_erfc().

******************************************************************************/
{
    FORWARD_IF_NAN_OR_INF(erfc, x);
    return __ieee754_erfc(x);
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(erfcl), erfcl, AROS_CSYM_FROM_ASM_NAME(erfcl), AROS_CSYM_FROM_ASM_NAME(erfc));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(erfcl));
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */
