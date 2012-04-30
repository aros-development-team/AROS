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
#include <libraries/z.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/z.h>
#include "endian.h"
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("DAX")

extern struct DiskImagePlugin dax_plugin;

PLUGIN_TABLE(&dax_plugin)

typedef struct {
	ULONG offset;
	UWORD size;
	UWORD comp;
} frame_t;

struct DAXImage {
	BPTR file;
	UBYTE *in_buf, *out_buf;
	ULONG nframes;
	frame_t *frames;
	frame_t *frame_in_buf;
	ULONG total_bytes;
	ULONG block_size;
	ULONG total_blocks;
	struct Library *zbase;
};

BOOL DAX_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL DAX_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR DAX_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void DAX_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG DAX_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG DAX_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin dax_plugin = {
	PLUGIN_NODE(0, "DAX"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	DAX_Init,
	NULL,
	DAX_CheckImage,
	DAX_OpenImage,
	DAX_CloseImage,
	DAX_Geometry,
	DAX_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;
#define ZBase image->zbase

BOOL DAX_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define DAX_MAGIC MAKE_ID('D','A','X',0)

BOOL DAX_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= sizeof(ULONG) && rbe32(test) == DAX_MAGIC;
}

#pragma pack(1)

#define DAX_FRAME_SIZE 8192
#define MAX_NCAREAS 192

typedef struct {
	ULONG magic;
	ULONG isosize;
	ULONG version;
	ULONG ncareas;
	UBYTE reserved[16];
} dax_t;

typedef struct {
	ULONG frame;
	ULONG size;
} ncarea_t;

#pragma pack()

static inline BOOL IsNCArea (ncarea_t *ncareas, ULONG frame, ULONG count) {
	ULONG i;
	for (i = 0; i < count; i++) {
		if (frame >= ncareas->frame &&
			(frame - ncareas->frame) < ncareas->size)
		{
			return TRUE;
		}
	}
	return FALSE;
}

APTR DAX_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct DAXImage *image = NULL;
	dax_t dax;
	ULONG nframes;
	ULONG *offsets = NULL;
	UWORD *lengths = NULL;
	ncarea_t *ncareas = NULL;
	ULONG i;

	if (FRead(file, &dax, 1, sizeof(dax)) != sizeof(dax)) {
		error = IoErr();
		goto error;
	}

	if (rle32(&dax.version) > 1) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->file = file;
	image->total_bytes = rle32(&dax.isosize);
	image->block_size = 2048;
	image->total_blocks = image->total_bytes >> 11;

	image->zbase = OpenLibrary("z.library", 1);
	if (!image->zbase || !CheckLib(image->zbase, 1, 6)) {
		error = ERROR_OBJECT_NOT_FOUND;
		error_string = MSG_REQVER;
		error_args[0] = (IPTR)"z.library";
		error_args[1] = 1;
		error_args[2] = 6;
		goto error;
	}

	nframes = image->total_bytes / DAX_FRAME_SIZE;
	if (image->total_bytes % DAX_FRAME_SIZE) nframes++;
	image->nframes = nframes;

	offsets = AllocVec(nframes << 2, MEMF_ANY);
	lengths = AllocVec(nframes << 1, MEMF_ANY);
	image->frames = AllocVec(nframes << 3, MEMF_ANY);
	if (!offsets || !lengths || !image->frames) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (FRead(file, offsets, 4, nframes) != nframes ||
		FRead(file, lengths, 2, nframes) != nframes)
	{
		error = IoErr();
		goto error;
	}

	for (i = 0; i < nframes; i++) {
		image->frames[i].offset = rle32(&offsets[i]);
		image->frames[i].size = rle16(&lengths[i]);
		image->frames[i].comp = 1;
	}

	FreeVec(offsets);
	FreeVec(lengths);
	offsets = NULL;
	lengths = NULL;

	if (dax.ncareas) {
		ULONG nncareas;
		nncareas = rle32(&dax.ncareas);
		ncareas = AllocVec(MAX_NCAREAS*sizeof(*ncareas), MEMF_ANY);
		if (!ncareas) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}
		if (FRead(file, ncareas, sizeof(*ncareas), MAX_NCAREAS) != MAX_NCAREAS) {
			error = IoErr();
			goto error;
		}
		for (i = 0; i < MAX_NCAREAS; i++) {
			ncareas[i].frame = rle32(&ncareas[i].frame);
			ncareas[i].size = rle32(&ncareas[i].size);
		}
		for (i = 0; i < nframes; i++) {
			if (IsNCArea(ncareas, i, nncareas)) {
				image->frames[i].comp = 0;
			}
		}
		FreeVec(ncareas);
		ncareas = NULL;
	}

	image->in_buf = AllocVec(DAX_FRAME_SIZE+1024, MEMF_ANY);
	image->out_buf = AllocVec(DAX_FRAME_SIZE, MEMF_ANY);
	if (!image->in_buf || !image->out_buf) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	done = TRUE;

error:
	FreeVec(ncareas);
	FreeVec(offsets);
	FreeVec(lengths);
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

void DAX_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct DAXImage *image = image_ptr;
	if (image) {
		if (image->zbase) CloseLibrary(image->zbase);
		FreeVec(image->in_buf);
		FreeVec(image->out_buf);
		FreeVec(image->frames);
		Close(image->file);
		FreeVec(image);
	}
}

LONG DAX_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct DAXImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG DAX_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct DAXImage *image = image_ptr;
	BPTR file = image->file;
	ULONG offset;
	UBYTE *buffer;
	ULONG size;
	ULONG i;
	ULONG to_skip, to_read;
	frame_t *frame;

	offset = io->io_Offset;
	buffer = io->io_Data;
	size = io->io_Length;
	io->io_Actual = 0;

	i = offset / DAX_FRAME_SIZE;
	to_skip = offset % DAX_FRAME_SIZE;
	if (i >= image->nframes) return IOERR_BADADDRESS;

	frame = &image->frames[i];
	io->io_Actual = size;
	while (size) {
		if (i >= image->nframes) {
			io->io_Actual -= size;
			return IOERR_BADLENGTH;
		}

		to_read = min(size, DAX_FRAME_SIZE - to_skip);
		if (frame->comp) {
			if (image->frame_in_buf != frame) {
				ULONG bytes;
				image->frame_in_buf = NULL;
				if (!ChangeFilePosition(file, frame->offset, OFFSET_BEGINNING) ||
					Read(file, image->in_buf, frame->size) != frame->size)
				{
					io->io_Actual -= size;
					return IPlugin_DOS2IOErr(IoErr());
				}
				bytes = DAX_FRAME_SIZE;
				if (Uncompress(image->out_buf, &bytes, image->in_buf, frame->size) != Z_OK) {
					io->io_Actual -= size;
					return TDERR_NotSpecified;
				}
				image->frame_in_buf = frame;
			}
			CopyMem(image->out_buf + to_skip, buffer, to_read);
		} else {
			if (!ChangeFilePosition(file, frame->offset + to_skip, OFFSET_BEGINNING) ||
				Read(file, buffer, to_read) != to_read)
			{
				io->io_Actual -= size;
				return IPlugin_DOS2IOErr(IoErr());
			}
		}

		buffer += to_read;
		size -= to_read;
		to_skip = 0;
		frame++;
		i++;
	}
	return IOERR_SUCCESS;
}
