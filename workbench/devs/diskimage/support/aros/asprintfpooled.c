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
#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <SDI_stdarg.h>

AROS_UFP2(void, CountPutCh,
	AROS_UFPA(UBYTE, c, D0),
	AROS_UFPA(ULONG *, s, A3)
);

typedef struct {
	STRPTR Target;
	ULONG TargetSize;
} SNPrintfStream;

AROS_UFP2(void, SNPrintfPutCh,
	AROS_UFPA(UBYTE, c, D0),
	AROS_UFPA(SNPrintfStream *, s, A3)
);

VARARGS68K STRPTR ASPrintfPooled (APTR pool, CONST_STRPTR fmt, ...) {
	VA_LIST args;
	STRPTR res;
	VA_START(args, fmt);
	res = VASPrintfPooled(pool, fmt, VA_ARG(args, CONST_APTR));
	VA_END(args);
	return res;
}

STRPTR VASPrintfPooled (APTR pool, CONST_STRPTR fmt, CONST_APTR args) {
	STRPTR buf;
	ULONG len = 0;
	RawDoFmt(fmt, (APTR)args, (VOID_FUNC)CountPutCh, &len);
	buf = AllocVecPooled(pool, len);
	if (buf) {
		SNPrintfStream s = { buf, len };
		RawDoFmt(fmt, (APTR)args, (VOID_FUNC)SNPrintfPutCh, &s);
	}
	return buf;
}
