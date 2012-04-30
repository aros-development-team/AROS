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
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("MDS")

extern struct DiskImagePlugin mds_plugin;

PLUGIN_TABLE(&mds_plugin)

struct MDSTrack {
	struct MDSTrack *next;
	UBYTE track_num;
	UBYTE audio;
	UWORD sector_size;
	UBYTE sync_size;
	UQUAD offset;
	UQUAD length;
	UQUAD sectors;
};

struct MDSImage {
	BPTR file;
	UBYTE first_track;
	UBYTE last_track;
	UBYTE num_tracks;
	struct MDSTrack *tracks;
	ULONG block_size;
	ULONG total_blocks;
	UBYTE *out_buf;
	ULONG out_size;
};

BOOL MDS_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL MDS_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR MDS_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void MDS_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG MDS_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG MDS_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void MDS_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG MDS_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin mds_plugin = {
	PLUGIN_NODE(0, "MDS"),
	PLUGIN_FLAG_M68K,
	16,
	ZERO,
	NULL,
	MDS_Init,
	NULL,
	MDS_CheckImage,
	MDS_OpenImage,
	MDS_CloseImage,
	MDS_Geometry,
	MDS_Read,
	NULL,
	NULL,
	NULL,
	MDS_GetCDTracks,
	MDS_ReadCDDA
};

struct Library *SysBase;
struct Library *DOSBase;
static struct Library *UtilityBase;
static struct DIPluginIFace *IPlugin;

BOOL MDS_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	UtilityBase = data->UtilityBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define MDS_SIGNATURE "MEDIA DESCRIPTOR"

BOOL MDS_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 16 && !memcmp(test, MDS_SIGNATURE, 16);
}

#pragma pack(1)

typedef struct {
	TEXT signature[16];
	UBYTE version[2];
	UWORD medium_type;
	UWORD num_sessions;
	UBYTE unknown1[4];
	UWORD bca_len;
	UBYTE unknown2[8];
	ULONG bca_data_offset;
	UBYTE unknown3[24];
	ULONG disc_structures_offset;
	UBYTE unknown4[12];
	ULONG sessions_blocks_offset;
	ULONG dpm_blocks_offset;
} mds_header_t;

#define MDS_MEDIUM_CD			0x00
#define MDS_MEDIUM_CD_R			0x01
#define MDS_MEDIUM_CD_RW		0x02
#define MDS_MEDIUM_DVD			0x10
#define MDS_MEDIUM_DVD_MINUS_R	0x12

typedef struct {
	LONG session_start;
	LONG session_end;
	UWORD session_number;
	UBYTE num_all_blocks;
	UBYTE num_nontrack_blocks;
	UWORD num_sessions;
	UWORD num_tracks;
	UBYTE unknown1[4];
	ULONG tracks_blocks_offset;
} mds_session_block_t;

typedef struct {
	UBYTE mode;
	UBYTE subchannel;
	UBYTE adr_ctl;
	UBYTE unknown1;
	UBYTE track_num;
	UBYTE unknown2[4];
	UBYTE min;
	UBYTE sec;
	UBYTE frame;
	ULONG extra_offset;
	UWORD sector_size;
	UBYTE unknown3[18];
	ULONG start_sector;
	UQUAD start_offset;
	ULONG number_of_files;
	ULONG footer_offset;
	UBYTE unknown4[24];
} mds_track_block_t;

#define MDS_TRACKMODE_UNKNOWN		0x00
#define MDS_TRACKMODE_AUDIO			0xA9
#define MDS_TRACKMODE_MODE1			0xAA
#define MDS_TRACKMODE_MODE2			0xAB
#define MDS_TRACKMODE_MODE2_FORM1	0xAC
#define MDS_TRACKMODE_MODE2_FORM2	0xAD

#define MDS_LEADIN_TRACK_FIRST		0xA0
#define MDS_LEADIN_TRACK_LAST		0xA1
#define MDS_LEADIN_TRACK_LEADOUT	0xA2

typedef struct {
	ULONG pregap;
	ULONG sectors;
} mds_extra_block_t;

typedef struct {
	ULONG filename_offset;
	ULONG widechar_filename;
	UBYTE unknown1[8];
} mds_footer_t;

#pragma pack()

static LONG mds_get_mdf_filename (BPTR file, STRPTR buffer, LONG size);

APTR MDS_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct MDSImage *image = NULL;
	struct MDSTrack *track = NULL;
	mds_header_t mds_header;
	mds_session_block_t mds_session;
	mds_track_block_t mds_track;
	mds_extra_block_t mds_extra;
	ULONG track_block_offset;
	ULONG extra_block_offset;
	BPTR new_lock = ZERO, old_lock = ZERO;
	TEXT filename[256];
	UBYTE mode;
	LONG i;
	
	new_lock = ParentOfFH(file);
	if (new_lock) {
		old_lock = CurrentDir(new_lock);
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	error = mds_get_mdf_filename(file, filename, sizeof(filename));
	if (error != NO_ERROR) goto error;

	image->file = Open(filename, MODE_OLDFILE);
	if (!image->file) {
		error = IoErr();
		goto error;
	}

	if (Read(file, &mds_header, sizeof(mds_header)) != sizeof(mds_header) ||
		!ChangeFilePosition(file, rle32(&mds_header.sessions_blocks_offset), OFFSET_BEGINNING) ||
		Read(file, &mds_session, sizeof(mds_session)) != sizeof(mds_session))
	{
		error = IoErr();
		goto error;
	}

	if (memcmp(mds_header.signature, MDS_SIGNATURE, sizeof(mds_header.signature))) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	image->num_tracks = rle16(&mds_session.num_tracks);
	image->block_size = 2048;
	image->total_blocks = rle32(&mds_session.session_end);

	track = (struct MDSTrack *)&image->tracks;
	track_block_offset = rle32(&mds_session.tracks_blocks_offset);
	for (i = 0; i < mds_session.num_all_blocks; i++) {
		if (!ChangeFilePosition(file, track_block_offset, OFFSET_BEGINNING) ||
			Read(file, &mds_track, sizeof(mds_track)) != sizeof(mds_track))
		{
			error = IoErr();
			break;
		}
		track_block_offset += sizeof(mds_track);
		mode = mds_track.mode;
		if (mode != MDS_TRACKMODE_UNKNOWN) {
			track = track->next = AllocVec(sizeof(*track), MEMF_ANY);
			if (!track) {
				error = ERROR_NO_FREE_STORE;
				break;
			}
			track->next = NULL;
			track->track_num = mds_track.track_num;
			if (!image->first_track || track->track_num < image->first_track) {
				image->first_track = track->track_num;
			}
			if (!image->last_track || track->track_num > image->last_track) {
				image->last_track = track->track_num;
			}
			track->sector_size = rle16(&mds_track.sector_size);
			track->sync_size = 0;
			switch (mode) {
				case MDS_TRACKMODE_AUDIO:
					track->audio = TRUE;
					if (track->sector_size != 2352) {
						error = ERROR_BAD_NUMBER;
					}
					break;
				case MDS_TRACKMODE_MODE1:
					track->audio = FALSE;
					switch (track->sector_size) {
						case 2048:
							break;
						case 2352:
							track->sync_size = 16;
							break;
						default:
							error = ERROR_BAD_NUMBER;
							break;
					}
					break;
			}
			if (error != NO_ERROR) break;
			track->offset = rle64(&mds_track.start_offset);
			extra_block_offset = rle32(&mds_track.extra_offset);
			if (!extra_block_offset) {
				error = ERROR_REQUIRED_ARG_MISSING;
				break;
			}
			if (!ChangeFilePosition(file, extra_block_offset, OFFSET_BEGINNING) ||
				Read(file, &mds_extra, sizeof(mds_extra)) != sizeof(mds_extra))
			{
				error = IoErr();
				break;
			}
			track->sectors = rle32(&mds_extra.sectors);
			track->length = track->sectors * track->sector_size;
		}
	}
	if (error != NO_ERROR) goto error;

	done = TRUE;

error:
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

static LONG mds_get_mdf_filename (BPTR file, STRPTR buffer, LONG size) {
	struct FileInfoBlock *fib;
	LONG error = NO_ERROR;
	fib = AllocDosObject(DOS_FIB, NULL);
	if (fib) {
		if (ExamineFH(file, fib)) {
			Strlcpy(buffer, fib->fib_FileName, size);
			strip_file_extension(buffer, ".mds");
			Strlcat(buffer, ".mdf", size);
		} else {
			error = IoErr();
		}
		FreeDosObject(DOS_FIB, fib);
	} else {
		error = ERROR_NO_FREE_STORE;
	}
	return error;
}

void MDS_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct MDSImage *image = image_ptr;
	if (image) {
		struct MDSTrack *track, *next_track;
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

LONG MDS_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct MDSImage *image = image_ptr;
	dg->dg_DeviceType = DG_CDROM;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG MDS_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct MDSImage *image = image_ptr;
	struct MDSTrack *track = image->tracks;
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

void MDS_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks)
{
	struct MDSImage *image = image_ptr;
	*tracks = (struct CDTrack *)image->tracks;
	*num_tracks = image->num_tracks;
}

LONG MDS_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size)
{
	struct MDSImage *image = image_ptr;
	UBYTE *buffer = buffer_ptr;
	struct MDSTrack *track = image->tracks;
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
