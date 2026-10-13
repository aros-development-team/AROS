/*-
 * Copyright (c) 2003, Steven G. Kargl
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

__FBSDID("$FreeBSD: src/lib/msun/src/s_roundl.c,v 1.2 2005/12/02 13:45:06 bde Exp $");

#include <float.h>
#if defined(__x86_64__)
#include "x86_64/ieeefp.h"
#elif defined(__i386__)
#include "i386/ieeefp.h"
#endif

#include "fpmath.h"
#include "math.h"
#include "math_private.h"

/****************************************************************************
    NAME */
        long double roundl(

/*  SYNOPSIS */
        long double x)

/*  FUNCTION
        Round x to the nearest integral value. When x is exactly halfway
        between two integers, choose the integer farther from zero. The
        value rule is independent of the current rounding direction.

    INPUTS
        x - Long double value to round. Finite values, signed zeros,
            infinities and NaNs are accepted.

    RESULT
        The rounded value, returned as long double, not as an integer type.
        For example, 0.5 becomes 1 and -0.5 becomes -1; 1.5 becomes 2
        and -1.5 becomes -2. Values with magnitude less than 0.5 produce
        a zero with the input sign. Signed zeros and infinities retain
        their signs. A NaN input produces a NaN result.

        An already integral finite value is unchanged. There is no integer
        conversion overflow or separate error sentinel.

    NOTES
        Include <math.h>. The implementation rounds the magnitude using
        floorl(), applies the half-unit test and restores the sign.
        It does not set or clear errno, allocate or retain application
        storage, perform I/O, or wait.

        Floating-point exception flags need not remain unchanged. The
        floor path contains arithmetic intended to raise inexact for
        nonintegral inputs. Signaling-NaN behavior and enabled traps
        depend on the target and its floating-point environment; no NaN
        payload preservation guarantee is made.

        Do not use this call to obtain the current rounding-direction
        behavior of rintl(). A later cast to an integer type still needs
        a separate range check.

        On i386, unless NO_FPSETPREC is defined, finite-input processing
        temporarily selects extended precision through ENTERI and restores
        the previous precision on normal return. This does not change the
        rounding direction. Restoration after a trap or nonlocal exit is
        not guaranteed by this function.

        The build selects the separate long double source for supported
        extended-precision targets. Where LDBL_MANT_DIG equals
        DBL_MANT_DIG, the double source supplies the roundl symbol alias.

    SEE ALSO
        floorl(), rintl(), lroundl(), llroundl()

*****************************************************************************/
{
	long double t;
	uint16_t hx;

	GET_LDBL_EXPSIGN(hx, x);
	if ((hx & 0x7fff) == 0x7fff)
		return (x + x);

	ENTERI();

	if (!(hx & 0x8000)) {
		t = floorl(x);
		if (t - x <= -0.5L)
			t += 1;
		RETURNI(t);
	} else {
		t = floorl(-x);
		if (t + x <= -0.5L)
			t += 1;
		RETURNI(-t);
	}
}
