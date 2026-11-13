/*-
 * Copyright (c) 2007 David Schultz
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
 * $FreeBSD: src/lib/msun/ld128/s_nanl.c,v 1.3 2008/03/02 20:16:55 das Exp $
 */

#include "math.h"

#include "fpmath.h"
#include "math_private.h"

/****************************************************************************
    NAME
        long double nanl(

    SYNOPSIS
        const char *s)

    FUNCTION
        Construct a quiet long double NaN from a hexadecimal tag.
        An optional 0x or 0X prefix is followed by hexadecimal
        digits. Scanning stops at the first other character.

    INPUTS
        s - Readable, null-terminated ASCII string. May be empty,
            but must not be NULL. Leading whitespace and signs
            are not skipped. The string is not modified or retained.
            The initial digit sequence must fit in an int index.

    RESULT
        A quiet NaN, subject to the scanner limitation below.
        An empty tag, or a tag with no initial digits after the
        optional prefix, supplies zero payload bits. Trailing
        nonhexadecimal text does not produce an error.

        Payload and sign are representation-dependent. Excess
        high-order digits are discarded before exponent and quiet
        bits are imposed; distinct tags may produce the same value.
        The implementation does not set or clear errno.

    NOTES
        Include <math.h>. The implementation allocates no storage.
        It scans the entire initial digit sequence even when only
        a fixed number of digits contributes to the result.

        The 80-bit implementation uses three 32-bit words; the
        128-bit implementation uses four. When long double has
        the same precision as double, nanl is an alias of nan.
        Do not use NaN tags as a portable serialization format.

    BUGS
        The shared scanner uses signed left shifts. On targets
        with 32-bit int, shifting a hexadecimal digit of 8 through
        F into bit position 28 has undefined behavior. Such tags
        have no reliable source-level result.

        The big-endian scanner reverses nibble placement within
        each word relative to the little-endian scanner. The
        same tag need not preserve the same payload across targets.

    SEE ALSO
        nan(), nanf(), isnan()

*****************************************************************************/
long double
nanl(const char *s)
{
	union {
		union IEEEl2bits ieee;
		uint32_t bits[4];
	} u;

	_scan_nan(u.bits, 4, s);
	u.ieee.bits.exp = 0x7fff;
	u.ieee.bits.manh |= 1ULL << 47;	/* make it a quiet NaN */
	return (u.ieee.e);
}
