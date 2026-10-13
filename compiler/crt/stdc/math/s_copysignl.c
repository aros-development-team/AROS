/*-
 * Copyright (c) 2004 Stefan Farfeleder
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/lib/msun/src/s_copysignl.c,v 1.2 2007/01/07 07:54:21 das Exp $
 */

#include <math.h>

#include "fpmath.h"

/****************************************************************************
    NAME */
        long double copysignl(

/*  SYNOPSIS */
        long double x, long double y)

/*  FUNCTION
        Return the magnitude of x with the sign bit of y. The sign is
        copied even when y is negative zero or a NaN; its numerical
        magnitude is irrelevant.

    INPUTS
        x - Long double value supplying the magnitude.
        y - Long double value supplying the sign bit.

    RESULT
        A long double value with the magnitude of x and the sign of y.
        Zero and infinity retain their magnitude and receive the selected
        sign. A NaN x remains a NaN; a NaN y supplies only its sign bit.
        There is no error sentinel.

    NOTES
        Include <math.h>. This operation does not compare y with zero: a
        negative-zero sign is significant. The separate implementation
        copies the target IEEEl2bits sign field without arithmetic on the
        operands. Other representation fields of x are left unchanged
        in that local union. NaN payload and signaling-state preservation
        across argument passing and return are not guaranteed here.

        The function does not set or clear errno, allocate or retain
        application storage, perform I/O, or wait. Enabled traps and
        signaling-NaN handling depend on the target ABI and environment.

        The build selects the separate body for supported extended-
        precision targets. The double source provides a copysignl alias
        when LDBL_MANT_DIG is 53. The suffix alone does not promise
        additional precision.

    SEE ALSO
        copysign(), copysignf(), fabsl()

*****************************************************************************/
{
	union IEEEl2bits ux, uy;

	ux.e = x;
	uy.e = y;
	ux.bits.sign = uy.bits.sign;
	return (ux.e);
}
