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
#include "endian.h"
#include "device_locale.h"
#include "scsicmd.h"
#include "support.h"
#include <string.h>
#include <ctype.h>
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("CCD")

extern struct DiskImagePlugin ccd_plugin;

PLUGIN_TABLE(&ccd_plugin)

#define LINE_BUFFER_SIZE 512

struct CCDTrack {
	struct CCDTrack *next;
	UBYTE track_num;
	UBYTE audio;
	UWORD sector_size;
	UBYTE sync_size;
	UQUAD offset;
	UQUAD length;
	UQUAD sectors;
	ULONG index1;
	ULONG index2;
};

struct CCDImage {
	BPTR file;
	UBYTE first_track;
	UBYTE last_track;
	UBYTE num_tracks;
	struct CCDTrack *tracks;
	ULONG block_size;
	ULONG total_blocks;
	QUAD file_size;
	UBYTE *out_buf;
	ULONG out_size;
};

BOOL CCD_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL CCD_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR CCD_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CCD_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CCD_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CCD_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void CCD_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG CCD_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin ccd_plugin = {
	PLUGIN_NODE(0, "CCD"),
	PLUGIN_FLAG_M68K,
	64,
	ZERO,
	NULL,
	CCD_Init,
	NULL,
	CCD_CheckImage,
	CCD_OpenImage,
	CCD_CloseImage,
	CCD_Geometry,
	CCD_Read,
	NULL,
	NULL,
	NULL,
	CCD_GetCDTracks,
	CCD_ReadCDDA
};

struct Library *SysBase;
struct Library *DOSBase;
static struct Library *UtilityBase;
static struct DIPluginIFace *IPlugin;

BOOL CCD_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	UtilityBase = data->UtilityBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL CCD_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 9 && !memcmp(test, "[CloneCD]", 9) && IsText(test, testsize);
}

static LONG ccd_get_img_filename (BPTR file, STRPTR buffer, LONG size);
static LONG ccd_parse_ini_line (STRPTR line, STRPTR *section, STRPTR *attr, STRPTR *value);

APTR CCD_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CCDImage *image = NULL;
	struct CCDTrack *track, *prev_track;
	STRPTR line_buffer;
	LONG track_num, mode, index;
	STRPTR section, attr, value;
	BPTR new_lock = ZERO, old_lock = ZERO;
	TEXT filename[256];
	UQUAD offset, length, sectors;
	
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
	error = ccd_get_img_filename(file, filename, sizeof(filename));
	if (error != NO_ERROR) goto error;

	image->file = Open(filename, MODE_OLDFILE);
	if (!image->file) {
		error = IoErr();
		goto error;
	}

	image->file_size = GetFileSize(image->file);
	if (image->file_size == -1) {
		error = IoErr();
		goto error;
	}
	
	track = prev_track = NULL;
	while (FGets(file, line_buffer, LINE_BUFFER_SIZE)) {
		error = ccd_parse_ini_line(line_buffer, &section, &attr, &value);
		if (error != NO_ERROR) break;
		if (section) {
			if (!strncmp(section, "TRACK ", 6)) {
				if (!isdigit(section[6]) || StrToLong(&section[6], &track_num) == -1) {
					error = ERROR_BAD_NUMBER;
					break;
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
				track->track_num = track_num;
				if (!image->first_track || track_num < image->first_track) {
					image->first_track = track_num;
				}
				if (!image->last_track || track_num > image->last_track) {
					image->last_track = track_num;
				}
				track->sector_size = 2352;
				if (track_num == 1) {
					track->audio = FALSE;
					track->sync_size = 16;
				} else {
					track->audio = TRUE;
					track->sync_size = 0;
				}
			}
		} else {
			if (track) {
				if (!strcmp(attr, "MODE") && value) {
					if (StrToLong(value, &mode) == -1) {
						error = ERROR_BAD_NUMBER;
						break;
					}
					switch (mode) {
						case 0:
							track->audio = TRUE;
							track->sync_size = 0;
							break;
						case 1:
							track->audio = FALSE;
							track->sync_size = 16;
							break;
						case 2:
							track->audio = FALSE;
							track->sync_size = 24;
							break;
						default:
							error = ERROR_NOT_IMPLEMENTED;
							break;
					}
					if (error != NO_ERROR) break;
				} else if (!strcmp(attr, "INDEX 1") && value) {
					if (StrToLong(value, &index) == -1) {
						error = ERROR_BAD_NUMBER;
						break;
					}
					track->index1 = index;
					if (prev_track && !prev_track->index2) {
						prev_track->index2 = index;
					}
				} else if (!strcmp(attr, "INDEX 2") && value) {
					if (StrToLong(value, &index) == -1) {
						error = ERROR_BAD_NUMBER;
						break;
					}
					track->index2 = index;
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
	
	image->block_size = 2048;
	image->total_blocks = 0;
	image->num_tracks = 0;
	track = image->tracks;
	while (track) {
		offset = (UQUAD)track->index1 * 2352ULL;
		if (!track->index2) {
			sectors = (image->file_size - offset) / track->sector_size;
			track->index2 = track->index1 + sectors;
		}
		if (track->index2 < track->index1) {
			error = ERROR_BAD_NUMBER;
			break;
		}
		sectors = track->index2 - track->index1;
		length = sectors * 2352ULL;
		track->offset = offset;
		track->sectors = sectors;
		track->length = length;
		image->total_blocks += sectors;
		image->num_tracks++;
		track = track->next;
	}
	if (error != NO_ERROR) {
		goto error;
	}

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

static void strip_file_extension (STRPTR name, CONST_STRPTR ext) {
	LONG len = strlen(name) - strlen(ext);
	if (len >= 0) {
		name += len;
		if (!Stricmp(name, ext)) {
			*name = 0;
		}
	}
}

static LONG ccd_get_img_filename (BPTR file, STRPTR buffer, LONG size) {
	struct FileInfoBlock *fib;
	LONG error = NO_ERROR;
	fib = AllocDosObject(DOS_FIB, NULL);
	if (fib) {
		if (ExamineFH(file, fib)) {
			Strlcpy(buffer, fib->fib_FileName, size);
			strip_file_extension(buffer, ".ccd");
			Strlcat(buffer, ".img", size);
		} else {
			error = IoErr();
		}
		FreeDosObject(DOS_FIB, fib);
	} else {
		error = ERROR_NO_FREE_STORE;
	}
	return error;
}

static void rtrim (STRPTR string) {
	int len = strlen(string);
	int i;
	for (i = len-1; i >= 0; i--) {
		if (isspace(string[i])) {
			string[i] = 0;
		} else {
			break;
		}
	}
}

static LONG ccd_parse_ini_line (STRPTR line, STRPTR *section, STRPTR *attr, STRPTR *value) {
	int len;
	*section = *attr = *value = NULL;
	rtrim(line);
	len = strlen(line);
	if (line[0] == '[' && line[len-1] == ']') {
		*section = line+1;
		line[len-1] = 0;
	} else {
		*value = strchr(*attr = line, '=');
		if (*value) *(*value)++ = 0;
	}
	return NO_ERROR;
}

void CCD_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct CCDImage *image = image_ptr;
	if (image) {
		struct CCDTrack *track, *next_track;
		track = image->tracks;
		while (track) {
			next_track = track->next;
			FreeVec(track);
			track = next_track;
		}
		FreeVec(image->out_buf);
		Close(image->file);
		FreeVec(image);
	}
}

LONG CCD_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct CCDImage *image = image_ptr;
	dg->dg_DeviceType = DG_CDROM;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG CCD_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CCDImage *image = image_ptr;
	struct CCDTrack *track = image->tracks;
	BPTR file = image->file;
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
		to_read = max((LONG)min(size, track->sectors - to_skip), 0);
		size -= to_read;
		read_pos = track->offset + ((UQUAD)to_skip * track->sector_size);
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
		if (size) {
			to_skip = 0;
			track = track->next;
			if (!track) return IOERR_BADLENGTH;
		}
	}
	return IOERR_SUCCESS;
}

void CCD_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks)
{
	struct CCDImage *image = image_ptr;
	*tracks = (struct CDTrack *)image->tracks;
	*num_tracks = image->num_tracks;
}

LONG CCD_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size)
{
	struct CCDImage *image = image_ptr;
	UBYTE *buffer = buffer_ptr;
	struct CCDTrack *track = image->tracks;
	BPTR file = image->file;
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
		to_read = max((LONG)min(size, track->sectors - to_skip), 0);
		size -= to_read;
		read_pos = track->offset + ((UQUAD)to_skip * 2352ULL);
		read_size = to_read * 2352;
		if (!ChangeFilePosition(file, read_pos, OFFSET_BEGINNING) ||
			Read(file, buffer, read_size) != read_size)
		{
			return bytes_read;
		}
		buffer += read_size;
		bytes_read += read_size;
		to_skip = 0;
		track = track->next;
		if (!track) return bytes_read;
	}
	return bytes_read;
}
