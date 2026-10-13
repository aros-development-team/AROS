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
        Extract the binary exponent of x and return it as a floating-point
        value. For finite nonzero x, the exponent e satisfies
        2^e <= |x| < 2^(e+1). The sign of x does not affect e. This is
        not a general logarithm: for example, 12 and -12 both yield 3.

    INPUTS
        x - Long double value whose exponent is required. Signed zeros,
            infinities and NaNs have the special results described below.

    RESULT
        The binary exponent, represented exactly as long double, for a
        finite nonzero input. A magnitude in [1, 2) yields positive zero.
        A subnormal input is normalized or scanned before its exponent
        is obtained.

        Either sign of zero yields negative infinity through division by
        zero. Either infinity yields positive infinity. NaN yields NaN.
        These value results assume nontrapping floating-point execution;
        an enabled trap may prevent the function from returning.

    NOTES
        Include <math.h>. Unlike ilogbl(), this function returns a
        floating-point value and uses infinities and NaNs for special
        inputs instead of integer sentinels. For finite nonzero x, its
        exponent is one less than the exponent reported by frexpl().

        The zero path performs a floating-point division by zero. Under
        an IEEE floating-point environment this raises FE_DIVBYZERO.
        The nonfinite path evaluates x * x. Exception flags, signaling-
        NaN handling and enabled traps depend on the target and its
        floating-point environment. No NaN payload preservation is
        guaranteed.

        The implementation does not set or clear errno. It does not
        allocate, modify or retain application storage, perform I/O, or
        wait. Finite exponent results do not depend on the rounding
        direction under ordinary gradual-underflow operation.

        The separate long double implementation scans significand fields
        in the target's IEEEl2bits layout. Where long double and double
        have equal precision, s_logb.c supplies the logbl alias instead;
        that implementation scales subnormal inputs by 2^54.

    BUGS
        The separate implementation uses an unsigned long scan mask.
        For the RISC-V binary128 layout, the significand fields have 64
        and 48 bits. If unsigned long has only 32 bits, the initial
        shifts by 63 or 47 have undefined behavior. Subnormal results
        on that path are not a portable guarantee of this source.

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
