/*-
 * Copyright (c) 2005-2011 David Schultz <das@FreeBSD.ORG>
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

__FBSDID("$FreeBSD: src/lib/msun/src/s_fmaf.c,v 1.3 2011/10/15 04:16:58 das Exp $");

#include <aros/system.h>

#include <fenv.h>
#include <float.h>

#include "math.h"
#include "math_private.h"

/*
 * Fused multiply-add: Compute x * y + z with a single rounding error.
 *
 * A double has more than twice as much precision than a float, so
 * direct double-precision arithmetic suffices, except where double
 * rounding occurs.
 */
/****************************************************************************
    NAME
        float fmaf(

    SYNOPSIS
        float x, float y, float z)

    FUNCTION
        Multiply x by y and add z without rounding the product
        to float first.

    INPUTS
        x - First factor.
        y - Second factor.
        z - Value to add to the product.

    RESULT
        The value of x*y + z as a float, subject to the rounding
        limitations below.

        A zero multiplied by infinity, or an infinite product
        added to an infinity of the opposite sign, produces NaN.
        NaN operands produce NaN.

    NOTES
        Include <math.h>. The calculation uses the current
        rounding mode. A correction in round-to-nearest mode
        temporarily selects round-toward-zero, then requests
        round-to-nearest again before returning.

        The function does not assign errno. Floating-point
        exceptions can occur; enabled traps may prevent return.

    BUGS
        Correct rounding requires working floating-point
        environment support and compiler support for dynamic
        rounding. Failed rounding-mode changes are not reported.

        On m68k and AArch64, fegetround() returns -1, so the
        correction for double rounding is skipped. Some results
        can differ from a single correctly rounded operation.

    SEE ALSO
        fma(), fmal(), fegetround(), fesetround()

*****************************************************************************/
float
fmaf(float x, float y, float z)
{
	double xy, result;
	uint32_t hr, lr;

	xy = (double)x * y;
	result = xy + z;
	EXTRACT_WORDS(hr, lr, result);
	/* Common case: The double precision result is fine. */
	if ((lr & 0x1fffffff) != 0x10000000 ||	/* not a halfway case */
	    (hr & 0x7ff00000) == 0x7ff00000 ||	/* NaN */
	    result - xy == z ||			/* exact */
	    fegetround() != FE_TONEAREST)	/* not round-to-nearest */
		return (result);

	/*
	 * If result is inexact, and exactly halfway between two float values,
	 * we need to adjust the low-order bit in the direction of the error.
	 */
	fesetround(FE_TOWARDZERO);
	volatile double vxy = xy;  /* XXX work around gcc CSE bug */
	double adjusted_result = vxy + z;
	fesetround(FE_TONEAREST);
	if (result == adjusted_result)
		SET_LOW_WORD(adjusted_result, lr + 1);
	return (adjusted_result);
}
