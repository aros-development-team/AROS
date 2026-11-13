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

#ifndef AUDIO_FLAC_H
#define AUDIO_FLAC_H

#ifndef DEVICES_DISKIMAGE_H
#include <devices/diskimage.h>
#endif
#ifndef FLAC__ALL_H
#include <FLAC/all.h>
#endif

typedef struct _FLAC_STREAM {
	ULONG length;
	void (*close)(struct _FLAC_STREAM *stream);
	LONG (*read)(struct _FLAC_STREAM *stream, APTR buffer, ULONG offset, ULONG length);
	BPTR file;
	FLAC__StreamDecoder *decoder;
	FLAC__StreamDecoderInitStatus init_status;
	FLAC__StreamMetadata *cuesheet;
	LONG total_samples;
	LONG sample_rate;
	LONG channels;
	LONG bits_per_sample;
	WORD *pcm[2];
	LONG pcm_size;
	LONG first_sample_in_pcm;
	LONG samples_in_pcm;
	LONG sample_offset;
	BOOL eof;
} FLAC_STREAM;

BOOL FLAC_header(CONST_APTR p);
FLAC_STREAM *FLAC_open(CONST_STRPTR filename, BOOL cuesheet);

#endif
