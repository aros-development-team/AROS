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
        Extract the binary exponent of x as an int. For finite nonzero
        x, the exponent e satisfies 2^e <= |x| < 2^(e+1). The sign of x
        does not affect e. Subnormal inputs require a significand scan;
        see BUGS for implementation limitations on that path.

    INPUTS
        x - Long double value whose exponent is required.

    RESULT
        For a finite nonzero value, the binary exponent. For example,
        12 and -12 yield 3; 0.5 yields -1. Both signs of zero return
        FP_ILOGB0. NaN returns FP_ILOGBNAN; either infinity returns
        INT_MAX. These are integer sentinels, not valid finite exponents.

        AROS defines FP_ILOGB0 as -INT_MAX and FP_ILOGBNAN as INT_MAX.
        The return value alone cannot distinguish NaN from infinity.

    NOTES
        Include <math.h>; <limits.h> declares INT_MAX. This function
        returns an integer exponent, not the logarithm of x. It differs
        from frexpl(): for finite nonzero x, its exponent is one less
        than the exponent of the fraction returned by that function.

        The implementation reads the floating-point representation and
        uses integer operations. It does not set or clear errno and has
        no explicit floating-point exception-raising operation. Do not
        use errno or an assumed FE_INVALID signal to distinguish the
        special return cases. Classify x when that distinction matters.
        Signaling-NaN handling and trap behavior are target-dependent.

        No application storage is modified or retained. The function
        allocates nothing and performs no I/O or waiting.

        The separate long double implementation uses the target's
        IEEEl2bits layout. Where long double and double have equal
        precision, s_ilogb.c supplies the ilogbl alias instead.

    BUGS
        The separate implementation uses an unsigned long scan mask.
        Its shifts require that type to hold each significand field.
        For the RISC-V binary128 layout, those fields have 64 and 48
        bits. On a target with 32-bit unsigned long, the initial shifts
        by 63 or 47 have undefined behavior for subnormal inputs.

        The equal-precision double alias has a separate limitation: its
        subnormal scan left-shifts signed int32_t values into the sign
        bit. That also has undefined behavior in C. Subnormal results
        on these affected paths are not a portable source guarantee.

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
