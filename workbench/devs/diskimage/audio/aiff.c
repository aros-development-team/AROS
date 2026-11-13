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

#include "audio/aiff.h"
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0
#define ZERO MKBADDR(NULL)

#pragma pack(1)

typedef struct {
	ULONG id;
	ULONG size;
	ULONG type;
} iff_header_t;

typedef struct {
	ULONG id;
	ULONG size;
} iff_chunk_t;

#define ID_AIFF MAKE_ID('A','I','F','F')
#define ID_AIFC MAKE_ID('A','I','F','C')
#define ID_FVER MAKE_ID('F','V','E','R')
#define ID_COMM MAKE_ID('C','O','M','M')
#define ID_SSND MAKE_ID('S','S','N','D')

typedef struct {
	ULONG timestamp;
} aiff_fver_t;

#define AIFC_VERSION1 0xA2805140

typedef struct {
	UWORD channels;
	ULONG frames;
	UWORD bitspersample;
	UWORD frequency[5];
	ULONG compression;
} aiff_comm_t;

#define AIFF_COMP_NONE MAKE_ID('N','O','N','E')

typedef struct {
	ULONG dataoffset;
	ULONG blocksize;
} aiff_ssnd_t;

#pragma pack()

BOOL AIFF_header(CONST_APTR p) {
	const iff_header_t *header = p;
	return rbe32(&header->id) == ID_FORM && (rbe32(&header->type) == ID_AIFF || rbe32(&header->type) == ID_AIFC);
}

static inline LONG extended2long (UWORD *ext) {
	ULONG man;
	WORD exp;
	LONG sign;
	exp = rbe16(ext++);
	man = rbe32(ext);
	if (exp & 0x8000)
		sign = -1;
	else
		sign = 1;
	exp = (exp & 0x7fff) - 0x3fff;
	if (exp < 0) return 0;
	exp -= 31;
	if (exp > 0)
		man = 0x7fffffff;
	else
		man >>= -exp;
	return sign * man;
}

static void AIFF_close(AIFF_STREAM *stream);
static LONG AIFF_read(AIFF_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

AIFF_STREAM *AIFF_open(CONST_STRPTR filename) {
	BPTR file = ZERO;
	AIFF_STREAM *stream = NULL;
	iff_header_t header;
	iff_chunk_t chunk;
	aiff_fver_t fver;
	aiff_comm_t comm;
	aiff_ssnd_t ssnd;
	LONG done = FALSE;

	SetIoErr(NO_ERROR);

	file = Open(filename, MODE_OLDFILE);
	if (!file) {
		goto error;
	}

	stream = AllocVec(sizeof(*stream), MEMF_CLEAR);
	if (!stream) {
		SetIoErr(ERROR_NO_FREE_STORE);
		Close(file);
		goto error;
	}
	stream->close = AIFF_close;
	stream->read = AIFF_read;
	stream->file = file;

	if (FRead(file, &header, 1, sizeof(header)) != sizeof(header)) {
		goto error;
	}
	wbe32(&header.id, header.id);
	wbe32(&header.size, header.size);
	wbe32(&header.type, header.type);
	if (header.id != ID_FORM || (header.type != ID_AIFF && header.type != ID_AIFC)) {
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		goto error;
	}

	while (!done) {
		if (FRead(file, &chunk, 1, sizeof(chunk)) != sizeof(chunk)) {
			goto error;
		}
		wbe32(&chunk.id, chunk.id);
		wbe32(&chunk.size, chunk.size);
		switch (chunk.id) {
			case ID_FVER:
				if (chunk.size != sizeof(fver)) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				if (FRead(file, &fver, 1, sizeof(fver)) != sizeof(fver)) {
					goto error;
				}
				if (rbe32(&fver.timestamp) != AIFC_VERSION1) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				break;
			case ID_COMM:
				if (chunk.size != 18 && chunk.size != 22) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				if (FRead(file, &comm, 1, chunk.size) != chunk.size) {
					goto error;
				}
				if (rbe16(&comm.channels) != 2 ||
					extended2long(comm.frequency) != 44100 ||
					rbe16(&comm.bitspersample) != 16)
				{
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				if (chunk.size == 22 && comm.compression != AIFF_COMP_NONE) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				stream->length = rbe32(&comm.frames) * 4UL;
				break;
			case ID_SSND:
				if (chunk.size < sizeof(ssnd)) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				if (FRead(file, &ssnd, 1, sizeof(ssnd)) != sizeof(ssnd)) {
					goto error;
				}
				if (ssnd.dataoffset &&
					!ChangeFilePosition(file, rbe32(&ssnd.dataoffset), OFFSET_CURRENT))
				{
					goto error;
				}
				stream->offset = GetFilePosition(file);
				if (stream->length == 0) {
					stream->length = chunk.size - rbe32(&ssnd.dataoffset);
				}
				if ((LONG)stream->offset == -1 && IoErr()) {
					goto error;
				}
				done = TRUE;
				break;
			default:
				if (!ChangeFilePosition(file, (chunk.size+1)&~1, OFFSET_CURRENT)) {
					goto error;
				}
				break;
		}
	}

	return stream;

error:
	AIFF_close(stream);
	return NULL;
}

static void AIFF_close(AIFF_STREAM *stream) {
	if (stream) {
		Close(stream->file);
		FreeVec(stream);
	}
}

static LONG AIFF_read(AIFF_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
	BPTR file = stream->file;
	LONG read_length;
	LONG bytes_read;
	if (offset >= stream->length) {
		memset(buffer, 0, length);
		return length;
	}
	if (!ChangeFilePosition(file, stream->offset + offset, OFFSET_BEGINNING)) {
		return 0;
	}
	if ((offset + length) > stream->length) {
		read_length = stream->length;
	} else {
		read_length = length;
	}
	bytes_read = FRead(file, buffer, 1, read_length);
	if (bytes_read < 0) {
		return 0;
	}
	swab2(buffer, buffer, bytes_read);
	memset((uint8 *)buffer + bytes_read, 0, length - bytes_read);
	return length;
}
