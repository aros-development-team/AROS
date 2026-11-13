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

__FBSDID("$FreeBSD: src/lib/msun/src/s_fdim.c,v 1.1 2004/06/30 07:04:01 das Exp $");

#include <math.h>

#define	DECL(type, fn)			\
type					\
fn(type x, type y)			\
{					\
					\
	if (isnan(x))			\
		return (x);		\
	if (isnan(y))			\
		return (y);		\
	return (x > y ? x - y : 0.0);	\
}

/****************************************************************************
    NAME
        double fdim(

    SYNOPSIS
        double x,
        double y)

    FUNCTION
        Return the positive difference between x and y.

    INPUTS
        x - First double operand.
        y - Second double operand.

    RESULT
        Return x - y when x is greater than y; otherwise return +0.
        If x is NaN, return x; otherwise if y is NaN, return y.
        The subtraction may overflow to infinity or round to a finite
        result according to the floating-point environment.

    NOTES
        Include <math.h>. This is not an absolute difference. Unlike
        fmax(x - y, 0), the subtraction is evaluated only when x > y.
        Subtraction may raise floating-point exceptions, but this
        implementation does not set errno on overflow.
        NaN payload and signaling behavior are not guaranteed.

    SEE ALSO
        fmax(), fabs(), fdimf(), fdiml()

*****************************************************************************/
DECL(double, fdim)
/****************************************************************************
    NAME
        float fdimf(

    SYNOPSIS
        float x,
        float y)

    FUNCTION
        Return the positive difference between x and y.

    INPUTS
        x - First float operand.
        y - Second float operand.

    RESULT
        Return x - y when x is greater than y; otherwise return +0.
        If x is NaN, return x; otherwise if y is NaN, return y.
        The subtraction may overflow to infinity or round to a finite
        result according to the floating-point environment.

    NOTES
        Include <math.h>. This is not an absolute difference. Unlike
        fmax(x - y, 0), the subtraction is evaluated only when x > y.
        Subtraction may raise floating-point exceptions, but this
        implementation does not set errno on overflow.
        NaN payload and signaling behavior are not guaranteed.

    SEE ALSO
        fmaxf(), fabsf(), fdim(), fdiml()

*****************************************************************************/
DECL(float, fdimf)
/****************************************************************************
    NAME
        long double fdiml(

    SYNOPSIS
        long double x,
        long double y)

    FUNCTION
        Return the positive difference between x and y.

    INPUTS
        x - First long double operand.
        y - Second long double operand.

    RESULT
        Return x - y when x is greater than y; otherwise return +0.
        If x is NaN, return x; otherwise if y is NaN, return y.
        The subtraction may overflow to infinity or round to a finite
        result according to the floating-point environment.

    NOTES
        Include <math.h>. This is not an absolute difference. Unlike
        fmax(x - y, 0), the subtraction is evaluated only when x > y.
        Subtraction may raise floating-point exceptions, but this
        implementation does not set errno on overflow.
        NaN payload and signaling behavior are not guaranteed.

    SEE ALSO
        fmaxl(), fabsl(), fdim(), fdimf()

*****************************************************************************/
DECL(long double, fdiml)
