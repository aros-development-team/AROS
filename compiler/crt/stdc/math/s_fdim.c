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
        Return the positive difference of x and y. Test for NaNs first.
        If neither operand is NaN, subtract y from x only when x > y;
        otherwise return positive zero.

    INPUTS
        x - First double operand; the minuend when subtraction occurs.
        y - Second double operand; the subtrahend when subtraction occurs.

    RESULT
        The difference x - y, rounded to double, when x > y.
        Otherwise positive zero, including equal values, every pair of
        signed zeros, and two infinities with the same sign.

        If x is NaN, return x. Otherwise, if y is NaN, return y. When
        both are NaNs, x is selected. No payload or signaling-state
        preservation is guaranteed across the target ABI.

        Positive infinity minus a finite value, or a finite value minus
        negative infinity, produces positive infinity. The pair
        (+infinity, -infinity) also produces positive infinity.

    NOTES
        Include <math.h>. This is not the absolute difference: reversing
        the operands can change a positive result to zero. It is also
        not equivalent to fmax(x - y, 0), which evaluates the subtraction
        unconditionally and treats a single NaN differently.

        The subtraction can overflow or round. Its result and exception
        flags depend on the target floating-point environment. With
        IEEE arithmetic, finite overflow may yield positive infinity or
        the largest finite value according to the rounding direction.
        An enabled trap may prevent return. NaN classification and
        signaling-NaN handling can also affect exception state.

        The implementation does not set or clear errno, including on
        overflow. It allocates nothing, retains no application storage,
        and performs no I/O or waiting. No target-independent guarantee
        is made that floating-point exception flags remain unchanged.

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
        Return the positive difference of x and y. Test for NaNs first.
        If neither operand is NaN, subtract y from x only when x > y;
        otherwise return positive zero.

    INPUTS
        x - First float operand; the minuend when subtraction occurs.
        y - Second float operand; the subtrahend when subtraction occurs.

    RESULT
        The difference x - y, rounded to float, when x > y.
        Otherwise positive zero, including equal values, every pair of
        signed zeros, and two infinities with the same sign.

        If x is NaN, return x. Otherwise, if y is NaN, return y. When
        both are NaNs, x is selected. No payload or signaling-state
        preservation is guaranteed across the target ABI.

        Positive infinity minus a finite value, or a finite value minus
        negative infinity, produces positive infinity. The pair
        (+infinity, -infinity) also produces positive infinity.

    NOTES
        Include <math.h>. This is not the absolute difference: reversing
        the operands can change a positive result to zero. It is also
        not equivalent to fmax(x - y, 0), which evaluates the subtraction
        unconditionally and treats a single NaN differently.

        The subtraction can overflow or round. Its result and exception
        flags depend on the target floating-point environment. With
        IEEE arithmetic, finite overflow may yield positive infinity or
        the largest finite value according to the rounding direction.
        An enabled trap may prevent return. NaN classification and
        signaling-NaN handling can also affect exception state.

        The implementation does not set or clear errno, including on
        overflow. It allocates nothing, retains no application storage,
        and performs no I/O or waiting. No target-independent guarantee
        is made that floating-point exception flags remain unchanged.

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
        Return the positive difference of x and y. Test for NaNs first.
        If neither operand is NaN, subtract y from x only when x > y;
        otherwise return positive zero.

    INPUTS
        x - First long double operand; the minuend when subtraction occurs.
        y - Second long double operand; the subtrahend when subtraction occurs.

    RESULT
        The difference x - y, rounded to long double, when x > y.
        Otherwise positive zero, including equal values, every pair of
        signed zeros, and two infinities with the same sign.

        If x is NaN, return x. Otherwise, if y is NaN, return y. When
        both are NaNs, x is selected. No payload or signaling-state
        preservation is guaranteed across the target ABI.

        Positive infinity minus a finite value, or a finite value minus
        negative infinity, produces positive infinity. The pair
        (+infinity, -infinity) also produces positive infinity.

    NOTES
        Include <math.h>. This is not the absolute difference: reversing
        the operands can change a positive result to zero. It is also
        not equivalent to fmax(x - y, 0), which evaluates the subtraction
        unconditionally and treats a single NaN differently.

        The subtraction can overflow or round. Its result and exception
        flags depend on the target floating-point environment. With
        IEEE arithmetic, finite overflow may yield positive infinity or
        the largest finite value according to the rounding direction.
        An enabled trap may prevent return. NaN classification and
        signaling-NaN handling can also affect exception state.

        The implementation does not set or clear errno, including on
        overflow. It allocates nothing, retains no application storage,
        and performs no I/O or waiting. No target-independent guarantee
        is made that floating-point exception flags remain unchanged.

        The common source instantiates a long double body directly; it
        does not use a double-symbol alias. The available precision is
        still determined by the target's long double representation.

    SEE ALSO
        fmaxl(), fabsl(), fdim(), fdimf()

*****************************************************************************/
DECL(long double, fdiml)
