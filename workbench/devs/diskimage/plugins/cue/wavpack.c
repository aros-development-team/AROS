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
#include <string.h>
#include "audio/wavpack.h"
#include "cue.h"

BOOL CUE_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL WavPack_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR WavPack_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CUE_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CUE_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CUE_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void CUE_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG CUE_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin wavpack_plugin = {
	PLUGIN_NODE(0, "WavPack"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	CUE_Init,
	NULL,
	WavPack_CheckImage,
	WavPack_OpenImage,
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

BOOL WavPack_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 4 && WAVPACK_header(test);
}

LONG cue_parse_args (STRPTR cmd, STRPTR *argv, LONG *argc_ptr);
LONG cue_parse_index (CONST_STRPTR index_str, ULONG *index);
LONG cue_fix_track_indexes (struct CUEImage *image);
char *strtok_r (char *str, const char *sep_set, char **state_ptr);

APTR WavPack_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CUEImage *image = NULL;
	struct CUEFile *cue_file = NULL;
	struct CUETrack *track = NULL;
	struct CUETrack *prev_track = NULL;
	ULONG cue_len;
	char *cuesheet = NULL;
	char *cuesheet_start = NULL;
	char *line, *last_line = NULL;
	LONG argc;
	STRPTR argv[MAX_ARGS];
	LONG track_num, index_type;
	ULONG index;
	WAVPACK_STREAM *stream;

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
	cue_file->type = FILE_WAVPACK;
	cue_file->f.aud.stream = (AUDIO_STREAM *)(stream = WAVPACK_open(name, TRUE));
	if (!stream) {
		error = IoErr();
		goto error;
	}

	if ((cue_len = WavpackGetTagItem(stream->wpc, "cuesheet", NULL, 0))) {
		cuesheet = AllocVec(cue_len + 1, MEMF_ANY);
		if (!cuesheet) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}
		WavpackGetTagItem(stream->wpc, "cuesheet", cuesheet, cue_len);
		cuesheet[cue_len] = 0;
		cuesheet_start = cuesheet;
	} else if ((cue_len = WavpackGetBinaryTagItem(stream->wpc, "cuesheet", NULL, 0))) {
		cuesheet = AllocVec(cue_len + 1, MEMF_ANY);
		if (!cuesheet) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}
		WavpackGetBinaryTagItem(stream->wpc, "cuesheet", cuesheet, cue_len);
		cuesheet[cue_len] = 0;
		cuesheet_start = cuesheet + strlen(cuesheet) + 1;
	} else {
		error = ERROR_REQUIRED_ARG_MISSING;
		goto error;
	}
	line = strtok_r(cuesheet_start, "\n", &last_line);
	while (line) {
		error = cue_parse_args(line, argv, &argc);
		if (error != NO_ERROR) break;
		if (argc > 0) {
			if (!strcmp(argv[0], "TRACK")) {
				if (argc < 3) {
					error = ERROR_REQUIRED_ARG_MISSING;
					break;
				}
				if (argc > 3) {
					error = ERROR_TOO_MANY_ARGS;
					break;
				}
				if (track) {
					prev_track = track;
				}
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
				if (StrToLong(argv[1], &track_num) == -1) {
					error = ERROR_BAD_NUMBER;
					break;
				}
				track->track_num = track_num;
				if (!image->first_track || track_num < image->first_track) {
					image->first_track = track_num;
				}
				if (!image->last_track || track_num > image->last_track) {
					image->last_track = track_num;
				}
				if (!strcmp(argv[2], "MODE1/2048")) {
					track->audio = FALSE;
					track->sector_size = 2048;
					track->sync_size = 0;
				} else if (!strcmp(argv[2], "MODE1/2352")) {
					track->audio = FALSE;
					track->sector_size = 2352;
					track->sync_size = 16;
				} else if (!strcmp(argv[2], "MODE2/2352")) {
					track->audio = FALSE;
					track->sector_size = 2352;
					track->sync_size = 24;
				} else if (!strcmp(argv[2], "AUDIO")) {
					track->audio = TRUE;
					track->sector_size = 2352;
					track->sync_size = 16;
				} else {
					error = ERROR_NOT_IMPLEMENTED;
					break;
				}
				track->index0 = -1;
				track->index1 = -1;
				track->index2 = -1;
			} else if (!strcmp(argv[0], "INDEX")) {
				if (!track) {
					error = ERROR_REQUIRED_ARG_MISSING;
					break;
				}
				if (argc < 3) {
					error = ERROR_REQUIRED_ARG_MISSING;
					break;
				}
				if (argc > 3) {
					error = ERROR_TOO_MANY_ARGS;
					break;
				}
				if (StrToLong(argv[1], &index_type) == -1) {
					error = ERROR_BAD_NUMBER;
					break;
				}
				error = cue_parse_index(argv[2], &index);
				if (error != NO_ERROR) break;
				switch (index_type) {
					case 0:
						if (prev_track && prev_track->index2 == -1) {
							prev_track->index2 = index;
						}
						track->index0 = index;
						break;
					case 1:
						if (prev_track && prev_track->index2 == -1) {
							prev_track->index2 = index;
						}
						track->index1 = index;
						break;
				}
			}
		}
		line = strtok_r(NULL, "\n", &last_line);
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
	FreeVec(cuesheet);
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

char *strtok_r (char *str, const char *sep_set, char **state_ptr) {
	char *result = NULL;
	char *last = *state_ptr;
	size_t size;
	if (str == NULL) {
		str = last;
		if (str == NULL) goto out;
	}
	last = NULL;
	str += strspn(str, sep_set);
	if (*str == 0) goto out;
	size = strcspn(str, sep_set);
	if (size == 0) goto out;
	last = &str[size];
	if (*last != 0) last++;
	str[size] = 0;
	result = str;
out:
	*state_ptr = last;
	return result;
}
