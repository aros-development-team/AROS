/*-
 * Copyright (c) 2011 David Schultz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Hyperbolic tangent of a complex argument z.  See s_ctanh.c for details.
 */

//__FBSDID("$FreeBSD: src/lib/msun/src/s_ctanhf.c,v 1.2 2011/10/21 06:30:16 das Exp $");

#include <complex.h>
#include "math.h"

#include "math_private.h"

/****************************************************************************
    NAME
        float complex ctanhf(

    SYNOPSIS
        float complex z)

    FUNCTION
        Return the complex hyperbolic tangent of z.

    INPUTS
        z - Float complex value x + i*y, with y in radians.

    RESULT
        An approximation to sinh(z) / cosh(z) in float complex.

        For infinite x, the real part is 1 with the sign of x
        and the imaginary part is zero. For finite y, that zero
        has the sign of sin(y) * cos(y); for infinite y, it has
        the sign of y. Its sign is unspecified when y is NaN.

        For NaN x and a signed-zero y, returns NaN with that
        same imaginary zero. For NaN x and nonzero y, both
        components are NaN. For finite x and infinite or NaN y,
        both components are NaN.

    NOTES
        Include <complex.h>. The mathematical function has poles
        at x = 0 and y = (k + 1/2) * pi for integer k. Rounded
        arguments near these poles can produce large results.

        Use round-to-nearest mode for argument reduction.
        Floating-point arithmetic can raise exceptions; enabled
        traps may prevent return. The function does not assign
        errno.

    BUGS
        Large finite real parts can reach undefined signed-shift
        arithmetic in expf(). Results on those paths are not
        portable.

    SEE ALSO
        ctanh(), ctanhl(), csinhf(), ccoshf(), tanhf()

*****************************************************************************/
float complex
ctanhf(float complex z)
{
	float x, y;
	float t, beta, s, rho, denom;
	uint32_t hx, ix;

	x = crealf(z);
	y = cimagf(z);

	GET_FLOAT_WORD(hx, x);
	ix = hx & 0x7fffffff;

	if (ix >= 0x7f800000) {
		if (ix & 0x7fffff)
			return (CMPLXF((x + 0) * (y + 0),
			    y == 0 ? y : (x + 0) * (y + 0)));
		SET_FLOAT_WORD(x, hx - 0x40000000);
		return (CMPLXF(x,
		    copysignf(0, isinf(y) ? y : sinf(y) * cosf(y))));
	}

	if (!isfinite(y))
		return (CMPLXF(y - y, y - y));

	if (ix >= 0x41300000) {	/* |x| >= 11 */
		float exp_mx = expf(-fabsf(x));
		return (CMPLXF(copysignf(1, x),
		    4 * sinf(y) * cosf(y) * exp_mx * exp_mx));
	}

	t = tanf(y);
	beta = 1.0 + t * t;
	s = sinhf(x);
	rho = sqrtf(1 + s * s);
	denom = 1 + beta * s * s;
	return (CMPLXF((beta * rho * s) / denom, t / denom));
}
