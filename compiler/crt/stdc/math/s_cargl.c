/*-
 * Copyright (c) 2005-2008 David Schultz <das@FreeBSD.ORG>
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

//__FBSDID("$FreeBSD: src/lib/msun/src/s_cargl.c,v 1.1 2008/07/31 22:41:26 das Exp $");

#include <complex.h>
#include "math.h"

#include "math_private.h"

/****************************************************************************
    NAME */
        long double cargl(

/*  SYNOPSIS */
        long double complex z)

/*  FUNCTION
        Return the principal phase angle of z in radians, using
        atan2l(cimagl(z), creall(z)). The signs of both components
        select the quadrant. The branch cut lies on the negative real axis.

    INPUTS
        z - Complex value passed by value. Write x for its real component
            and y for its imaginary component. Both signs of zero, infinities
            and NaNs are significant.

    RESULT
        The principal angle, conventionally in [-pi, pi], represented in
        long double precision. Constants such as pi and pi/2 below denote
        the corresponding floating-point approximations. A NaN component
        produces a NaN result, even if the other component is infinite.

        For a nonzero z with neither component NaN:
            y = +/-0, x > 0: return the same signed zero as y.
            y = +/-0, x < 0: return +/-pi, with the sign of y.
            x = +/-0, y != 0: return +/-pi/2, with the sign of y.
            x = +infinity, y finite: return a zero with the sign of y.
            x = -infinity, y finite: return +/-pi, with the sign of y.
            y = +/-infinity, x finite: return +/-pi/2.
            x = +infinity, y = +/-infinity: return +/-pi/4.
            x = -infinity, y = +/-infinity: return +/-3*pi/4.

        When both components are zero, see BUGS: the result depends on
        whether this target uses the separate long double implementation
        or the double symbol alias.

    NOTES
        Include <complex.h>. The input is copied by value; no application
        storage is retained or allocated. The function returns an angle,
        not a magnitude, and does not modify z.

        The arithmetic can affect floating-point exception flags, including
        inexact. No correct-rounding or NaN-payload guarantee is made here.
        Behavior with enabled floating-point traps depends on the target
        and its floating-point environment.

        The build selects separate long double sources for i386, x86_64,
        m68k, aarch64, riscv and riscv64. Where LDBL_MANT_DIG equals
        DBL_MANT_DIG, cargl is an alias of carg and follows its behavior.
        The l suffix does not guarantee extra precision on every target.

    BUGS
        The separate long double path calls an atan2l wrapper that rejects
        all four signed-zero pairs. It calls feraiseexcept(FE_INVALID),
        then sets errno to EDOM and returns NaN if execution continues.
        This bypasses the core's signed-zero angle handling and differs
        from carg(), cargf() and the double-alias implementation.

        The wrapper ignores the return value of feraiseexcept(). On AROS
        aarch64 its inline implementation is a no-op, so this error path
        does not establish that FE_INVALID was raised. Other target
        implementations may raise or trap. errno is not cleared on other
        paths; a NaN input returns before this explicit domain check.

    SEE ALSO
        atan2l(), creall(), cimagl(), carg(), cargf()

*****************************************************************************/
{

	return (atan2l(cimagl(z), creall(z)));
}
