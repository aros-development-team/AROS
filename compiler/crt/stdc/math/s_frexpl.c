/*-
 * Copyright (c) 2004-2005 David Schultz <das@FreeBSD.ORG>
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
 *
 * $FreeBSD: src/lib/msun/src/s_frexpl.c,v 1.1 2005/03/07 04:54:51 das Exp $
 */

#include <float.h>
#include <math.h>

#include "fpmath.h"

#if LDBL_MAX_EXP != 0x4000
#error "Unsupported long double format"
#endif

/****************************************************************************
    NAME */
        long double frexpl(

/*  SYNOPSIS */
        long double x,
        int *ex)

/*  FUNCTION
        Split x into a signed binary fraction and an integer exponent.
        For finite nonzero x, the returned fraction m and stored exponent e
        satisfy x = m * 2^e, with 0.5 <= |m| < 1. Subnormal inputs are
        normalized before the fraction and exponent are extracted.

    INPUTS
        x - Long double value to decompose.
        ex - Non-NULL pointer to a writable int for the exponent. Required
            even when x is zero or nonfinite. The pointer is not retained.

    RESULT
        The signed fraction in long double precision. A zero input returns
        the same signed zero and stores exponent 0. An infinite or NaN
        input returns that value; see NOTES for the exponent output.
        There is no separate error sentinel.

    NOTES
        Include <math.h>. The exponent is a power of two, not a decimal
        exponent. For example, x = 12 yields fraction 0.75 and exponent 4.

        The call writes through ex but retains no application storage,
        allocates nothing, performs no I/O or waiting, and does not set
        or clear errno. NULL is not a supported exponent output: this
        function does not provide a discard-output or size-query mode.

        For infinity or NaN, the separate long double implementation does
        not write *ex. The previous int value remains; it must not be used
        as an exponent for the nonfinite result. Initialize the output if
        its unchanged value will be inspected after such a call.

        The separate source requires LDBL_MAX_EXP == 0x4000 and is selected
        by the build for supported extended-precision targets. Where long
        double and double have equal precision, the double source supplies
        the frexpl alias. That alias stores exponent 0 for nonfinite inputs.
        Do not rely on one nonfinite exponent value across AROS targets.

        No NaN payload, signaling-NaN or trap-behavior guarantee is made
        here. The value contract assumes the target's supported floating-
        point representation and ordinary subnormal handling.

    SEE ALSO
        frexp(), frexpf(), ldexpl()

*****************************************************************************/
{
	union IEEEl2bits u;

	u.e = x;
	switch (u.bits.exp) {
	case 0:		/* 0 or subnormal */
		if ((u.bits.manl | u.bits.manh) == 0) {
			*ex = 0;
		} else {
			u.e *= 0x1.0p514;
			*ex = u.bits.exp - 0x4200;
			u.bits.exp = 0x3ffe;
		}
		break;
	case 0x7fff:	/* infinity or NaN; value of *ex is unspecified */
		break;
	default:	/* normal */
		*ex = u.bits.exp - 0x3ffe;
		u.bits.exp = 0x3ffe;
		break;
	}
	return (u.e);
}
