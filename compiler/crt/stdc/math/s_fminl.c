/*-
 * Copyright (c) 2004 David Schultz <das@FreeBSD.ORG>
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

__FBSDID("$FreeBSD: src/lib/msun/src/s_fminl.c,v 1.1 2004/06/30 07:04:01 das Exp $");

#include <math.h>

#include "fpmath.h"

#if LDBL_MANT_DIG != DBL_MANT_DIG
/****************************************************************************
    NAME */
        long double fminl(

/*  SYNOPSIS */
        long double x,
        long double y)

/*  FUNCTION
        Return the lesser of x and y. A single NaN is treated as missing
        data: return the other argument. The implementation classifies
        NaNs before comparing values and handles opposite signs explicitly.

    INPUTS
        x - First long double value.
        y - Second long double value.
            Finite values, signed zeros, infinities and NaNs are accepted.

    RESULT
        The lesser numerical value when neither argument is NaN.
        For a pair containing +0 and -0, return -0, regardless of argument
        order. Equal non-NaN arguments with the same sign select y.
        If exactly one argument is NaN, return the non-NaN argument.
        If both arguments are NaN, return a NaN. There is no error sentinel.

    NOTES
        Include <math.h>. Arguments and results are values; no application
        storage is retained or allocated. The call performs no I/O or
        waiting and does not set or clear errno.

        This operation is not interchangeable with a comparison expression
        such as x < y ? x : y: that expression has different behavior for
        NaNs and opposite-signed zeros.

        When both inputs are NaNs, this implementation returns y without
        an arithmetic combination. This is not a guarantee of NaN payload
        preservation or unchanged exception state across the target ABI.

        Signaling-NaN handling and enabled floating-point traps depend on
        the target and its floating-point environment. A NaN result alone
        is not evidence of an errno error.

        A separate long double body is compiled when LDBL_MANT_DIG differs
        from DBL_MANT_DIG. Otherwise the double source supplies the symbol
        alias. The l suffix alone does not promise additional precision.

    SEE ALSO
        fmaxl(), fmin(), fminf()

*****************************************************************************/
{
	union IEEEl2bits u[2];

	u[0].e = x;
	mask_nbit_l(u[0]);
	u[1].e = y;
	mask_nbit_l(u[1]);

	/* Check for NaNs to avoid raising spurious exceptions. */
	if (u[0].bits.exp == 32767 && (u[0].bits.manh | u[0].bits.manl) != 0)
		return (y);
	if (u[1].bits.exp == 32767 && (u[1].bits.manh | u[1].bits.manl) != 0)
		return (x);

	/* Handle comparisons of signed zeroes. */
	if (u[0].bits.sign != u[1].bits.sign)
		return (u[1].bits.sign ? y : x);

	return (x < y ? x : y);
}
#endif
