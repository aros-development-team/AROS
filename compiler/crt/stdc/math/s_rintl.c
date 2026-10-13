/*-
 * Copyright (c) 2008 David Schultz <das@FreeBSD.ORG>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char rcsid[] = "$FreeBSD: src/lib/msun/src/s_rintl.c,v 1.5 2008/02/22 11:59:05 bde Exp $";
#endif

#include <float.h>
#include <fenv.h>
#include "math.h"

#include "fpmath.h"
#include "math_private.h"

#if LDBL_MAX_EXP != 0x4000
/* We also require the usual bias, min exp and expsign packing. */
#error "Unsupported long double format"
#endif

#define	BIAS	(LDBL_MAX_EXP - 1)

static const float
shift[2] = {
#if LDBL_MANT_DIG == 64
	0x1.0p63, -0x1.0p63
#elif LDBL_MANT_DIG == 113
	0x1.0p112, -0x1.0p112
#else
#error "Unsupported long double format"
#endif
};
static const float zero[2] = { 0.0, -0.0 };

/****************************************************************************
    NAME */
        long double rintl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Round x to an integral floating-point value using the current
        rounding direction. Nearest rounding selects the even integer
        at a tie; upward, downward and toward-zero rounding use their
        respective directions. See BUGS for implementation limitations.

    INPUTS
        x - Long double value to round.

    RESULT
        The rounded value in long double, not a C integer conversion.
        Signed zeros retain their sign. Infinities remain infinite and
        NaNs produce NaNs. An enabled trap may prevent return.
        Already integral finite values are unchanged. There is no
        separate error sentinel.

    NOTES
        Include <math.h>; use <fenv.h> to configure rounding. Halfway
        cases differ from roundl(), which rounds away from zero. A
        later integer conversion still needs its own range check.

        Unlike nearbyintl(), this function does not save and restore
        the floating-point environment. Its rounding arithmetic can
        raise FE_INEXACT for nonintegral inputs under IEEE execution.
        Existing exception flags are not deliberately cleared. Enabled
        traps, signaling NaNs and payload handling are target-dependent.

        The implementation does not set or clear errno, allocate or
        retain application storage, perform I/O, or wait.

        The separate source requires LDBL_MAX_EXP == 0x4000 and a 64- or
        113-bit significand. It adds and subtracts a signed 2^63 or 2^112
        and corrects zero signs. The double source supplies the alias
        when long double and double have equal precision.

    BUGS
        The separate source assumes intermediate evaluation in exactly
        long double precision. It does not select x87 extended precision.
        A reduced x87 precision setting can produce incorrect results;
        greater intermediate precision can cause double rounding.

        The equal-precision double alias has an additional limitation:
        its small negative-input path left-shifts a signed sign bit into
        bit 31, which has undefined behavior in C.

    SEE ALSO
        rint(), rintf(), nearbyintl(), roundl()

*****************************************************************************/
{
	union IEEEl2bits u;
	uint32_t expsign;
	int ex, sign;

	u.e = x;
	expsign = u.xbits.expsign;
	ex = expsign & 0x7fff;

	if (ex >= BIAS + LDBL_MANT_DIG - 1) {
		if (ex == BIAS + LDBL_MAX_EXP)
			return (x + x);	/* Inf, NaN, or unsupported format */
		return (x);		/* finite and already an integer */
	}
	sign = expsign >> 15;

	/*
	 * The following code assumes that intermediate results are
	 * evaluated in long double precision. If they are evaluated in
	 * greater precision, double rounding may occur, and if they are
	 * evaluated in less precision (as on i386), results will be
	 * wildly incorrect.
	 */
	x += shift[sign];
	x -= shift[sign];

	/*
	 * If the result is +-0, then it must have the same sign as x, but
	 * the above calculation doesn't always give this.  Fix up the sign.
	 */
	if (ex < BIAS && x == 0.0L)
		return (zero[sign]);

	return (x);
}
