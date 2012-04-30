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

#ifndef AUDIO_WAVPACK_H
#define AUDIO_WAVPACK_H

#ifndef DEVICES_DISKIMAGE_H
#include <devices/diskimage.h>
#endif
#ifndef WAVPACK_H
#include <wavpack.h>
#endif

typedef struct _WAVPACK_STREAM {
	ULONG length;
	void (*close)(struct _WAVPACK_STREAM *stream);
	LONG (*read)(struct _WAVPACK_STREAM *stream, APTR buffer, ULONG offset, ULONG length);
	BPTR wv_file;
	BPTR wvc_file;
	WavpackContext *wpc;
	LONG total_samples;
	int32_t *pcm;
} WAVPACK_STREAM;

BOOL WAVPACK_header(CONST_APTR p);
WAVPACK_STREAM *WAVPACK_open(CONST_STRPTR wv_filename, BOOL cuesheet);

#endif
