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

#include "audio/wave.h"
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
} riff_header_t;

typedef struct {
	ULONG id;
	ULONG size;
} riff_chunk_t;

#define ID_RIFF MAKE_ID('R','I','F','F')
#define ID_WAVE MAKE_ID('W','A','V','E')
#define ID_fmt  MAKE_ID('f','m','t',' ')
#define ID_data MAKE_ID('d','a','t','a')

typedef struct {
	UWORD format;
	UWORD channels;
	ULONG frequency;
	ULONG bytespersec;
	UWORD blockalign;
	UWORD bitspersample;
	UWORD extrasize;
} wave_fmt_t;

#define WAVE_FORMAT_PCM 0x0001

#pragma pack()

BOOL WAVE_header(CONST_APTR p) {
	const riff_header_t *header = p;
	return rbe32(&header->id) == ID_RIFF && rbe32(&header->type) == ID_WAVE;
}

static void WAVE_close(WAVE_STREAM *stream);
static LONG WAVE_read(WAVE_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

WAVE_STREAM *WAVE_open(CONST_STRPTR filename) {
	BPTR file = ZERO;
	WAVE_STREAM *stream = NULL;
	riff_header_t header;
	riff_chunk_t chunk;
	wave_fmt_t fmt;
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
	stream->close = WAVE_close;
	stream->read = WAVE_read;
	stream->file = file;

	if (FRead(file, &header, 1, sizeof(header)) != sizeof(header)) {
		goto error;
	}
	if (rbe32(&header.id) != ID_RIFF || rbe32(&header.type) != ID_WAVE) {
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		goto error;
	}

	while (!done) {
		if (FRead(file, &chunk, 1, sizeof(chunk)) != sizeof(chunk)) {
			goto error;
		}
		wbe32(&chunk.id, chunk.id);
		wle32(&chunk.size, chunk.size);
		switch (chunk.id) {
			case ID_fmt:
				if (chunk.size != 16 && chunk.size != 18) {
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				if (FRead(file, &fmt, 1, chunk.size) != chunk.size) {
					goto error;
				}
				if (rle16(&fmt.format) != WAVE_FORMAT_PCM ||
					rle16(&fmt.channels) != 2 ||
					rle32(&fmt.frequency) != 44100 ||
					rle16(&fmt.bitspersample) != 16)
				{
					SetIoErr(ERROR_OBJECT_WRONG_TYPE);
					goto error;
				}
				break;
			case ID_data:
				stream->offset = GetFilePosition(file);
				stream->length = chunk.size;
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
	WAVE_close(stream);
	return NULL;
}

static void WAVE_close(WAVE_STREAM *stream) {
	if (stream) {
		Close(stream->file);
		FreeVec(stream);
	}
}

static LONG WAVE_read(WAVE_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
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
	memset((uint8 *)buffer + bytes_read, 0, length - bytes_read);
	return length;
}
