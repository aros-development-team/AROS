/*	$OpenBSD: s_ctan.c,v 1.6 2013/07/03 04:46:36 espie Exp $	*/
/*
 * Copyright (c) 2008 Stephen L. Moshier <steve@moshier.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*							ctan()
 *
 *	Complex circular tangent
 *
 *
 *
 * SYNOPSIS:
 *
 * double complex ctan();
 * double complex z, w;
 *
 * w = ctan (z);
 *
 *
 *
 * DESCRIPTION:
 *
 * If
 *     z = x + iy,
 *
 * then
 *
 *           sin 2x  +  i sinh 2y
 *     w  =  --------------------.
 *            cos 2x  +  cosh 2y
 *
 * On the real axis the denominator is zero at odd multiples
 * of PI/2.  The denominator is evaluated by its Taylor
 * series near these points.
 *
 * ctan(z) = -i ctanh(iz).
 *
 * ACCURACY:
 *
 *                      Relative error:
 * arithmetic   domain     # trials      peak         rms
 *    DEC       -10,+10      5200       7.1e-17     1.6e-17
 *    IEEE      -10,+10     30000       7.2e-16     1.2e-16
 * Also tested by ctan * ccot = 1 and catan(ctan(z))  =  z.
 */

#include <float.h>
#include <complex.h>
#include "math.h"
#include "math_private.h"

double complex
ctan(double complex z)
{

	/* ctan(z) = -I * ctanh(I * z) = I * conj(ctanh(I * conj(z))) */
	z = ctanh(CMPLX(cimag(z), creal(z)));
	return (CMPLX(cimag(z), creal(z)));
}

#if	LDBL_MANT_DIG == DBL_MANT_DIG
AROS_MAKE_ASM_SYM(typeof(ctanl), ctanl, AROS_CSYM_FROM_ASM_NAME(ctanl), AROS_CSYM_FROM_ASM_NAME(ctan));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(ctanl));
#endif	/* LDBL_MANT_DIG == DBL_MANT_DIG */
