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

__FBSDID("$FreeBSD: src/lib/msun/src/s_round.c,v 1.4 2005/12/02 13:45:06 bde Exp $");

#include <float.h>
#include <math.h>

#include "math_private.h"

/****************************************************************************
    NAME */
        double round(

/*  SYNOPSIS */
        double x)

/*  FUNCTION
        Round x to the nearest integral value. When x is exactly halfway
        between two integers, choose the integer farther from zero. The
        value rule is independent of the current rounding direction.

    INPUTS
        x - Double value to round. Finite values, signed zeros,
            infinities and NaNs are accepted.

    RESULT
        The rounded value, returned as double, not as an integer type.
        For example, 0.5 becomes 1 and -0.5 becomes -1; 1.5 becomes 2
        and -1.5 becomes -2. Values with magnitude less than 0.5 produce
        a zero with the input sign. Signed zeros and infinities retain
        their signs. A NaN input produces a NaN result.

        An already integral finite value is unchanged. There is no integer
        conversion overflow or separate error sentinel.

    NOTES
        Include <math.h>. The implementation rounds the magnitude using
        floor(), applies the half-unit test and restores the sign.
        It does not set or clear errno, allocate or retain application
        storage, perform I/O, or wait.

        Floating-point exception flags need not remain unchanged. The
        floor path contains arithmetic intended to raise inexact for
        nonintegral inputs. Signaling-NaN behavior and enabled traps
        depend on the target and its floating-point environment; no NaN
        payload preservation guarantee is made.

        Do not use this call to obtain the current rounding-direction
        behavior of rint(). A later cast to an integer type still needs
        a separate range check.

    SEE ALSO
        floor(), rint(), lround(), llround()

*****************************************************************************/
{
	double t;
	uint32_t hx;

	GET_HIGH_WORD(hx, x);
	if ((hx & 0x7fffffff) == 0x7ff00000)
		return (x + x);

	if (!(hx & 0x80000000)) {
		t = floor(x);
		if (t - x <= -0.5)
			t += 1;
		return (t);
	} else {
		t = floor(-x);
		if (t + x <= -0.5)
			t += 1;
		return (-t);
	}
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(roundl), roundl, AROS_CSYM_FROM_ASM_NAME(roundl), AROS_CSYM_FROM_ASM_NAME(round));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(roundl));
#endif
