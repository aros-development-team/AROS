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
        Return the magnitude of x with the sign bit of y.

    INPUTS
        x - Value supplying the magnitude.
        y - Value supplying the sign bit.

    RESULT
        The magnitude of x with the sign of y, including the sign of
        zero, infinity or NaN. No numerical comparison with y is made.

    NOTES
        Include <math.h>. A NaN input retains its NaN classification;
        NaN payload behavior depends on the target representation.
        On equal-precision long double targets, copysignl() aliases
        copysign().

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
