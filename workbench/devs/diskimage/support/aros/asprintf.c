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

#include <proto/exec.h>
#include <proto/utility.h>

#include <exec/rawfmt.h>
#include <SDI_stdarg.h>

#include "support.h"

typedef struct {
	STRPTR Target;
	ULONG TargetSize;
} SNPrintfStream;

SNPrintfStream *SNPrintfPutCh(SNPrintfStream * s, UBYTE c)
{
	if (s->TargetSize > 0) {
		if (s->TargetSize > 1) {
			*s->Target++ = c;
		} else {
			*s->Target++ = 0;
		}
		s->TargetSize--;
	}

	return s;
}

#if !defined(__AROS__)
VARARGS68K void SNPrintf (STRPTR buf, LONG len, CONST_STRPTR fmt, ...) {
	VA_LIST args;
	VA_START(args, fmt);

	VSNPrintf(buf, len, fmt, (RAWARG)args);

	VA_END(args);
}
#endif

VARARGS68K STRPTR ASPrintf (CONST_STRPTR fmt, ...) {
	VA_LIST args;
	STRPTR res;
	VA_START(args, fmt);
	res = VASPrintf(fmt, args);
	VA_END(args);
	return res;
}

#if (0)
void VSNPrintf (STRPTR buf, LONG len, CONST_STRPTR fmt, VA_LIST args) {
	SNPrintfStream s = { buf, len };
	VNewRawDoFmt(fmt, (VOID_FUNC)SNPrintfPutCh, &s, args);
}
#endif

STRPTR VASPrintf (CONST_STRPTR fmt, VA_LIST args) {
	STRPTR buf;
	ULONG len = 0;
	VA_LIST copy;

	/* We must copy the VA_LIST, as it will be altered in place */
	VA_COPY(copy, args);
	VNewRawDoFmt(fmt, RAWFMTFUNC_COUNT, &len, copy);
	VA_END(copy);

	buf = AllocVec(len, MEMF_ANY);
	if (buf) {
		SNPrintfStream s = { buf, len };
		VNewRawDoFmt(fmt, (VOID_FUNC)SNPrintfPutCh, &s, args);
	}
	return buf;
}
