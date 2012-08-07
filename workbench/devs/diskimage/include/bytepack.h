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

#ifndef BYTEPACK_H
#define BYTEPACK_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#include <string.h>

typedef struct {
	UBYTE *start, *current, *end;
} BytePackBuffer;

static inline void BytePackInit (BytePackBuffer *b, APTR addr, ULONG len) {
	b->start = b->current = addr;
	b->end = (UBYTE *)addr + len;
}

static inline void BytePackWrite8 (BytePackBuffer *b, UBYTE val) {
	if (b->current < b->end) {
		*b->current++ = val;
	}
}

static inline void BytePackWrite16MSB (BytePackBuffer *b, UWORD val) {
	BytePackWrite8(b, val >> 8);
	BytePackWrite8(b, val);
}

static inline void BytePackWrite24MSB (BytePackBuffer *b, ULONG val) {
	BytePackWrite8(b, val >> 16);
	BytePackWrite8(b, val >> 8);
	BytePackWrite8(b, val);
}
static inline void BytePackWrite32MSB (BytePackBuffer *b, ULONG val) {
	BytePackWrite8(b, val >> 24);
	BytePackWrite8(b, val >> 16);
	BytePackWrite8(b, val >> 8);
	BytePackWrite8(b, val);
}

static inline void BytePackWrite16LSB (BytePackBuffer *b, UWORD val) {
	BytePackWrite8(b, val);
	BytePackWrite8(b, val >> 8);
}

static inline void BytePackWrite24LSB (BytePackBuffer *b, ULONG val) {
	BytePackWrite8(b, val);
	BytePackWrite8(b, val >> 8);
	BytePackWrite8(b, val >> 16);
}
static inline void BytePackWrite32LSB (BytePackBuffer *b, ULONG val) {
	BytePackWrite8(b, val);
	BytePackWrite8(b, val >> 8);
	BytePackWrite8(b, val >> 16);
	BytePackWrite8(b, val >> 24);
}

static inline void BytePackWriteText (BytePackBuffer *b, CONST_STRPTR txt, ULONG len) {
	ULONG space = b->end - b->current;
	if (len > space) {
		len = space;
	}
	if (len > 0) {
		strncpy(b->current, txt, len);
		b->current += len;
	}
}

static inline ULONG BytePackBytesWritten (BytePackBuffer *b) {
	return (ULONG)(b->current - b->start);
}

#endif
