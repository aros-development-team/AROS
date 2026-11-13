/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>
#include "base64.h"

static BOOL inline is_base64 (const TEXT c) {
	if ((c >= 'A' && c <= 'Z') ||
		(c >= 'a' && c <= 'z') ||
		(c >= '0' && c <= '9') ||
		c == '+' || c == '/' || c == '=')
		return TRUE;
	else
		return FALSE;
}

void cleanup_base64 (STRPTR src) {
	STRPTR dst = src;
	while (*src) {
		if (is_base64(*src)) {
			*dst++ = *src++;
		} else {
			src++;
		}
	}
	*dst = 0;
}

static inline UBYTE decode_base64_char (const TEXT c) {
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return 0;
}

ULONG decode_base64(CONST_STRPTR src, STRPTR dst) {
	ULONG cnt, dlen;
	UBYTE c0, c1, c2, c3;
	cnt = strlen(src) >> 2;
	dlen = cnt * 3;
	while (cnt--) {
		c0 = decode_base64_char(*src++);
		c1 = decode_base64_char(*src++);
		c2 = decode_base64_char(*src++);
		c3 = decode_base64_char(*src++);
		*dst++ = (c0 << 2)|(c1 >> 4);
		*dst++ = (c1 << 4)|(c2 >> 2);
		*dst++ = (c2 << 6)|c3;
	}
	while (*--src == '=' && dlen) dlen--;
	return dlen;
}
