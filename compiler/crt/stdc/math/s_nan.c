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
 * $FreeBSD: src/lib/msun/src/s_nan.c,v 1.2 2007/12/18 23:46:32 das Exp $
 */
 
#include <aros/cpu.h>

#include <ctype.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h> //for memset
#include <math.h>

#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__DragonFly__)
static __inline int digittoint(int c) {
	if ('0' <= c && c <= '9')
		return (c - '0');
	else if ('A' <= c && c <= 'F')
		return (c - 'A' + 10);
	else if ('a' <= c && c <= 'f')
		return (c - 'a' + 10);
	return 0;
}
#endif

/*
 * Scan a string of hexadecimal digits (the format nan(3) expects) and
 * make a bit array (using the local endianness). We stop when we
 * encounter an invalid character, NUL, etc.  If we overflow, we do
 * the same as gcc's __builtin_nan(), namely, discard the high order bits.
 *
 * The format this routine accepts needs to be compatible with what is used
 * in contrib/gdtoa/hexnan.c (for strtod/scanf) and what is used in
 * __builtin_nan(). In fact, we're only 100% compatible for strings we
 * consider valid, so we might be violating the C standard. But it's
 * impossible to use nan(3) portably anyway, so this seems good enough.
 */
void
_scan_nan(uint32_t *words, int num_words, const char *s)
{
	int si;		/* index into s */
	int bitpos;	/* index into words (in bits) */

	bzero(words, num_words * sizeof(uint32_t));

	/* Allow a leading '0x'. (It's expected, but redundant.) */
	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		s += 2;

	/* Scan forwards in the string, looking for the end of the sequence. */
	for (si = 0; isxdigit(s[si]); si++)
		;

	/* Scan backwards, filling in the bits in words[] as we go. */
#if !AROS_BIG_ENDIAN
	for (bitpos = 0; bitpos < 32 * num_words; bitpos += 4) {
#else
	for (bitpos = 32 * num_words - 4; bitpos >= 0; bitpos -= 4) {
#endif
		if (--si < 0)
			break;
		words[bitpos / 32] |= digittoint(s[si]) << (bitpos % 32);
	}
}

double
nan(const char *s)
{
	union {
		double d;
		uint32_t bits[2];
	} u;

	_scan_nan(u.bits, 2, s);
#if !AROS_BIG_ENDIAN
	u.bits[1] |= 0x7ff80000;
#else
	u.bits[0] |= 0x7ff80000;
#endif
	return (u.d);
}

/****************************************************************************
    NAME */
        float nanf(

/*  SYNOPSIS */
        const char *s)

/*  FUNCTION
        Construct a quiet float NaN from a hexadecimal tag string. The
        implementation scans an optional leading 0x or 0X followed by
        hexadecimal digits. Scanning stops at the first other character
        or the terminating NUL. See BUGS before relying on tag bits.

    INPUTS
        s - Non-NULL pointer to a readable NUL-terminated ASCII string.
            The string is read during the call and is not retained. An
            empty string is permitted. No leading whitespace or sign is
            skipped. NULL is not an empty-tag shorthand.

    RESULT
        A float NaN with its exponent and quiet bit forced on. An empty
        string, or a string with no initial hexadecimal digits after the
        optional prefix, supplies zero tag bits. Invalid trailing text
        does not produce a separate error return.

        On the little-endian path, the low 32 scanned bits are placed in
        the float word, then ORed with 0x7fc00000. Only fraction bits not
        forced by that mask can distinguish payloads. This construction
        is subject to the scanner defects described below.

    NOTES
        Include <math.h>. Tags are interpreted as hexadecimal, including
        without a 0x prefix. Different tags can produce the same NaN.
        Do not use the result as a portable payload serialization format.

        The scanner reads the complete initial digit sequence before
        filling its fixed-size word array. Work therefore grows with
        the digit-sequence length, even though only one word is retained.
        No storage is allocated or retained, no I/O or wait occurs, and
        the implementation does not set or clear errno.

        The source constructs the value through integer representation
        bits rather than invalid floating-point arithmetic. This is not
        a guarantee about exception state across every target ABI.

    BUGS
        The shared scanner shifts the signed int returned by digittoint.
        A digit of 8 through F shifted by 28 bits is not representable
        as int on the supported 32-bit-int model and has undefined C
        behavior. High tag digits therefore cannot be treated as a
        portable source-level sign or payload guarantee.

        The big-endian branch places the rightmost digit in the highest
        nibble of the output word, reversing the little-endian nibble
        placement. Tag-to-payload mapping differs between these paths.
        The scan index is an int; digit sequences exceeding INT_MAX
        characters are outside the supported input range.

    SEE ALSO
        nan(), nanl()

*****************************************************************************/
{
	union {
		float f;
		uint32_t bits[1];
	} u;

	_scan_nan(u.bits, 1, s);
	u.bits[0] |= 0x7fc00000;
	return (u.f);
}

#if LDBL_MANT_DIG == DBL_MANT_DIG
/* Alias nan -> nanl */
AROS_MAKE_ASM_SYM(typeof(nanl), nanl, AROS_CSYM_FROM_ASM_NAME(nanl), AROS_CSYM_FROM_ASM_NAME(nan));
AROS_EXPORT_ASM_SYM(AROS_CSYM_FROM_ASM_NAME(nanl));
#endif
