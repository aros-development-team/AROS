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

#include <float.h>
#include <limits.h>
#include "math.h"
#include "fpmath.h"
#include "math_private.h"

/****************************************************************************
    NAME */
        long double logbl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Return the base-two exponent of the magnitude of x
        as a floating-point value.

    INPUTS
        x - Long double value to examine.

    RESULT
        For finite nonzero x, e such that 2^e <= |x| < 2^(e+1).
        Zero returns negative infinity, infinity returns positive
        infinity, and NaN returns NaN.

    NOTES
        Include <math.h>. The zero path divides by zero and can
        raise FE_DIVBYZERO. A floating-point trap may prevent
        return. Unlike ilogbl(), special inputs produce
        floating-point values rather than integer sentinels.
        Where long double equals double, logbl() aliases logb().

    BUGS
        The separate long double subnormal scan requires an
        unsigned long wide enough for the significand field.
        Binary128 on a 32-bit-long target can shift by 63
        or 47 bits, which is undefined behavior in C.

    SEE ALSO
        logb(), logbf(), ilogbl(), frexpl()

*****************************************************************************/
{
	union IEEEl2bits u;
	unsigned long m;
	int b;

	u.e = x;
	if (u.bits.exp == 0) {
		if ((u.bits.manl | u.bits.manh) == 0) {	/* x == 0 */
			u.bits.sign = 1;
			return (1.0L / u.e);
		}
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
		return ((long double)(LDBL_MIN_EXP - b - 1));
	}
	if (u.bits.exp < (LDBL_MAX_EXP << 1) - 1)	/* normal */
		return ((long double)(u.bits.exp - LDBL_MAX_EXP + 1));
	else						/* +/- inf or nan */
		return (x * x);
}
