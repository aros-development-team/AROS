/*
 * From: @(#)s_ilogb.c 5.1 93/09/24
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_ilogbl.c,v 1.2 2008/02/22 02:30:35 das Exp $";
#endif

#include <float.h>
#include <limits.h>
#include <math.h>

#include "fpmath.h"

/****************************************************************************
    NAME */
        int ilogbl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Return the binary exponent of |x| as an integer.

    INPUTS
        x - Long double value to classify.

    RESULT
        The integer exponent e such that 2^e <= |x| < 2^(e+1)
        for finite nonzero x. Zero returns FP_ILOGB0; NaN returns
        FP_ILOGBNAN; infinity returns INT_MAX. In AROS,
        FP_ILOGBNAN equals INT_MAX, so these results are ambiguous.

    NOTES
        Include <math.h>. Use isnan() or isinf() on the input to
        distinguish NaN from infinity. Unlike logbl(), the
        result is an integer sentinel for exceptional inputs.
        Where long double equals double, ilogbl() aliases ilogb().

    BUGS
        The separate long double subnormal scan assumes unsigned
        long can hold the significand field. On 32-bit-long
        targets using binary128, shifts by 63 or 47 bits
        are undefined. The equal-precision double alias has
        its own signed-shift defect for subnormal inputs.

    SEE ALSO
        ilogb(), ilogbf(), logbl(), frexpl()

*****************************************************************************/
{
	union IEEEl2bits u;
	unsigned long m;
	int b;

	u.e = x;
	if (u.bits.exp == 0) {
		if ((u.bits.manl | u.bits.manh) == 0)
			return (FP_ILOGB0);
		/* denormalized */
		if (u.bits.manh == 0) {
			m = 1lu << (LDBL_MANL_SIZE - 1);
			for (b = LDBL_MANH_SIZE; !(u.bits.manl & m); m >>= 1)
				b++;
		} else {
			m = 1lu << (LDBL_MANH_SIZE - 1);
			for (b = 0; !(u.bits.manh & m); m >>= 1)
				b++;
		}
#ifdef LDBL_IMPLICIT_NBIT
		b++;
#endif
		return (LDBL_MIN_EXP - b - 1);
	} else if (u.bits.exp < (LDBL_MAX_EXP << 1) - 1)
		return (u.bits.exp - LDBL_MAX_EXP + 1);
	else if (u.bits.manl != 0 || u.bits.manh != 0)
		return (FP_ILOGBNAN);
	else
		return (INT_MAX);
}
