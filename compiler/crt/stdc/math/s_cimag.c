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
 * $FreeBSD: src/lib/msun/src/s_cimag.c,v 1.3 2009/03/14 18:24:15 das Exp $
 */

#include <float.h>
#include <complex.h>
#include "math_private.h"

/****************************************************************************
    NAME */
        double cimag(

/*  SYNOPSIS */
        double complex z)

/*  FUNCTION
        Return the imaginary component of z in double precision.
        The other component is discarded; no magnitude or phase is computed.

    INPUTS
        z - Complex value passed by value. Zero, infinite and NaN components
            are accepted.

    RESULT
        The imaginary component of z. The component's sign is retained,
        including the sign of zero. An infinite component remains infinite;
        a NaN component produces a NaN. There is no error sentinel.

    NOTES
        Include <complex.h>. The argument and result are values: no storage
        is retained or allocated, and the call performs no I/O or waiting.
        The implementation does not set errno. Floating-point exception
        flags and NaN payload preservation are not specified here.

    SEE ALSO
        creal(), conj(), cproj()

*****************************************************************************/
{
	const double_complex z1 = { .f = z };

	return (IMAGPART(z1));
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(cimagl), cimagl, AROS_CSYM_FROM_ASM_NAME(cimagl), AROS_CSYM_FROM_ASM_NAME(cimag));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(cimagl));
#endif
