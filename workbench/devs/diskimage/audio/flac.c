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

#include "audio/flac.h"
#include <libraries/iffparse.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0
#define ZERO MKBADDR(NULL)

#define ID_fLaC MAKE_ID('f','L','a','C')

BOOL FLAC_header(CONST_APTR p) {
	return rbe32(p) == ID_fLaC;
}

static void FLAC_close(FLAC_STREAM *stream);
static LONG FLAC_read(FLAC_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

static FLAC__StreamDecoderReadStatus FLAC_read_callback(const FLAC__StreamDecoder *decoder,
	FLAC__byte buffer[], size_t *bytes, void *client_data);
static FLAC__StreamDecoderSeekStatus FLAC_seek_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus FLAC_tell_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus FLAC_length_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool FLAC_eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
static FLAC__StreamDecoderWriteStatus FLAC_write_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void FLAC_metadata_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__StreamMetadata *metadata, void *client_data);
static void FLAC_error_callback(const FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC_STREAM *FLAC_open(CONST_STRPTR filename, BOOL cuesheet) {
	BPTR file = ZERO;
	FLAC_STREAM *stream = NULL;
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
	stream->close = FLAC_close;
	stream->read = FLAC_read;
	stream->file = file;

	stream->decoder = FLAC__stream_decoder_new();
	if (!stream->decoder) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (cuesheet) {
		FLAC__stream_decoder_set_metadata_respond(stream->decoder, FLAC__METADATA_TYPE_CUESHEET);
	}

	stream->init_status = FLAC__stream_decoder_init_stream(stream->decoder,
		FLAC_read_callback,
		FLAC_seek_callback,
		FLAC_tell_callback,
		FLAC_length_callback,
		FLAC_eof_callback,
		FLAC_write_callback,
		FLAC_metadata_callback,
		FLAC_error_callback,
		stream);
	if (stream->init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	if (!FLAC__stream_decoder_process_until_end_of_metadata(stream->decoder)) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	if (stream->total_samples == 0 || stream->sample_rate != 44100 ||
		stream->channels != 2 || stream->bits_per_sample != 16)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	return stream;

error:
	FLAC_close(stream);
	SetIoErr(error);
	return NULL;
}

static void FLAC_close(FLAC_STREAM *stream) {
	if (stream) {
		if (stream->decoder) {
			if (stream->cuesheet) {
				FLAC__metadata_object_delete(stream->cuesheet);
			}
			if (stream->init_status == FLAC__STREAM_DECODER_INIT_STATUS_OK) {
				FLAC__stream_decoder_finish(stream->decoder);
			}
			FLAC__stream_decoder_delete(stream->decoder);
		}
		Close(stream->file);
		FreeVec(stream);
	}
}

static LONG FLAC_read(FLAC_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
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
			samples_to_read -= samples_read;
		}
	}
	if (stream->sample_offset == -1 || sample_offset != stream->sample_offset) {
		stream->sample_offset = -1;
		stream->eof = FALSE;
		if (!FLAC__stream_decoder_seek_absolute(stream->decoder, sample_offset)) {
			return (int32)dst - (int32)buffer;
		}
		stream->sample_offset = sample_offset;
	}
	while (samples_to_read) {
		if (stream->eof) {
			memset(dst, 0, samples_to_read << 2);
			dst += (samples_to_read << 1);
			break;
		}
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
				samples_to_read -= samples_read;
			}
		}
		if (!FLAC__stream_decoder_process_single(stream->decoder)) {
			if (FLAC__stream_decoder_get_state(stream->decoder) == FLAC__STREAM_DECODER_END_OF_STREAM) {
				stream->eof = TRUE;
				continue;
			}
			stream->sample_offset = -1;
			break;
		}
	}
	return (LONG)dst - (LONG)buffer;
}

static FLAC__StreamDecoderReadStatus FLAC_read_callback(const FLAC__StreamDecoder *decoder,
	FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	FLAC_STREAM *stream = client_data;
	BPTR file = stream->file;
	if (*bytes > 0) {
		*bytes = FRead(file, buffer, 1, *bytes);
		if (*bytes == -1)
			return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
		else if (*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	} else
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

static FLAC__StreamDecoderSeekStatus FLAC_seek_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 absolute_byte_offset, void *client_data)
{
	FLAC_STREAM *stream = client_data;
	BPTR file = stream->file;
	if (!ChangeFilePosition(file, absolute_byte_offset, OFFSET_BEGINNING))
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}

static FLAC__StreamDecoderTellStatus FLAC_tell_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	FLAC_STREAM *stream = client_data;
	BPTR file = stream->file;
	QUAD pos;
	if ((pos = GetFilePosition(file)) == -1 && IoErr())
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

static FLAC__StreamDecoderLengthStatus FLAC_length_callback(const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *stream_length, void *client_data)
{
	FLAC_STREAM *stream = client_data;
	BPTR file = stream->file;
	QUAD size;
	if ((size = GetFileSize(file)) == -1 && IoErr())
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	else {
		*stream_length = (FLAC__uint64)size;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

static FLAC__bool FLAC_eof_callback(const FLAC__StreamDecoder *decoder, void *client_data) {
	FLAC_STREAM *stream = client_data;
	BPTR file = stream->file;
	int byte;
	if ((byte = FGetC(file)) == -1)
		return TRUE;
	else {
		UnGetC(file, byte);
		return FALSE;
	}
}

static FLAC__StreamDecoderWriteStatus FLAC_write_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	FLAC_STREAM *stream = client_data;
	LONG sample;
	if (stream->total_samples == 0 || stream->sample_rate != 44100 ||
		stream->channels != 2 || stream->bits_per_sample != 16)
	{
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}
	if (frame->header.blocksize > stream->pcm_size) {
		FreeVec(stream->pcm[0]);
		FreeVec(stream->pcm[1]);
		stream->pcm_size = frame->header.blocksize;
		stream->pcm[0] = AllocVec(stream->pcm_size * sizeof(int16), MEMF_ANY);
		stream->pcm[1] = AllocVec(stream->pcm_size * sizeof(int16), MEMF_ANY);
		if (!stream->pcm[0] || !stream->pcm[1]) {
			FreeVec(stream->pcm[0]);
			FreeVec(stream->pcm[1]);
			stream->pcm[0] = stream->pcm[1] = NULL;
			stream->pcm_size = 0;
			stream->sample_offset = -1;
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}
	stream->first_sample_in_pcm = frame->header.number.sample_number;
	stream->samples_in_pcm = frame->header.blocksize;
	for (sample = 0; sample < stream->samples_in_pcm; sample++) {
		stream->pcm[0][sample] = (int16)buffer[0][sample];
		stream->pcm[1][sample] = (int16)buffer[1][sample];
	}
	stream->sample_offset = stream->first_sample_in_pcm + stream->samples_in_pcm;
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void FLAC_metadata_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__StreamMetadata *metadata, void *client_data)
{
	FLAC_STREAM *stream = client_data;
	switch (metadata->type) {
		case FLAC__METADATA_TYPE_STREAMINFO:
			stream->total_samples = metadata->data.stream_info.total_samples;
			stream->sample_rate = metadata->data.stream_info.sample_rate;
			stream->channels = metadata->data.stream_info.channels;
			stream->bits_per_sample = metadata->data.stream_info.bits_per_sample;
			stream->length = stream->total_samples << 2;
			break;
		case FLAC__METADATA_TYPE_CUESHEET:
			stream->cuesheet = FLAC__metadata_object_clone(metadata);
			break;
		default:
			break;
	}
}

static void FLAC_error_callback(const FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status, void *client_data)
{
}
