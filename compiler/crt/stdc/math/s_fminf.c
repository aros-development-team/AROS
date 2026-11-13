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

__FBSDID("$FreeBSD: src/lib/msun/src/s_fminf.c,v 1.1 2004/06/30 07:04:01 das Exp $");

#include <math.h>

#include "fpmath.h"

/****************************************************************************
    NAME */
        float fminf(

/*  SYNOPSIS */
        float x,
        float y)

/*  FUNCTION
        Return the smaller numeric operand, ignoring a lone NaN.

    INPUTS
        x - First float operand.
        y - Second float operand.

    RESULT
        The smaller value for numeric operands. For opposite-signed
        zeros, return -0. If one operand is NaN, return the other.
        If both are NaNs, return a NaN.

    NOTES
        Include <math.h>. For two NaNs, the second argument is returned.

        The sign of zero and NaN handling differ from a simple conditional
        comparison. Signaling NaNs may affect the floating-point exception
        state; NaN payloads are not guaranteed.

    SEE ALSO
        fmaxf(), fmin(), fminl()

*****************************************************************************/
{
	union IEEEf2bits u[2];

	u[0].f = x;
	u[1].f = y;

	/* Check for NaNs to avoid raising spurious exceptions. */
	if (u[0].bits.exp == 255 && u[0].bits.man != 0)
		return (y);
	if (u[1].bits.exp == 255 && u[1].bits.man != 0)
		return (x);

	/* Handle comparisons of signed zeroes. */
	if (u[0].bits.sign != u[1].bits.sign)
		return (u[u[1].bits.sign].f);

	return (x < y ? x : y);
}
