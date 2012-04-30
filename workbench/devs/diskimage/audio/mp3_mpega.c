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

#include "audio/mp3_mpega.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/mpega.h>
#include <string.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0

static const MPEGA_CTRL mpa_ctrl = {
	NULL,
	{ FALSE, { 1, 2, 44100 }, { 1, 2, 44100 } },
	{ FALSE, { 1, 2, 44100 }, { 1, 2, 44100 } },
	0,
	0
};

static void MP3_close(MP3_STREAM *stream);
static LONG MP3_read(MP3_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

MP3_STREAM *MP3_open(CONST_STRPTR filename) {
	MP3_STREAM *stream = NULL;
	LONG error = NO_ERROR;

	stream = AllocVec(sizeof(*stream), MEMF_CLEAR);
	if (!stream) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	stream->close = MP3_close;
	stream->read = MP3_read;

	stream->stream = MPEGA_open((STRPTR)filename, (MPEGA_CTRL *)&mpa_ctrl);
	if (!stream->stream) {
		error = ERROR_OBJECT_NOT_FOUND;
		goto error;
	}

	if (stream->stream->dec_frequency != 44100 ||
		stream->stream->dec_channels != 2)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	stream->pcm[0] = AllocVec(MPEGA_PCM_SIZE * sizeof(WORD), MEMF_ANY);
	stream->pcm[1] = AllocVec(MPEGA_PCM_SIZE * sizeof(WORD), MEMF_ANY);
	if (!stream->pcm[0] || !stream->pcm[1]) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	stream->length = ((UQUAD)stream->stream->ms_duration * (44100ULL * 4ULL)) / 1000ULL;

	return stream;

error:
	MP3_close(stream);
	SetIoErr(error);
	return NULL;
}

static void MP3_close(MP3_STREAM *stream) {
	if (stream) {
		if (stream->stream) MPEGA_close(stream->stream);
		FreeVec(stream->pcm[0]);
		FreeVec(stream->pcm[1]);
		FreeVec(stream);
	}
}

static LONG MP3_read(MP3_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
	LONG sample_offset = offset >> 2;
	LONG samples_to_read = length >> 2;
	LONG samples_to_skip;
	LONG samples_read;
	LONG sample;
	WORD *dst = buffer;
	if (sample_offset >= stream->first_sample_in_pcm &&
		sample_offset < (stream->first_sample_in_pcm + stream->samples_in_pcm))
	{
		samples_to_skip = sample_offset - stream->first_sample_in_pcm;
		samples_read = stream->samples_in_pcm;
		sample_offset = stream->first_sample_in_pcm + samples_read;
		if (samples_read > samples_to_skip) {
			if (samples_read > (samples_to_skip + samples_to_read)) {
				samples_read = samples_to_skip + samples_to_read;
			}
			for (sample = samples_to_skip; sample < samples_read; sample++) {
				wle16(dst, stream->pcm[0][sample]); dst++;
				wle16(dst, stream->pcm[1][sample]); dst++;
			}
			samples_read -= samples_to_skip;
			samples_to_skip = 0;
			samples_to_read -= samples_read;
		} else {
			samples_to_skip -= samples_read;
		}
	}
	if (stream->sample_offset != -1 && sample_offset >= stream->sample_offset) {
		samples_to_skip = sample_offset - stream->sample_offset;
		sample_offset = stream->sample_offset;
	} else {
		stream->sample_offset = -1;
		stream->eof = FALSE;
		if (MPEGA_seek(stream->stream, 0) < 0) {
			return (LONG)dst - (LONG)buffer;
		}
		samples_to_skip = sample_offset;
		stream->sample_offset = sample_offset = 0;
	}
	while (samples_to_read) {
		if (stream->eof) {
			memset(dst, 0, samples_to_read << 2);
			dst += (samples_to_read << 1);
			break;
		}
		stream->first_sample_in_pcm = 0;
		stream->samples_in_pcm = 0;
		samples_read = MPEGA_decode_frame(stream->stream, stream->pcm);
		if (samples_read < 0) {
			if (samples_read == MPEGA_ERR_EOF) {
				stream->eof = TRUE;
				continue;
			}
			stream->sample_offset = -1;
			break;
		}
		stream->first_sample_in_pcm = sample_offset;
		stream->samples_in_pcm = samples_read;
		sample_offset += samples_read;
		stream->sample_offset = sample_offset;
		if (samples_read > samples_to_skip) {
			if (samples_read > (samples_to_skip + samples_to_read)) {
				samples_read = samples_to_skip + samples_to_read;
			}
			for (sample = samples_to_skip; sample < samples_read; sample++) {
				wle16(dst, stream->pcm[0][sample]); dst++;
				wle16(dst, stream->pcm[1][sample]); dst++;
			}
			samples_read -= samples_to_skip;
			samples_to_skip = 0;
			samples_to_read -= samples_read;
		} else {
			samples_to_skip -= samples_read;
		}
	}
	return (LONG)dst - (LONG)buffer;
}
