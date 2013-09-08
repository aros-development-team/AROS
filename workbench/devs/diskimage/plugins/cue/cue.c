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
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#ifndef USE_MPG123
#include <libraries/mpega.h>
#include <proto/mpega.h>
#endif
#include "endian.h"
#include "device_locale.h"
#include "scsicmd.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <SDI_compiler.h>
#include "audio/aiff.h"
#ifdef USE_FLAC
#include "audio/flac.h"
#endif
#ifdef USE_MPG123
#include "audio/mp3_mpg123.h"
#else
#include "audio/mp3_mpega.h"
#endif
#ifdef USE_VORBIS
#include "audio/vorbis.h"
#endif
#include "audio/wave.h"
#ifdef USE_WAVPACK
#include "audio/wavpack.h"
#endif
#include "cue.h"
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("CUE")

extern struct DiskImagePlugin cue_plugin;
#ifdef USE_FLAC
extern struct DiskImagePlugin flac_plugin;
#endif
#ifdef USE_WAVPACK
extern struct DiskImagePlugin wavpack_plugin;
#endif

#if !defined(USE_FLAC) && !defined(USE_WAVPACK)
PLUGIN_TABLE(&cue_plugin)
#else
PLUGIN_TABLE(&cue_plugin
#ifdef USE_FLAC
	, &flac_plugin
#endif
#ifdef USE_WAVPACK
	, &wavpack_plugin
#endif
	)
#endif

BOOL CUE_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
void CUE_Exit (struct DiskImagePlugin *Self);
BOOL CUE_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR CUE_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CUE_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CUE_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CUE_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void CUE_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG CUE_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin cue_plugin = {
	PLUGIN_NODE(0, "CUE"),
	PLUGIN_FLAG_M68K,
	64,
	ZERO,
	NULL,
	CUE_Init,
	CUE_Exit,
	CUE_CheckImage,
	CUE_OpenImage,
	CUE_CloseImage,
	CUE_Geometry,
	CUE_Read,
	NULL,
	NULL,
	NULL,
	CUE_GetCDTracks,
	CUE_ReadCDDA
};

struct Library *SysBase;
struct Library *DOSBase;
struct Library *UtilityBase;
struct DIPluginIFace *IPlugin;
#ifndef USE_MPG123
struct Library *MPEGABase;
#endif
#if defined(USE_MPG123) && defined(__AROS__)
struct Library *StdCBase;
#endif

#ifdef USE_MPG123
int malloc_init(void);
void malloc_exit(void);
#endif

BOOL CUE_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	UtilityBase = data->UtilityBase;
	IPlugin = data->IPlugin;
#ifdef USE_MPG123
#ifdef __AROS__
	StdCBase = data->StdCBase;
#endif
	if (!malloc_init()) {
		return FALSE;
	}
	if (mpg123_init() != MPG123_OK) {
		malloc_exit();
		return FALSE;
	}
#endif
	return TRUE;
}

void CUE_Exit (struct DiskImagePlugin *Self) {
#ifdef USE_MPG123
	mpg123_exit();
	malloc_exit();
#endif
}

BOOL CUE_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	int len;
	len = strlen(name)-4;
	if (len > 0) {
		name += len;
		return !Stricmp(name, ".cue") && IsText(test, testsize);
	}
	return FALSE;
}

#ifndef USE_MPG123
static const MPEGA_CTRL mpa_ctrl = {
	NULL,
	{ FALSE, { 1, 2, 44100 }, { 1, 2, 44100 } },
	{ FALSE, { 1, 2, 44100 }, { 1, 2, 44100 } },
	0,
	0
};
#endif

LONG cue_parse_args (STRPTR cmd, STRPTR *argv, int *argc_ptr);
LONG cue_parse_index (CONST_STRPTR index_str, ULONG *index);
LONG cue_fix_track_indexes (struct CUEImage *image);
static inline LONG get_file_header (CONST_STRPTR filename, APTR header, LONG size);

APTR CUE_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CUEImage *image = NULL;
	struct CUEFile *cue_file;
	struct CUETrack *track, *prev_track;
	STRPTR line_buffer;
	int argc;
	STRPTR argv[MAX_ARGS];
	LONG track_num, index_type;
	ULONG index;
	BPTR new_lock = ZERO, old_lock = ZERO;
	
	new_lock = ParentOfFH(file);
	if (new_lock) {
		old_lock = CurrentDir(new_lock);
	}

	line_buffer = AllocVec(LINE_BUFFER_SIZE, MEMF_ANY);
	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!line_buffer || !image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	cue_file = NULL;
	track = prev_track = NULL;
	while (FGets(file, line_buffer, LINE_BUFFER_SIZE)) {
		error = cue_parse_args(line_buffer, argv, &argc);
		if (error != NO_ERROR) break;
		if (argc > 0) {
			if (!strcmp(argv[0], "REM"));
			else if (!strcmp(argv[0], "FILE")) {
				prev_track = NULL;
				if (argc < 3) {
					error = ERROR_REQUIRED_ARG_MISSING;
					break;
				}
				if (argc > 3) {
					error = ERROR_TOO_MANY_ARGS;
					break;
				}
				if (!cue_file) {
					cue_file = image->files = AllocVec(sizeof(*cue_file), MEMF_CLEAR);
				} else {
					cue_file = cue_file->next = AllocVec(sizeof(*cue_file), MEMF_CLEAR);
				}
				if (!cue_file) {
					error = ERROR_NO_FREE_STORE;
					break;
				}
				if (!strcmp(argv[2], "BINARY") || !strcmp(argv[2], "MOTOROLA")) {
					cue_file->type = !strcmp(argv[2], "BINARY") ? FILE_BINARY : FILE_MOTOROLA;
					cue_file->f.bin.file = Open(argv[1], MODE_OLDFILE);
					if (!cue_file->f.bin.file) {
						error = IoErr();
						break;
					}
					cue_file->f.bin.file_size = GetFileSize(cue_file->f.bin.file);
					if (cue_file->f.bin.file_size == -1) {
						error = IoErr();
						break;
					}
				} else {
					UBYTE header[12];
					if (get_file_header(argv[1], header, sizeof(header)) != sizeof(header)) {
						error = IoErr();
						if (error == NO_ERROR) {
							error = ERROR_OBJECT_WRONG_TYPE;
							error_string = MSG_EOF;
						}
						break;
					}
					if (AIFF_header(header)) {
						cue_file->type = FILE_AIFF;
						cue_file->f.aud.stream = (AUDIO_STREAM *)AIFF_open(argv[1]);
#ifdef USE_FLAC
					} else if (FLAC_header(header)) {
						cue_file->type = FILE_FLAC;
						cue_file->f.aud.stream = (AUDIO_STREAM *)FLAC_open(argv[1], FALSE);
#endif
#ifdef USE_VORBIS
					} else if (VORBIS_header(header)) {
						cue_file->type = FILE_VORBIS;
						cue_file->f.aud.stream = (AUDIO_STREAM *)VORBIS_open(argv[1]);
#endif
					} else if (strcmp(argv[2], "MP3") && WAVE_header(header)) {
						cue_file->type = FILE_WAVE;
						cue_file->f.aud.stream = (AUDIO_STREAM *)WAVE_open(argv[1]);
#ifdef USE_WAVPACK
					} else if (WAVPACK_header(header)) {
						cue_file->type = FILE_WAVPACK;
						cue_file->f.aud.stream = (AUDIO_STREAM *)WAVPACK_open(argv[1], FALSE);
#endif
					} else if (!strcmp(argv[2], "MP3")) {
						cue_file->type = FILE_MP3;
#ifndef USE_MPG123
						image->uses_mp3 = TRUE;
						if (!image->mpegabase) {
							image->mpegabase = OpenLibrary("mpega.library", 0);
						}
						if (!image->mpegabase) {
							error = ERROR_OBJECT_NOT_FOUND;
							error_string = MSG_REQ;
							error_args[0] = (IPTR)"mpega.library";
							break;
						}
						MPEGABase = image->mpegabase;
#endif
						cue_file->f.aud.stream = (AUDIO_STREAM *)MP3_open(argv[1]);
					} else {
						error = ERROR_OBJECT_WRONG_TYPE;
						error_string = MSG_UNKNCOMPMETHOD;
						break;
					}
					if (!cue_file->f.aud.stream) {
						error = IoErr();
						if (error == NO_ERROR) {
							error = ERROR_OBJECT_WRONG_TYPE;
							error_string = MSG_EOF;
						}
						break;
					}
				}
			} else if (!strcmp(argv[0], "TRACK")) {
				if (!cue_file) {
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
				if (track && cue_file == track->file) {
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
					track->sync_size = 0;
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
	}
	if (error != NO_ERROR) {
		goto error;
	}
	error = IoErr();
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

#ifndef USE_MPG123
	if (image->uses_mp3) {
		LONG i;
		for (i = 0; i < MPEGA_MAX_CHANNELS; i++) {
			image->pcm[i] = AllocVec(MPEGA_PCM_SIZE * sizeof(WORD), MEMF_ANY);
			if (!image->pcm[i]) {
				error = ERROR_NO_FREE_STORE;
				goto error;
			}
		}
	}
#endif
	
	done = TRUE;

error:
	FreeVec(line_buffer);
	if (new_lock) {
		CurrentDir(old_lock);
		UnLock(new_lock);
	}
	Close(file);
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

enum {
	NO_ARG,
	UNQUOTED_ARG,
	DOUBLE_QUOTED_ARG,
	SINGLE_QUOTED_ARG,
	COMMENT
};

LONG cue_parse_args (STRPTR cmd, STRPTR *argv, int *argc_ptr) {
	LONG error = NO_ERROR;
	int argc = 0;
	int state = NO_ARG;
	TEXT c;
	do {
		c = *cmd;
		switch (state) {
			case NO_ARG:
				if (c == 0 || isspace(c));
				else if (c == ';' || c == '#')
					state = COMMENT;
				else if (c == '"') {
					if (argc < MAX_ARGS) {
						argv[argc++] = cmd+1;
						state = DOUBLE_QUOTED_ARG;
					} else {
						error = ERROR_TOO_MANY_ARGS;
					}
				} else if (c == '\'') {
					if (argc < MAX_ARGS) {
						argv[argc++] = cmd+1;
						state = SINGLE_QUOTED_ARG;
					} else {
						error = ERROR_TOO_MANY_ARGS;
					}
				} else {
					if (argc < MAX_ARGS) {
						argv[argc++] = cmd;
						state = UNQUOTED_ARG;
					} else {
						error = ERROR_TOO_MANY_ARGS;
					}
				}
				break;
			case UNQUOTED_ARG:
				if (c == 0 || isspace(c)) {
					*cmd = 0;
					if (argc == 1 && !strcmp(argv[0], "REM"))
						state = COMMENT;
					else
						state = NO_ARG;
				} else if (c == ';' || c == '#') {
					*cmd = 0;
					state = COMMENT;
				}
				break;
			case DOUBLE_QUOTED_ARG:
				if (c == 0) {
					error = ERROR_UNMATCHED_QUOTES;
					state = NO_ARG;
				} else if (c == '"') {
					*cmd = 0;
					state = NO_ARG;
				}
				break;
			case SINGLE_QUOTED_ARG:
				if (c == 0) {
					error = ERROR_UNMATCHED_QUOTES;
					state = NO_ARG;
				} else if (c == '\'') {
					*cmd = 0;
					state = NO_ARG;
				}
				break;
			case COMMENT:
				break;
		}
		cmd++;
	} while (c != 0 && error == NO_ERROR && state != COMMENT);
	*argc_ptr = argc;
	return error;
}

LONG cue_parse_index (CONST_STRPTR index_str, ULONG *index) {
	if (strlen(index_str) == 8 &&
		isdigit(index_str[0]) && isdigit(index_str[1]) && index_str[2] == ':' &&
		isdigit(index_str[3]) && isdigit(index_str[4]) && index_str[5] == ':' &&
		isdigit(index_str[6]) && isdigit(index_str[7]))
	{
		ULONG minute, second, frame;
		minute = (index_str[0] - '0')*10 + (index_str[1] - '0');
		second = (index_str[3] - '0')*10 + (index_str[4] - '0');
		frame = (index_str[6] - '0')*10 + (index_str[7] - '0');
		if (second < 60 && frame < 75) {
			*index = MSF2ADDR(minute, second, frame);
			return NO_ERROR;
		}
	}
	return ERROR_BAD_NUMBER;
}

LONG cue_fix_track_indexes (struct CUEImage *image) {
	struct CUETrack *track = image->tracks;
	struct CUEFile *cue_file = NULL;
	UQUAD offset = 0, length, sectors;
	LONG error = NO_ERROR;

	image->block_size = 2048;
	image->total_blocks = 0;
	image->num_tracks = 0;
	track = image->tracks;
	while (track) {
		if (track->file != cue_file) {
			cue_file = track->file;
			offset = 0;
		}
		if (track->index0 == -1) {
			track->index0 = track->index1;
		}
		if (track->index1 == -1) {
			error = ERROR_REQUIRED_ARG_MISSING;
			break;
		}
		offset += (track->index1 - track->index0) * track->sector_size;
		if (track->index2 == -1) {
			switch (cue_file->type) {
				case FILE_BINARY:
				case FILE_MOTOROLA:
					sectors = (cue_file->f.bin.file_size - offset) / track->sector_size;
					track->index2 = track->index1 + sectors;
					break;

				default:
					sectors = (cue_file->f.aud.stream->length - offset + (track->sector_size-1)) / track->sector_size;
					track->index2 = track->index1 + sectors;
					break;
			}
		}
		if (track->index1 < track->index0 || track->index2 < track->index1) {
			error = ERROR_BAD_NUMBER;
			break;
		}
		sectors = track->index2 - track->index1;
		length = sectors * track->sector_size;
		track->offset = offset;
		track->sectors = sectors;
		track->length = length;
		image->total_blocks += sectors;
		image->num_tracks++;
		offset += length;
		track = track->next;
	}
	return error;
}

static inline LONG get_file_header (CONST_STRPTR filename, APTR header, LONG size) {
	BPTR file;
	file = Open(filename, MODE_OLDFILE);
	if (file) {
		size = Read(file, header, size);
		Close(file);
		return size;
	}
	return -1;
}

void CUE_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct CUEImage *image = image_ptr;
	if (image) {
		struct CUEFile *cue_file, *next_file;
		struct CUETrack *track, *next_track;
#ifndef USE_MPG123
		if (image->uses_mp3) {
			LONG i;
			for (i = 0; i < MPEGA_MAX_CHANNELS; i++) {
				FreeVec(image->pcm[i]);
			}
		}
#endif
		track = image->tracks;
		while (track) {
			next_track = track->next;
			FreeVec(track);
			track = next_track;
		}
		cue_file = image->files;
		while (cue_file) {
			next_file = cue_file->next;
			switch (cue_file->type) {
				case FILE_BINARY:
				case FILE_MOTOROLA:
					Close(cue_file->f.bin.file);
					break;

				default:
					if (cue_file->f.aud.stream) {
						AUDIO_close(cue_file->f.aud.stream);
					}
					break;
			}
			FreeVec(cue_file);
			cue_file = next_file;
		}
		FreeVec(image->out_buf);
#ifndef USE_MPG123
		if (image->mpegabase) CloseLibrary(image->mpegabase);
#endif
		FreeVec(image);
	}
}

LONG CUE_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct CUEImage *image = image_ptr;
	dg->dg_DeviceType = DG_CDROM;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG CUE_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CUEImage *image = image_ptr;
	struct CUETrack *track = image->tracks;
	struct CUEFile *cue_file;
	BPTR file;
	UBYTE *buffer;
	UQUAD offset;
	ULONG size;
	UQUAD read_offs, next_offs;
	ULONG to_skip, to_read;
	UQUAD read_pos;
	ULONG read_size;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 0x7ff) return IOERR_BADADDRESS;
	if (size & 0x7ff) return IOERR_BADLENGTH;

	offset >>= 11;
	size >>= 11;
	if (offset >= image->total_blocks) {
		return TDERR_SeekError;
	}
	read_offs = next_offs = 0;
	for (;;) {
		next_offs += track->sectors;
		if (next_offs > offset) break;
		read_offs = next_offs;
		track = track->next;
		if (!track) return TDERR_SeekError;
	}

	to_skip = offset - read_offs;
	while (size) {
		if (track->audio) {
			return TDERR_NoSecHdr;
		}
		cue_file = track->file;
		to_read = max((LONG)min(size, track->sectors - to_skip), 0);
		size -= to_read;
		read_pos = track->offset + ((uint64)to_skip * track->sector_size);
		switch (cue_file->type) {
			case FILE_BINARY:
			case FILE_MOTOROLA:
				file = cue_file->f.bin.file;
				if (!ChangeFilePosition(file, read_pos, OFFSET_BEGINNING)) {
					return TDERR_SeekError;
				}
				if (track->sector_size == 2048) {
					read_size = to_read << 11;
					if (Read(file, buffer, read_size) != read_size) {
						return IPlugin_DOS2IOErr(IoErr());
					}
					buffer += read_size;
					io->io_Actual += read_size;
				} else {
					read_size = track->sector_size;
					if (!(image->out_buf = ReAllocBuf(image->out_buf, &image->out_size, read_size))) {
						return ERROR_NO_FREE_STORE;
					}
					while (to_read--) {
						if (Read(file, image->out_buf, read_size) != read_size) {
							return IPlugin_DOS2IOErr(IoErr());
						}
						CopyMem(image->out_buf + track->sync_size, buffer, 2048);
						buffer += 2048;
						io->io_Actual += 2048;
					}
				}
				break;

			default:
				if (track->sector_size == 2048) {
					read_size = to_read << 11;
					if (AUDIO_read(cue_file->f.aud.stream, buffer, read_pos, read_size) != read_size) {
						return IPlugin_DOS2IOErr(IoErr());
					}
					buffer += read_size;
					io->io_Actual += read_size;
				} else {
					read_size = track->sector_size;
					if (!(image->out_buf = ReAllocBuf(image->out_buf, &image->out_size, read_size))) {
						return ERROR_NO_FREE_STORE;
					}
					while (to_read--) {
						if (AUDIO_read(cue_file->f.aud.stream, image->out_buf, read_pos, read_size) != read_size) {
							return IPlugin_DOS2IOErr(IoErr());
						}
						CopyMem(image->out_buf + track->sync_size, buffer, 2048);
						buffer += 2048;
						io->io_Actual += 2048;
						read_pos += read_size;
					}
				}
				break;
		}
		if (size) {
			to_skip = 0;
			track = track->next;
			if (!track) return IOERR_BADLENGTH;
		}
	}
	return IOERR_SUCCESS;
}

void CUE_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks)
{
	struct CUEImage *image = image_ptr;
	*tracks = (struct CDTrack *)image->tracks;
	*num_tracks = image->num_tracks;
}

LONG CUE_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size)
{
	struct CUEImage *image = image_ptr;
	UBYTE *buffer = buffer_ptr;
	struct CUETrack *track = image->tracks;
	struct CUEFile *cue_file;
	BPTR file;
	UQUAD read_offs, next_offs;
	ULONG to_skip, to_read;
	UQUAD read_pos;
	ULONG read_size;
	ULONG bytes_read = 0;

	if (offset >= image->total_blocks) {
		return bytes_read;
	}
	read_offs = next_offs = 0;
	for (;;) {
		next_offs += track->sectors;
		if (next_offs > offset) break;
		read_offs = next_offs;
		track = track->next;
		if (!track) return bytes_read;
	}

	to_skip = offset - read_offs;
	while (size) {
		if (!track->audio || track->sector_size != 2352) {
			return bytes_read;
		}
		cue_file = track->file;
		to_read = max((LONG)min(size, track->sectors - to_skip), 0);
		size -= to_read;
		read_pos = track->offset + ((UQUAD)to_skip * 2352ULL);
		read_size = to_read * 2352UL;
		switch (cue_file->type) {
			case FILE_BINARY:
			case FILE_MOTOROLA:
				file = cue_file->f.bin.file;
				if (!ChangeFilePosition(file, read_pos, OFFSET_BEGINNING) ||
					Read(file, buffer, read_size) != read_size)
				{
					return bytes_read;
				}
				if (cue_file->type == FILE_MOTOROLA) {
					swab2(buffer, buffer, read_size);
				}
				buffer += read_size;
				bytes_read += read_size;
				break;

			default:
				if (AUDIO_read(cue_file->f.aud.stream, buffer, read_pos, read_size) != read_size) {
					return bytes_read;
				}
				buffer += read_size;
				bytes_read += read_size;
				break;
		}
		if (size) {
			to_skip = 0;
			track = track->next;
			if (!track) return bytes_read;
		}
	}
	return bytes_read;
}
