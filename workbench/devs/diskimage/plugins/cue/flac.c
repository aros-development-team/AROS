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

#define USED_PLUGIN_API_VERSION 8
#include <devices/diskimage.h>
#include <interfaces/diplugin.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include "device_locale.h"
#include "scsicmd.h"
#include "audio/flac.h"
#include "cue.h"
#include "rev/diskimage.device_rev.h"

BOOL CUE_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL FLAC_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR FLAC_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CUE_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CUE_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CUE_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void CUE_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG CUE_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin flac_plugin = {
	PLUGIN_NODE(0, "FLAC"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	CUE_Init,
	NULL,
	FLAC_CheckImage,
	FLAC_OpenImage,
	CUE_CloseImage,
	CUE_Geometry,
	CUE_Read,
	NULL,
	NULL,
	NULL,
	CUE_GetCDTracks,
	CUE_ReadCDDA
};

extern struct Library *SysBase;
extern struct Library *DOSBase;
extern struct DIPluginIFace *IPlugin;

BOOL FLAC_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 4 && FLAC_header(test);
}

LONG cue_fix_track_indexes (struct CUEImage *image);

APTR FLAC_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CUEImage *image = NULL;
	struct CUEFile *cue_file = NULL;
	struct CUETrack *track = NULL;
	struct CUETrack *prev_track = NULL;
	LONG track_num, index_type;
	ULONG index;
	LONG i, j;
	FLAC_STREAM *stream;
	FLAC__StreamMetadata_CueSheet *cuesheet;
	FLAC__StreamMetadata_CueSheet_Track *flac_track;
	FLAC__StreamMetadata_CueSheet_Index *flac_index;

	Close(file);

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->files = cue_file = AllocVec(sizeof(*cue_file), MEMF_CLEAR);
	if (!cue_file) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	cue_file->type = FILE_FLAC;
	cue_file->f.aud.stream = (AUDIO_STREAM *)(stream = FLAC_open(name, TRUE));
	if (!stream) {
		error = IoErr();
		goto error;
	}
	if (!stream->cuesheet) {
		error = ERROR_REQUIRED_ARG_MISSING;
		goto error;
	}

	cuesheet = &stream->cuesheet->data.cue_sheet;
	for (i = 0; i < cuesheet->num_tracks; i++) {
		flac_track = &cuesheet->tracks[i];
		track_num = flac_track->number;
		if (track_num >= 0xA0) {
			continue;
		}
		prev_track = track;
		if (!track) {
			track = image->tracks = AllocVec(sizeof(*track), MEMF_CLEAR);
		} else {
			track = track->next = AllocVec(sizeof(*track), MEMF_CLEAR);
		}
		if (!track) {
			error = ERROR_NO_FREE_STORE;
			break;
		}
		track->file = cue_file;
		cue_file->refcount++;
		track->track_num = track_num;
		if (!image->first_track || track_num < image->first_track) {
			image->first_track = track_num;
		}
		if (!image->last_track || track_num > image->last_track) {
			image->last_track = track_num;
		}
#if defined(__GNUC__)
		track->audio = !flac_track->type;
#else
		track->audio = !(flac_track->bitfield & 0x80);
#endif
		track->sector_size = 2352;
		track->sync_size = 16;
		index = flac_track->offset / SAMPLESPERFRAME;
		if (prev_track && prev_track->index2 == -1) {
			prev_track->index2 = index;
		}
		track->index0 = index;
		track->index1 = index;
		track->index2 = -1;
		for (j = 0; j < flac_track->num_indices; j++) {
			flac_index = &flac_track->indices[j];
			index = flac_index->offset / SAMPLESPERFRAME;
			index_type = flac_index->number;
			switch (index_type) {
				case 0:
					track->index0 = track->index0 + index;
					break;
				case 1:
					track->index1 = track->index0 + index;
					break;
			}
		}
	}
	if (error != NO_ERROR) {
		goto error;
	}

	if (!image->tracks) {
		error = ERROR_REQUIRED_ARG_MISSING;
		goto error;
	}

	if ((error = cue_fix_track_indexes(image)) != NO_ERROR) {
		goto error;
	}
	
	done = TRUE;

error:
	if (!done) {
		Plugin_CloseImage(Self, image);
		image = NULL;
		if (error == NO_ERROR) {
			error = ERROR_OBJECT_WRONG_TYPE;
			error_string = MSG_EOF;
		}
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}
