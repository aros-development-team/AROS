/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/lib/libc/locale/iswctype.c,v 1.9 2007/10/23 17:39:28 ache Exp $");

#include <wctype.h>

#ifdef __AROS__
#include <ctype.h>
#include "_ctype.h"
#define _CTYPE_A _ISalpha
#define _CTYPE_C _IScntrl
#define _CTYPE_D _ISdigit
#define _CTYPE_G _ISgraph
#define _CTYPE_L _ISlower
#define _CTYPE_P _ISpunct
#define _CTYPE_S _ISspace
#define _CTYPE_U _ISupper
#define _CTYPE_X _ISxdigit
#define _CTYPE_B _ISblank
#define _CTYPE_R _ISprint
#endif

#undef iswalnum
int
iswalnum(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_A|_CTYPE_D));
}

#undef iswalpha
int
iswalpha(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_A));
}

#undef iswascii
int
iswascii(wc)
	wint_t wc;
{
	return ((wc & ~0x7F) == 0);
}

#undef iswblank
int
iswblank(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_B));
}

#undef iswcntrl
int
iswcntrl(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_C));
}

#undef iswdigit
int
iswdigit(wc)
	wint_t wc;
{
	return (__isctype(wc, _CTYPE_D));
}

#undef iswgraph
int
iswgraph(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_G));
}

#undef iswhexnumber 
int
iswhexnumber(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_X));
}

#ifndef __AROS__ /* not suppoted atm */
#undef iswideogram
int
iswideogram(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_I));
}
#endif

#undef iswlower
int
iswlower(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_L));
}

#undef iswnumber
int
iswnumber(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_D));
}

#ifndef __AROS__ /* not suppoted atm */
#undef iswphonogram	
int
iswphonogram(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_Q));
}
#endif

#undef iswprint
int
iswprint(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_R));
}

#undef iswpunct
int
iswpunct(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_P));
}

#undef iswrune
int
iswrune(wc)
	wint_t wc;
{
	return (__istype(wc, 0xFFFFFF00L));
}

#undef iswspace
int
iswspace(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_S));
}

#ifndef __AROS__ /* not suppoted atm */
#undef iswspecial
int
iswspecial(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_T));
}
#endif

#undef iswupper
int
iswupper(wc)
	wint_t wc;
{
	return (__istype(wc, _CTYPE_U));
}

#undef iswxdigit
int
iswxdigit(wc)
	wint_t wc;
{
	return (__isctype(wc, _CTYPE_X));
}

#undef towlower
wint_t
towlower(wc)
	wint_t wc;
{
        return (__tolower(wc));
}

#undef towupper
wint_t
towupper(wc)
	wint_t wc;
{
        return (__toupper(wc));
}

