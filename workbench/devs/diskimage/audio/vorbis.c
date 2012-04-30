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

#include "audio/vorbis.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <stdio.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0
#define ZERO MKBADDR(NULL)
#define OGG_SEEK_SET SEEK_SET
#define OGG_SEEK_CUR SEEK_CUR
#define OGG_SEEK_END SEEK_END

#define ID_OggS MAKE_ID('O','g','g','S')

BOOL VORBIS_header(CONST_APTR p) {
	return rbe32(p) == ID_OggS;
}

static void VORBIS_close(VORBIS_STREAM *stream);
static LONG VORBIS_read(VORBIS_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

static size_t ogg_read (APTR buffer, size_t size, size_t nitems, APTR file);
static int ogg_seek (APTR file, ogg_int64_t offset, int mode);
static long ogg_tell (APTR file);

static const ov_callbacks callbacks = {
	ogg_read,
	ogg_seek,
	NULL,
	ogg_tell
};

VORBIS_STREAM *VORBIS_open(CONST_STRPTR filename) {
	BPTR file = ZERO;
	VORBIS_STREAM *stream = NULL;
	vorbis_info *vi;
	LONG error = NO_ERROR;

	file = Open(filename, MODE_OLDFILE);
	if (!file) {
		error = IoErr();
		goto error;
	}

	stream = AllocVec(sizeof(*stream), MEMF_CLEAR);
	if (!stream) {
		error = ERROR_NO_FREE_STORE;
		Close(file);
		goto error;
	}
	stream->close = VORBIS_close;
	stream->read = VORBIS_read;
	stream->file = file;

	stream->vf = AllocVec(sizeof(OggVorbis_File), MEMF_CLEAR);
	if (!stream->vf) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (ov_open_callbacks(stream, stream->vf, NULL, 0, callbacks) < 0) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	stream->bs = -1;

	vi = ov_info(stream->vf, stream->bs);
	if (vi->channels != 2 || vi->rate != 44100) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	stream->total_samples = ov_pcm_total(stream->vf, stream->bs);
	stream->length = stream->total_samples << 2;

	return stream;

error:
	VORBIS_close(stream);
	SetIoErr(error);
	return NULL;
}

static void VORBIS_close(VORBIS_STREAM *stream) {
	if (stream) {
		if (stream->vf) {
			ov_clear(stream->vf);
			FreeVec(stream->vf);
		}
		Close(stream->file);
		FreeVec(stream);
	}
}

static LONG VORBIS_read(VORBIS_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
	LONG sample_offset = offset >> 2;
	LONG bytes_to_read = length;
	LONG bytes_read;
	char *dst = buffer;
	if (sample_offset >= stream->total_samples) {
		memset(buffer, 0, length);
		return length;
	}
	if (ov_pcm_tell(stream->vf) != sample_offset) {
		stream->eof = FALSE;
		if (ov_pcm_seek(stream->vf, sample_offset) < 0) {
			return 0;
		}
	}
	while (bytes_to_read > 0) {
		if (stream->eof) {
			memset(dst, 0, bytes_to_read);
			dst += bytes_to_read;
			break;
		}
		bytes_read = ov_read(stream->vf, dst, bytes_to_read, 0, 2, 1, &stream->bs);
		if (bytes_read < 0) {
			break;
		}
		if (bytes_read == 0) {
			stream->eof = TRUE;
		} else {
			dst += bytes_read;
			bytes_to_read -= bytes_read;
		}
	}
	return (LONG)dst - (LONG)buffer;
}

static size_t ogg_read (APTR buffer, size_t size, size_t nitems, APTR handle) {
	VORBIS_STREAM *stream = handle;
	size_t items_read;
	items_read = FRead(stream->file, buffer, size, nitems);
	return items_read == -1 ? 0 : items_read;
}

static int ogg_seek (APTR handle, ogg_int64_t offset, int mode) {
	VORBIS_STREAM *stream = handle;
	switch (mode) {
		case OGG_SEEK_SET:
			mode = OFFSET_BEGINNING;
			break;
		case OGG_SEEK_CUR:
			mode = OFFSET_CURRENT;
			break;
		case OGG_SEEK_END:
			mode = OFFSET_END;
			break;
		default:
			return -1;
	}
	return !ChangeFilePosition(stream->file, offset, mode) ? -1 : 0;
}

static long ogg_tell (APTR handle) {
	VORBIS_STREAM *stream = handle;
	return GetFilePosition(stream->file);
}

