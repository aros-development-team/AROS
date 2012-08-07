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
#include <string.h>
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("NRG")

extern struct DiskImagePlugin nrg_plugin;

PLUGIN_TABLE(&nrg_plugin)

struct NRGTrack {
	struct NRGTrack *next;
	UBYTE track_num;
	UBYTE audio;
	UWORD sector_size;
	UBYTE sync_size;
	UQUAD offset;
	UQUAD length;
	UQUAD sectors;
};

struct NRGImage {
	BPTR file;
	UBYTE nrg_version;
	UBYTE first_track;
	UBYTE last_track;
	UBYTE num_tracks;
	struct NRGTrack *tracks;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
	UBYTE *out_buf;
	ULONG out_size;
};

BOOL NRG_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL NRG_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR NRG_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void NRG_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG NRG_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG NRG_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
void NRG_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks);
LONG NRG_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size);

struct DiskImagePlugin nrg_plugin = {
	PLUGIN_NODE(0, "NRG"),
	PLUGIN_FLAG_FOOTER|PLUGIN_FLAG_M68K,
	12,
	ZERO,
	NULL,
	NRG_Init,
	NULL,
	NRG_CheckImage,
	NRG_OpenImage,
	NRG_CloseImage,
	NRG_Geometry,
	NRG_Read,
	NULL,
	NULL,
	NULL,
	NRG_GetCDTracks,
	NRG_ReadCDDA
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL NRG_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define ID_NERO MAKE_ID('N','E','R','O') /* v1 */
#define ID_NER5 MAKE_ID('N','E','R','5') /* v2 */
#define ID_CUES MAKE_ID('C','U','E','S') /* v1 */
#define ID_CUEX MAKE_ID('C','U','E','X') /* v2 */
#define ID_DAOI MAKE_ID('D','A','O','I') /* v1 */
#define ID_DAOX MAKE_ID('D','A','O','X') /* v2 */
#define ID_TAOI MAKE_ID('T','A','O','I') /* v1 */
#define ID_TAOX MAKE_ID('T','A','O','X') /* v2 */
#define ID_CDTX MAKE_ID('C','D','T','X') /* v2 */
#define ID_ETNF MAKE_ID('E','T','N','F') /* v1 */
#define ID_ETN2 MAKE_ID('E','T','N','2') /* v2 */
#define ID_SINF MAKE_ID('S','I','N','F') /* v1 & v2 */
#define ID_MTYP MAKE_ID('M','T','Y','P') /* v1 & v2 */
#define ID_END  MAKE_ID('E','N','D','!') /* v1 & v2 */

BOOL NRG_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return (testsize >= 8 && rbe32(&test[testsize-8]) == ID_NERO) ||
		(testsize >= 12 && rbe32(&test[testsize-12]) == ID_NER5);
}

#pragma pack(1)

typedef union {
	struct {
		ULONG pad;
		ULONG id;
		ULONG offset;
	} v1;
	struct {
		ULONG id;
		UQUAD offset;
	} v2;
} nrg_footer_t;

typedef struct {
	ULONG id;
	ULONG size;
} nrg_chunk_t;

typedef struct {
	UBYTE reserved[10];
	ULONG sector_size;
	ULONG mode;
	ULONG index0; // pre gap
	ULONG index1; // start of track
	ULONG index2; // end of track + 1
} nrg_daoi_track_t;

typedef struct {
	ULONG chunk_size; // chunk size in little-endian format
	ULONG reserved[3];
	ULONG toc_type;
	UBYTE first_track;
	UBYTE last_track;
	nrg_daoi_track_t tracks[];
} nrg_daoi_t;

typedef struct {
	UBYTE reserved[10];
	ULONG sector_size;
	ULONG mode;
	UQUAD index0; // pre gap
	UQUAD index1; // start of track
	UQUAD index2; // end of track + 1
} nrg_daox_track_t;

typedef struct {
	ULONG chunk_size; // chunk size in little-endian format
	ULONG reserved[3];
	ULONG toc_type;
	UBYTE first_track;
	UBYTE last_track;
	nrg_daox_track_t tracks[];
} nrg_daox_t;

typedef struct {
	ULONG track_offset;
	ULONG track_length;
} nrg_taoi_t;

typedef struct {
	UQUAD track_offset;
	UQUAD track_length;
} nrg_taox_t;

#pragma pack()

static APTR nrg_read_chunk (BPTR file, nrg_chunk_t *chunk, APTR *buf, LONG *buf_size);

APTR NRG_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct NRGImage *image = NULL;
	nrg_footer_t footer;
	QUAD offset;
	nrg_chunk_t chunk;
	APTR buf = NULL;
	LONG buf_size = 0;
	nrg_daoi_t *daoi;
	nrg_daox_t *daox;
	nrg_taoi_t *taoi;
	nrg_taox_t *taox;
	struct NRGTrack *track;
	ULONG i;
	
	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;
	
	if (!ChangeFilePosition(file, -(int)sizeof(footer), OFFSET_END) ||
		Read(file, &footer, sizeof(footer)) != sizeof(footer))
	{
		error = IoErr();
		goto error;
	}
	
	if (rbe32(&footer.v1.id) == ID_NERO) {
		image->nrg_version = 1;
		offset = rbe32(&footer.v1.offset);
	} else if (rbe32(&footer.v2.id) == ID_NER5) {
		image->nrg_version = 2;
		offset = rbe64(&footer.v2.offset);
	} else {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	if (!ChangeFilePosition(file, offset, OFFSET_BEGINNING)) {
		error = IoErr();
		goto error;
	}
	
	while (!done) {
		if (Read(file, &chunk, sizeof(chunk)) != sizeof(chunk)) {
			error = IoErr();
			goto error;
		}
		wbe32(&chunk.id, chunk.id);
		wbe32(&chunk.size, chunk.size);
		switch (chunk.id) {

			case ID_DAOI:
				if (chunk.size < (sizeof(nrg_daoi_t) + sizeof(nrg_daoi_track_t))) {
					error = ERROR_BAD_NUMBER;
					goto error;
				}
				if (!(daoi = nrg_read_chunk(file, &chunk, &buf, &buf_size))) {
					error = IoErr();
					goto error;
				}
				track = (struct NRGTrack *)&image->tracks;
				image->first_track = daoi->first_track;
				image->last_track = daoi->last_track;
				image->num_tracks = image->last_track - image->first_track + 1;
				for (i = 0; i < image->num_tracks; i++) {
					track->next = AllocVec(sizeof(*track), MEMF_ANY);
					if (!(track = track->next)) {
						error = ERROR_NO_FREE_STORE;
						goto error;
					}
					track->next = NULL;
					track->track_num = image->first_track+i;
					track->sector_size = rbe32(&daoi->tracks[i].sector_size);
					track->sync_size = 0;
					switch (track->sector_size) {
						case 2048:
							track->audio = FALSE;
							break;
						case 2352:
							track->audio = TRUE;
							break;
						default:
							error = ERROR_BAD_NUMBER;
							goto error;
					}
					track->offset = rbe32(&daoi->tracks[i].index1);
					track->length = rbe32(&daoi->tracks[i].index2) - track->offset;
					track->sectors = track->length / track->sector_size;
					image->total_bytes += track->sectors << 11;
					if (track->sector_size != 2048 && track->sector_size != 2352) {
						error = ERROR_BAD_NUMBER;
						goto error;
					}
				}
				done = TRUE;
				break;

			case ID_DAOX:
				if (chunk.size < (sizeof(nrg_daox_t) + sizeof(nrg_daox_track_t))) {
					error = ERROR_BAD_NUMBER;
					goto error;
				}
				if (!(daox = nrg_read_chunk(file, &chunk, &buf, &buf_size))) {
					error = IoErr();
					goto error;
				}
				track = (struct NRGTrack *)&image->tracks;
				image->first_track = daox->first_track;
				image->last_track = daox->last_track;
				image->num_tracks = image->last_track - image->first_track + 1;
				for (i = 0; i < image->num_tracks; i++) {
					track->next = AllocVec(sizeof(*track), MEMF_ANY);
					if (!(track = track->next)) {
						error = ERROR_NO_FREE_STORE;
						goto error;
					}
					track->next = NULL;
					track->track_num = image->first_track+i;
					track->sector_size = rbe32(&daox->tracks[i].sector_size);
					track->sync_size = 0;
					switch (track->sector_size) {
						case 2048:
							track->audio = FALSE;
							break;
						case 2352:
							track->audio = TRUE;
							break;
						default:
							error = ERROR_BAD_NUMBER;
							goto error;
					}
					track->offset = rbe64(&daox->tracks[i].index1);
					track->length = rbe64(&daox->tracks[i].index2) - track->offset;
					track->sectors = track->length / track->sector_size;
					image->total_bytes += track->sectors << 11;
					if (track->sector_size != 2048 && track->sector_size != 2352) {
						error = ERROR_BAD_NUMBER;
						goto error;
					}
				}
				done = TRUE;
				break;

			case ID_TAOI:
				if (chunk.size < sizeof(nrg_taoi_t)) {
					error = ERROR_BAD_NUMBER;
					goto error;
				}
				if (!(taoi = nrg_read_chunk(file, &chunk, &buf, &buf_size))) {
					error = IoErr();
					goto error;
				}
				image->first_track = 1;
				image->last_track = 1;
				image->num_tracks = image->last_track - image->first_track + 1;
				image->tracks = AllocVec(sizeof(*track), MEMF_ANY);
				if (!(track = image->tracks)) {
					error = ERROR_NO_FREE_STORE;
					goto error;
				}
				track->next = NULL;
				track->track_num = 1;
				track->audio = FALSE;
				track->sector_size = 2048;
				track->sync_size = 0;
				track->offset = rbe32(&taoi->track_offset);
				track->length = rbe32(&taoi->track_length);
				track->sectors = track->length / track->sector_size;
				image->total_bytes += track->sectors << 11;
				done = TRUE;
				break;

			case ID_TAOX:
				if (chunk.size < sizeof(nrg_taox_t)) {
					error = ERROR_BAD_NUMBER;
					goto error;
				}
				if (!(taox = nrg_read_chunk(file, &chunk, &buf, &buf_size))) {
					error = IoErr();
					goto error;
				}
				image->first_track = 1;
				image->last_track = 1;
				image->num_tracks = image->last_track - image->first_track + 1;
				image->tracks = AllocVec(sizeof(*track), MEMF_ANY);
				if (!(track = image->tracks)) {
					error = ERROR_NO_FREE_STORE;
					goto error;
				}
				track->next = NULL;
				track->track_num = 1;
				track->audio = FALSE;
				track->sector_size = 2048;
				track->sync_size = 0;
				track->offset = rbe64(&taox->track_offset);
				track->length = rbe64(&taox->track_length);
				track->sectors = track->length / track->sector_size;
				image->total_bytes += track->sectors << 11;
				done = TRUE;
				break;

			case ID_END:
				error = ERROR_OBJECT_NOT_FOUND;
				goto error;

			default:
				if (!ChangeFilePosition(file, chunk.size, OFFSET_CURRENT)) {
					error = IoErr();
					goto error;
				}
				break;

		}
	}
	image->block_size = 2048;
	image->total_blocks = image->total_bytes >> 11;

error:
	FreeVec(buf);
	if (!done) {
		if (image) {
			Plugin_CloseImage(Self, image);
			image = NULL;
		} else {
			Close(file);
		}
		if (error == NO_ERROR) {
			error = ERROR_OBJECT_WRONG_TYPE;
			error_string = MSG_EOF;
		}
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}

static APTR nrg_read_chunk (BPTR file, nrg_chunk_t *chunk, APTR *buf, LONG *buf_size) {
	if (*buf_size < chunk->size) {
		FreeVec(*buf);
		*buf = AllocVec(chunk->size, MEMF_ANY);
		*buf_size = chunk->size;
		if (!*buf) {
			*buf_size = 0;
			SetIoErr(ERROR_NO_FREE_STORE);
			return NULL;
		}
	}
	if (Read(file, *buf, chunk->size) != chunk->size) {
		return NULL;
	}
	return *buf;
}

void NRG_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct NRGImage *image = image_ptr;
	if (image) {
		struct NRGTrack *track, *next_track;
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

LONG NRG_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct NRGImage *image = image_ptr;
	dg->dg_DeviceType = DG_CDROM;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG NRG_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct NRGImage *image = image_ptr;
	struct NRGTrack *track = image->tracks;
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

void NRG_GetCDTracks (struct DiskImagePlugin *Self, APTR image_ptr, struct CDTrack **tracks,
	ULONG *num_tracks)
{
	struct NRGImage *image = image_ptr;
	*tracks = (struct CDTrack *)image->tracks;
	*num_tracks = image->num_tracks;
}

LONG NRG_ReadCDDA (struct DiskImagePlugin *Self, APTR image_ptr, APTR buffer_ptr, ULONG offset,
	ULONG size)
{
	struct NRGImage *image = image_ptr;
	UBYTE *buffer = buffer_ptr;
	struct NRGTrack *track = image->tracks;
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
