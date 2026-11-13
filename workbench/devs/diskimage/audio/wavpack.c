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

#include "audio/wavpack.h"
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <string.h>
#include <stdio.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0
#define ZERO MKBADDR(NULL)
#define WAVPACK_SEEK_SET SEEK_SET
#define WAVPACK_SEEK_CUR SEEK_CUR
#define WAVPACK_SEEK_END SEEK_END

#define ID_wvpk MAKE_ID('w','v','p','k')

#define PCM_BUFFER_SIZE 588

BOOL WAVPACK_header(CONST_APTR p) {
	return rbe32(p) == ID_wvpk;
}

static int32_t WAVPACK_read_bytes(void *id, void *data, int32_t bcount);
static uint32_t WAVPACK_get_pos(void *id);
static int WAVPACK_set_pos_abs(void *id, uint32_t pos);
static int WAVPACK_set_pos_rel(void *id, int32_t delta, int mode);
static int WAVPACK_push_back_byte(void *id, int c);
static uint32_t WAVPACK_get_length(void *id);
static int WAVPACK_can_seek(void *id);
static int32_t WAVPACK_write_bytes(void *id, void *data, int32_t bcount);

static const WavpackStreamReader reader = {
	WAVPACK_read_bytes,
	WAVPACK_get_pos,
	WAVPACK_set_pos_abs,
	WAVPACK_set_pos_rel,
	WAVPACK_push_back_byte,
	WAVPACK_get_length,
	WAVPACK_can_seek,
	WAVPACK_write_bytes
};

static LONG WAVPACK_get_wvc_filename (BPTR file, STRPTR buffer, LONG size);

static void WAVPACK_close(WAVPACK_STREAM *stream);
static LONG WAVPACK_read(WAVPACK_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

WAVPACK_STREAM *WAVPACK_open(CONST_STRPTR wv_filename, BOOL cuesheet) {
	WAVPACK_STREAM *stream = NULL;
	TEXT wvc_filename[512];
	TEXT error_buffer[80];
	LONG error = NO_ERROR;
	int flags = 0;
	int mode;
	int channels;
	uint32_t sample_rate;
	int bits_per_sample;

	stream = AllocVec(sizeof(*stream), MEMF_CLEAR);
	if (!stream) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	stream->close = WAVPACK_close;
	stream->read = WAVPACK_read;

	stream->wv_file = Open(wv_filename, MODE_OLDFILE);
	if (!stream->wv_file) {
		error = IoErr();
		goto error;
	}
	error = WAVPACK_get_wvc_filename(stream->wv_file, wvc_filename, sizeof(wvc_filename));
	if (error != NO_ERROR) goto error;
	stream->wvc_file = Open(wvc_filename, MODE_OLDFILE);
	if (stream->wvc_file) {
		flags |= OPEN_WVC;
	}
	if (cuesheet) {
		flags |= OPEN_TAGS;
	}

	stream->wpc = WavpackOpenFileInputEx((WavpackStreamReader *)&reader,
		(APTR)stream->wv_file, (APTR)stream->wvc_file, error_buffer, flags, 0);
	if (!stream->wpc) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	mode = WavpackGetMode(stream->wpc);
	channels = WavpackGetNumChannels(stream->wpc);
	sample_rate = WavpackGetSampleRate(stream->wpc);
	bits_per_sample = WavpackGetBitsPerSample(stream->wpc);
	stream->total_samples = WavpackGetNumSamples(stream->wpc);
	if ((mode & MODE_FLOAT) || channels != 2 || sample_rate != 44100 ||
		bits_per_sample != 16 || stream->total_samples <= 0)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	stream->length = stream->total_samples << 2;

	stream->pcm = AllocVec(PCM_BUFFER_SIZE * 2 * sizeof(int32_t), MEMF_ANY);
	if (!stream->pcm) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	return stream;

error:
	WAVPACK_close(stream);
	SetIoErr(error);
	return NULL;
}

static void strip_file_extension (STRPTR name, CONST_STRPTR ext) {
	int32 len = strlen(name) - strlen(ext);
	if (len >= 0) {
		name += len;
		if (!Stricmp(name, ext)) {
			*name = 0;
		}
	}
}

static LONG WAVPACK_get_wvc_filename (BPTR file, STRPTR buffer, LONG size) {
	struct FileInfoBlock *fib;
	int32 error = NO_ERROR;
	fib = AllocDosObject(DOS_FIB, NULL);
	if (fib) {
		if (ExamineFH(file, fib)) {
			Strlcpy(buffer, fib->fib_FileName, size);
			strip_file_extension(buffer, ".wv");
			Strlcat(buffer, ".wvc", size);
		} else {
			error = IoErr();
		}
		FreeDosObject(DOS_FIB, fib);
	} else {
		error = ERROR_NO_FREE_STORE;
	}
	return error;
}

static void WAVPACK_close(WAVPACK_STREAM *stream) {
	if (stream) {
		if (stream->wpc) {
			WavpackCloseFile(stream->wpc);
		}
		FreeVec(stream->pcm);
		Close(stream->wv_file);
		Close(stream->wvc_file);
		FreeVec(stream);
	}
}

static LONG WAVPACK_read(WAVPACK_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
	LONG sample_offset = offset >> 2;
	LONG samples_to_read = length >> 2;
	LONG samples_read;
	LONG sample;
	int32_t *src;
	WORD *dst = buffer;
	if (stream->wpc == NULL) {
		TEXT error_buffer[80];
		int flags = 0;
		if (stream->wvc_file) {
			flags |= OPEN_WVC;
		}
		stream->wpc = WavpackOpenFileInputEx((WavpackStreamReader *)&reader,
			(APTR)stream->wv_file, (APTR)stream->wvc_file, error_buffer, flags, 0);
		if (!stream->wpc) {
			return 0;
		}
	}
	if (WavpackGetSampleIndex(stream->wpc) != sample_offset) {
		if (!WavpackSeekSample(stream->wpc, sample_offset)) {
			WavpackCloseFile(stream->wpc);
			stream->wpc = NULL;
			return 0;
		}
	}
	while (samples_to_read) {
		if (samples_to_read < PCM_BUFFER_SIZE)
			samples_read = samples_to_read;
		else
			samples_read = PCM_BUFFER_SIZE;
		samples_read = WavpackUnpackSamples(stream->wpc, stream->pcm, samples_read);
		if (samples_read == 0) {
			break;
		}
		src = stream->pcm;
		for (sample = 0; sample < samples_read; sample++) {
			wle16(dst, (UWORD)*src); src++; dst++;
			wle16(dst, (UWORD)*src); src++; dst++;
		}
		samples_to_read -= samples_read;
	}
	memset(dst, 0, samples_to_read << 2);
	return length;
}

static int32_t WAVPACK_read_bytes(void *id, void *data, int32_t bcount) {
	BPTR file = (BPTR)id;
	return FRead(file, data, 1, bcount);
}

static uint32_t WAVPACK_get_pos(void *id) {
	BPTR file = (BPTR)id;
	return GetFilePosition(file);
}

static int WAVPACK_set_pos_abs(void *id, uint32_t pos) {
	BPTR file = (BPTR)id;
	return !ChangeFilePosition(file, pos, OFFSET_BEGINNING);
}

static int WAVPACK_set_pos_rel(void *id, int32_t delta, int mode) {
	BPTR file = (BPTR)id;
	switch (mode) {
		case WAVPACK_SEEK_SET: return !ChangeFilePosition(file, delta, OFFSET_BEGINNING);
		case WAVPACK_SEEK_CUR: return !ChangeFilePosition(file, delta, OFFSET_CURRENT);
		case WAVPACK_SEEK_END: return !ChangeFilePosition(file, delta, OFFSET_END);
		default: return !0;
	}
}

static int WAVPACK_push_back_byte(void *id, int c) {
	BPTR file = (BPTR)id;
	if (c == -1) return -1;
	SetIoErr(NO_ERROR);
	if (UnGetC(file, c) != c || (c == FALSE && IoErr())) {
		return -1;
	}
	return c;
}

static uint32_t WAVPACK_get_length(void *id) {
	BPTR file = (BPTR)id;
	QUAD size;
	if (file == ZERO) return 0;
	if ((size = GetFileSize(file)) == -1 && IoErr()) {
		return 0;
	}
	return size;
}

static int WAVPACK_can_seek(void *id) {
	BPTR file = (BPTR)id;
	if (file == ZERO) return FALSE;
	return TRUE;
}

static int32_t WAVPACK_write_bytes(void *id, void *data, int32_t bcount) {
	return 0;
}
