/*-
 * Copyright (c) 2005 Bruce D. Evans and Steven G. Kargl
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
 * Hyperbolic sine of a complex argument z.  See s_csinh.c for details.
 */

//__FBSDID("$FreeBSD: src/lib/msun/src/s_csinhf.c,v 1.2 2011/10/21 06:29:32 das Exp $");

#include <complex.h>
#include "math.h"

#include "math_private.h"

static const float huge = 0x1p127;

/****************************************************************************
    NAME
        float complex csinhf(

    SYNOPSIS
        float complex z)

    FUNCTION
        Return the complex hyperbolic sine of z.

    INPUTS
        z - Float complex value x + i*y, with y in radians.

    RESULT
        An approximation to sinh(x) * cos(y) + i*cosh(x) * sin(y)
        in float complex.

        For signed-zero y, returns sinh(x) with that same
        imaginary zero; infinite or NaN x is retained in the
        real component. For zero x and infinite or NaN y,
        returns x + i*NaN.

        For finite nonzero x and infinite or NaN y, both
        components are NaN. For infinite x and finite nonzero y,
        evaluates x*cos(y) + i*infinity*sin(y). For infinite x
        and infinite or NaN y, returns x + i*NaN. For NaN x and
        nonzero y, both components are NaN.

    NOTES
        Include <complex.h>. Each component can overflow or
        underflow independently. Use round-to-nearest mode for
        trigonometric argument reduction. Floating-point
        exceptions can occur; enabled traps may prevent return.

        The function does not assign errno.

    BUGS
        Large finite real parts can reach undefined signed-shift
        arithmetic in expf(). Results on those paths are not
        portable.

    SEE ALSO
        ccoshf(), ctanhf(), csinf()

*****************************************************************************/
float complex
csinhf(float complex z)
{
	float x, y, h;
	int32_t hx, hy, ix, iy;

	x = crealf(z);
	y = cimagf(z);

	GET_FLOAT_WORD(hx, x);
	GET_FLOAT_WORD(hy, y);

	ix = 0x7fffffff & hx;
	iy = 0x7fffffff & hy;

	if (ix < 0x7f800000 && iy < 0x7f800000) {
		if (iy == 0)
			return (CMPLXF(sinhf(x), y));
		if (ix < 0x41100000)	/* |x| < 9: normal case */
			return (CMPLXF(sinhf(x) * cosf(y), coshf(x) * sinf(y)));

		/* |x| >= 9, so cosh(x) ~= exp(|x|) */
		if (ix < 0x42b17218) {
			/* x < 88.7: expf(|x|) won't overflow */
			h = expf(fabsf(x)) * 0.5F;
			return (CMPLXF(copysignf(h, x) * cosf(y), h * sinf(y)));
		} else if (ix < 0x4340b1e7) {
			/* x < 192.7: scale to avoid overflow */
			z = __ldexp_cexpf(CMPLXF(fabsf(x), y), -1);
			return (CMPLXF(crealf(z) * copysignf(1, x), cimagf(z)));
		} else {
			/* x >= 192.7: the result always overflows */
			h = huge * x;
			return (CMPLXF(h * cosf(y), h * h * sinf(y)));
		}
	}

	if (ix == 0)			/* && iy >= 0x7f800000 */
		return (CMPLXF(x, y - y));

	if (iy == 0)			/* && ix >= 0x7f800000 */
		return (CMPLXF(x + x, y));

	if (ix < 0x7f800000)		/* && iy >= 0x7f800000 */
		return (CMPLXF(y - y, y - y));

	if (ix == 0x7f800000) {
		if (iy >= 0x7f800000)
			return (CMPLXF(x, y - y));
		return (CMPLXF(x * cosf(y), INFINITY * sinf(y)));
	}

	return (CMPLXF((x + x) * (y - y), (x * x) * (y - y)));
}

/****************************************************************************
    NAME
        float complex csinf(

    SYNOPSIS
        float complex z)

    FUNCTION
        Return the complex sine of z.

    INPUTS
        z - Float complex value x + i*y, with x in radians.

    RESULT
        An approximation to sin(x) * cosh(y) + i*cos(x) * sinh(y)
        in float complex.

        For signed-zero x and infinite or NaN y, retains x as
        the real component. The imaginary component is infinity
        with the sign of y, or NaN, respectively.
        For infinite or NaN x and signed-zero y,
        returns NaN with that same imaginary zero.

        For infinite or NaN x and finite nonzero y, both
        components are NaN. For finite nonzero x and infinite y,
        evaluates infinity*sin(x) + i*y*cos(x). For infinite or
        NaN x and infinite y, returns NaN + i*y. For nonzero x
        and NaN y, both components are NaN.

    NOTES
        Include <complex.h>. Each component can overflow or
        underflow independently. Use round-to-nearest mode for
        trigonometric argument reduction. Floating-point
        exceptions can occur; enabled traps may prevent return.
        The function does not assign errno.

    BUGS
        Large finite imaginary parts can reach undefined
        signed-shift arithmetic in expf(). Results on those
        paths are not portable.

    SEE ALSO
        csin(), csinl(), ccosf(), csinhf()

*****************************************************************************/
float complex
csinf(float complex z)
{

	z = csinhf(CMPLXF(-cimagf(z), crealf(z)));
	return (CMPLXF(cimagf(z), -crealf(z)));
}
