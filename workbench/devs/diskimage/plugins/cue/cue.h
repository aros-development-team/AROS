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

#ifndef CUE_H
#define CUE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#define MAX_ARGS 32
#define LINE_BUFFER_SIZE 512

typedef struct _AUDIO_STREAM {
	ULONG length;
	void (*close)(struct _AUDIO_STREAM *stream);
	LONG (*read)(struct _AUDIO_STREAM *stream, APTR buffer, ULONG offset, ULONG length);
} AUDIO_STREAM;

#define AUDIO_close(stream) (stream)->close(stream)
#define AUDIO_read(stream,buffer,offset,length) (stream)->read(stream,buffer,offset,length)

struct CUEFile {
	struct CUEFile *next;
	LONG type;
	LONG refcount;
	union {
		struct {
			BPTR file;
			QUAD file_size;
		} bin;
		struct {
			AUDIO_STREAM *stream;
		} aud;
	} f;
};

enum {
	FILE_BINARY,
	FILE_MOTOROLA,
	FILE_AIFF,
	FILE_FLAC,
	FILE_MP3,
	FILE_VORBIS,
	FILE_WAVE,
	FILE_WAVPACK
};

struct CUETrack {
	struct CUETrack *next;
	UBYTE track_num;
	UBYTE audio;
	UWORD sector_size;
	UBYTE sync_size;
	UQUAD offset;
	UQUAD length;
	UQUAD sectors;
	struct CUEFile *file;
	LONG index0;
	LONG index1;
	LONG index2;
};

struct CUEImage {
	struct CUEFile *files;
	UBYTE first_track;
	UBYTE last_track;
	UBYTE num_tracks;
#ifndef USE_MPG123
	UBYTE uses_mp3;
#endif
	struct CUETrack *tracks;
	ULONG block_size;
	ULONG total_blocks;
#ifndef USE_MPG123
	WORD *pcm[MPEGA_MAX_CHANNELS];
	struct CUEFile *file_in_pcm;
	LONG first_sample_in_pcm;
	LONG samples_in_pcm;
	struct Library *mpegabase;
#endif
	UBYTE *out_buf;
	ULONG out_size;
};

#endif
