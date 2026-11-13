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

#include "support.h"
#include "endian.h"

void swab2 (CONST_APTR source, APTR dest, ULONG bytes) {
	ULONG longs = (bytes >> 1) >> 2;
	ULONG words = (bytes >> 1) & 3;
	const ULONG *l_src;
	ULONG *l_dst;
	const UWORD *w_src;
	UWORD *w_dst;
	const ULONG m1 = 0xff00ff00;
	const ULONG m2 = 0x00ff00ff;
	l_src = (const ULONG *)source;
	l_dst = (ULONG *)dest;
	while (longs) {
		*l_dst++ = ((*l_src << 8) & m1)|((*l_src >> 8) & m2);
		l_src++;
		*l_dst++ = ((*l_src << 8) & m1)|((*l_src >> 8) & m2);
		l_src++;
		longs--;
	}
	w_src = (const UWORD *)l_src;
	w_dst = (UWORD *)l_dst;
	switch (words) {
		case 3:
			wswap16(w_dst++, *w_src); w_src++;
		case 2:
			wswap16(w_dst++, *w_src); w_src++;
		case 1:
			wswap16(w_dst++, *w_src); w_src++;
		default:
			break;
	}
}
