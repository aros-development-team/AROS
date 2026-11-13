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

#include "audio/mp3_mpg123.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <mpg123.h>
#include <stdio.h>
#include "endian.h"
#include "support.h"

#define NO_ERROR 0
#define ZERO MKBADDR(NULL)
#define MPG123_SEEK_SET SEEK_SET
#define MPG123_SEEK_CUR SEEK_CUR
#define MPG123_SEEK_END SEEK_END

static void MP3_close(MP3_STREAM *stream);
static LONG MP3_read(MP3_STREAM *stream, APTR buffer, ULONG offset, ULONG length);

ssize_t mpg123_r_read(void *handle, void *buffer, size_t length);
off_t mpg123_r_lseek(void *handle, off_t offset, int whence);
void mpg123_cleanup(void *handle);

MP3_STREAM *MP3_open(CONST_STRPTR filename) {
	BPTR file = ZERO;
	MP3_STREAM *stream = NULL;
	LONG error = NO_ERROR;

	file = Open(filename, MODE_OLDFILE);
	if (!file) {
		error = IoErr();
		goto error;
	}

	stream = AllocVec(sizeof(*stream), MEMF_ANY|MEMF_CLEAR);
	if (!stream) {
		error = ERROR_NO_FREE_STORE;
		Close(file);
		goto error;
	}
	stream->close = MP3_close;
	stream->read = MP3_read;
	stream->file = file;
	
	stream->mh = mpg123_new(NULL, NULL);
	if (!stream->mh) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	
	if (mpg123_format_none(stream->mh) != MPG123_OK ||
		mpg123_format(stream->mh, 44100, MPG123_STEREO,
		MPG123_ENC_SIGNED_16) != MPG123_OK ||
		mpg123_replace_reader_handle(stream->mh, mpg123_r_read,
		mpg123_r_lseek, mpg123_cleanup) != MPG123_OK ||
		mpg123_open_handle(stream->mh, stream) != MPG123_OK)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	stream->total_samples = mpg123_length(stream->mh);
	if (stream->total_samples == MPG123_ERR) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	stream->length = stream->total_samples << 2;

	return stream;

error:
	MP3_close(stream);
	SetIoErr(error);
	return NULL;
}

static void MP3_close(MP3_STREAM *stream) {
	if (stream) {
		mpg123_delete(stream->mh);
		Close(stream->file);
		FreeVec(stream);
	}
}

static LONG MP3_read(MP3_STREAM *stream, APTR buffer, ULONG offset, ULONG length) {
	LONG sample_offset = offset >> 2;
	LONG samples_to_read = length >> 2;
	LONG bytes_read;
	LONG samples_read;
	WORD *dst = buffer;
	int res;
	if (sample_offset >= stream->total_samples) {
		memset(buffer, 0, length);
		return length;
	}
	if (mpg123_tell(stream->mh) != sample_offset) {
		LONG curr_sample_offset;
		stream->eof = FALSE;
		curr_sample_offset = mpg123_seek(stream->mh, sample_offset,
			MPG123_SEEK_SET);
		if (curr_sample_offset < 0) {
			return 0;
		} else if (curr_sample_offset < sample_offset) {
			LONG seek_len, do_len;
			seek_len = (sample_offset - curr_sample_offset) << 2;
			do_len = length;
			while (seek_len > 0) {
				if (seek_len < length) do_len = seek_len;
				res = mpg123_read(stream->mh, buffer, do_len, &bytes_read);
				if (bytes_read > 0) {
					seek_len -= bytes_read;
				}
				if (res == MPG123_DONE) {
					stream->eof = TRUE;
					break;
				}
				if (res != MPG123_OK) {
					return 0;
				}
			}
		} else if (curr_sample_offset > sample_offset) {
			return 0;
		}
	}
	while (samples_to_read) {
		if (stream->eof) {
			memset(dst, 0, samples_to_read << 2);
			dst += (samples_to_read << 1);
			break;
		}
		res = mpg123_read(stream->mh, (UBYTE *)dst, samples_to_read << 2, &bytes_read);
		samples_read = bytes_read >> 2;
		if (samples_read > 0) {
			LONG sample;
			for (sample = 0; sample < samples_read; sample++) {
				wle16(dst, *dst); dst++;
				wle16(dst, *dst); dst++;
			}
			samples_to_read -= samples_read;
		}
		if (res == MPG123_DONE) {
			stream->eof = TRUE;
			continue;
		}
		if (res != MPG123_OK && res != MPG123_NEW_FORMAT) {
			break;
		}
	}
	return (IPTR)dst - (IPTR)buffer;
}

ssize_t mpg123_r_read(void *handle, void *buffer, size_t length) {
	MP3_STREAM *stream = handle;
	BPTR file = stream->file;
	return Read(file, buffer, length);
}

off_t mpg123_r_lseek(void *handle, off_t offset, int whence) {
	MP3_STREAM *stream = handle;
	BPTR file = stream->file;
	LONG mode;
	int64 res;
	switch (whence) {
		case MPG123_SEEK_SET:
			mode = OFFSET_BEGINNING;
			break;
		case MPG123_SEEK_CUR:
			mode = OFFSET_CURRENT;
			break;
		case MPG123_SEEK_END:
			mode = OFFSET_END;
			break;
		default:
			return (off_t)-1;
	}
	if (!ChangeFilePosition(file, offset, mode)) {
		return (off_t)-1;
	}
	res = GetFilePosition(file);
	if (res >= 0LL && res <= 0x7fffffffLL) {
		return res;
	} else {
		return (off_t)-1;
	}
}

void mpg123_cleanup(void *handle) {
	/* do nothing */
}

