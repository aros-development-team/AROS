/*-
 * Copyright (c) 2005 David Schultz <das@FreeBSD.ORG>
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

//__FBSDID("$FreeBSD: src/lib/msun/src/s_carg.c,v 1.1 2007/12/12 23:43:51 das Exp $");

#include <float.h>
#include <complex.h>
#include "math.h"

#include "math_private.h"

/****************************************************************************
    NAME */
        double carg(

/*  SYNOPSIS */
        double complex z)

/*  FUNCTION
        Return the principal argument of z, in radians, with a branch cut
        along the negative real axis. The angle is calculated by
        atan2(cimag(z), creal(z)).

    INPUTS
        z - Complex input value.

    RESULT
        An angle in the interval [-pi, pi]. The signs of zero select
        the side of the branch cut. With a negative real component,
        imaginary +0 gives +pi and imaginary -0 gives -pi.
        An imaginary zero with positive real part is preserved.
        Infinite components select the corresponding axis or
        quadrant angle; a NaN component produces NaN.

    NOTES
        Include <complex.h>.

    SEE ALSO
        atan2(), creal(), cimag(), cargf(), cargl()

*****************************************************************************/
{

	return (atan2(cimag(z), creal(z)));
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(cargl), cargl, AROS_CSYM_FROM_ASM_NAME(cargl), AROS_CSYM_FROM_ASM_NAME(carg));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cargl));
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */
