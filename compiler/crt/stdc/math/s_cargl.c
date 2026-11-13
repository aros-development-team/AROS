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
        Return the principal argument of z, in radians, with a branch cut
        along the negative real axis. The angle is calculated by
        atan2l(cimagl(z), creall(z)).

    INPUTS
        z - Complex input value.

    RESULT
        An angle in the interval [-pi, pi]. The signs of zero select
        the side of the branch cut. With a negative real component,
        imaginary +0 gives +pi and imaginary -0 gives -pi.
        An imaginary zero with positive real part is preserved.
        Infinite components select the corresponding axis or
        quadrant angle; a NaN component produces NaN.
        Signed-zero pairs differ on targets using the separate long
        double implementation; see BUGS.

    NOTES
        Include <complex.h>.

    BUGS
        On targets with a separate long double implementation,
        atan2l() treats both signed-zero components as a domain
        error, sets errno to EDOM and returns NaN if execution
        continues. It requests FE_INVALID, but the exception
        may not be raised by every AROS target. Where cargl()
        aliases carg(), the double behavior applies.

    SEE ALSO
        atan2l(), creall(), cimagl(), carg(), cargf()

*****************************************************************************/
{

	return (atan2l(cimagl(z), creall(z)));
}
