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
 * $FreeBSD: src/lib/msun/src/s_conj.c,v 1.2 2008/08/07 14:39:56 das Exp $
 */

#include <float.h>
#include <complex.h>

#include "math_private.h"

/****************************************************************************
    NAME */
        double complex conj(

/*  SYNOPSIS */
        double complex z)

/*  FUNCTION
        Return the complex conjugate of z. Keep the real component and
        negate the imaginary component. The result is constructed from
        these two components without a complex multiplication.

    INPUTS
        z - Complex value passed by value. Zero, infinite and NaN components
            are accepted.

    RESULT
        A complex value with real part equal to that of z and imaginary
        part equal to the negated imaginary part of z. The sign of an
        imaginary zero or infinity is reversed. A NaN component remains
        a NaN. There is no error sentinel.

    NOTES
        Include <complex.h>. The argument and result are values: no storage
        is retained or allocated, and the call performs no I/O or waiting.
        The implementation does not set errno. Floating-point exception
        flags and NaN payload preservation are not specified here.

    SEE ALSO
        creal(), cimag(), cproj()

*****************************************************************************/
{
	return (CMPLX(creal(z), -cimag(z)));
}

#if LDBL_MANT_DIG == 53
AROS_MAKE_ASM_SYM(typeof(conjl), conjl, AROS_CSYM_FROM_ASM_NAME(conjl), AROS_CSYM_FROM_ASM_NAME(conj));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(conjl));
#endif
