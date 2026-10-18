/*-
 * Copyright (c) 2007-2008 David Schultz <das@FreeBSD.ORG>
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

#include <float.h>
#include <complex.h>
#include "math.h"

#include "math_private.h"

#ifdef __clang__
/*
 * gcc doesn't implement complex multiplication or division correctly,
 * so we need to handle infinities specially. We turn on this pragma to
 * notify conforming c99 compilers that the fast-but-incorrect code that
 * gcc generates is acceptable, since the special cases have already been
 * handled.
 */
#pragma	STDC CX_LIMITED_RANGE	ON
#endif

/*
 * We risk spurious overflow for components >= LDBL_MAX / (1 + sqrt(2)).
 * Rather than determining the fully precise value at which we might
 * overflow, just use a threshold of approximately LDBL_MAX / 4.
 */
#if LDBL_MAX_EXP != 0x4000
#error "Unsupported long double format"
#else
#define	THRESH	0x1p16382L
#endif

/****************************************************************************
    NAME
        long double complex csqrtl(

    SYNOPSIS
        long double complex z)

    FUNCTION
        Return the principal square root of a complex number.

    INPUTS
        z - Complex argument with long double components.

    RESULT
        An approximation to the square root of z, with a
        nonnegative real part. For finite inputs, the imaginary
        part has the sign of the input imaginary part, subject
        to the limitations below.

        At complex zero, the real part is positive zero and the
        imaginary part retains its sign. On the negative real
        axis, the sign of the imaginary zero selects the side
        of the branch cut.

        An infinite imaginary part returns positive infinity
        plus the original imaginary infinity, even when the real
        part is NaN. For a finite imaginary part, positive real
        infinity returns positive infinity and a signed imaginary
        zero; negative real infinity returns positive real zero
        and a signed imaginary infinity.

        Other NaN inputs return two NaNs, except that a positive
        infinite real part remains positive infinity, and a
        negative infinite real part produces an imaginary
        infinity with the sign of the input imaginary NaN.

    NOTES
        Include <complex.h>. Extended-precision targets use
        sqrtl() and hypotl(). Where long double has the same
        precision as double, csqrtl() aliases csqrt().

        Floating-point exceptions may occur, and enabled traps
        may prevent return. The extended-precision path relies
        on the floating-point environment support used by sqrtl().
        On m68k, that dependency can clear or change errno.

    BUGS
        Very small finite components can underflow internally
        before the square root is taken. A purely imaginary
        argument whose magnitude is the smallest positive
        subnormal can produce an infinite imaginary result
        in round-to-nearest mode.

        The m68k and AArch64 floating-point environment stubs
        do not provide the rounding and exception control
        required by sqrtl(). Correct rounding and environment
        preservation are not assured on those paths.

    SEE ALSO
        csqrt(), csqrtf(), sqrtl(), hypotl()

*****************************************************************************/
long double complex
csqrtl(long double complex z)
{
	long double complex result;
	long double a, b;
	long double t;
	int scale;

	a = creall(z);
	b = cimagl(z);

	/* Handle special cases. */
	if (z == 0)
		return (CMPLXL(0, b));
	if (isinf(b))
		return (CMPLXL(INFINITY, b));
	if (isnan(a)) {
		t = (b - b) / (b - b);	/* raise invalid if b is not a NaN */
		return (CMPLXL(a, t));	/* return NaN + NaN i */
	}
	if (isinf(a)) {
		/*
		 * csqrt(inf + NaN i)  = inf +  NaN i
		 * csqrt(inf + y i)    = inf +  0 i
		 * csqrt(-inf + NaN i) = NaN +- inf i
		 * csqrt(-inf + y i)   = 0   +  inf i
		 */
		if (signbit(a))
			return (CMPLXL(fabsl(b - b), copysignl(a, b)));
		else
			return (CMPLXL(a, copysignl(b - b, b)));
	}
	/*
	 * The remaining special case (b is NaN) is handled just fine by
	 * the normal code path below.
	 */

	/* Scale to avoid overflow. */
	if (fabsl(a) >= THRESH || fabsl(b) >= THRESH) {
		a *= 0.25;
		b *= 0.25;
		scale = 1;
	} else {
		scale = 0;
	}

	/* Algorithm 312, CACM vol 10, Oct 1967. */
	if (a >= 0) {
		t = sqrtl((a + hypotl(a, b)) * 0.5);
		result = CMPLXL(t, b / (2 * t));
	} else {
		t = sqrtl((-a + hypotl(a, b)) * 0.5);
		result = CMPLXL(fabsl(b) / (2 * t), copysignl(t, b));
	}

	/* Rescale. */
	if (scale)
		return (result * 2);
	else
		return (result);
}
