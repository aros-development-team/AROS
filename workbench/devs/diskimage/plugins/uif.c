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
#include <proto/utility.h>
#include <proto/z.h>
#include <string.h>
#include "endian.h"
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("UIF")

extern struct DiskImagePlugin uif_plugin;

PLUGIN_TABLE(&uif_plugin)

#define HASH_FUNC 20

BOOL UIF_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL UIF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR UIF_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void UIF_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG UIF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG UIF_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin uif_plugin = {
	PLUGIN_NODE(0, "UIF"),
	PLUGIN_FLAG_FOOTER|PLUGIN_FLAG_M68K,
	64,
	ZERO,
	NULL,
	UIF_Init,
	NULL,
	UIF_CheckImage,
	UIF_OpenImage,
	UIF_CloseImage,
	UIF_Geometry,
	UIF_Read,
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

BOOL UIF_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define BBIS_MAGIC MAKE_ID('b','b','i','s')
#define BLHR_MAGIC MAKE_ID('b','l','h','r')
#define BSDR_MAGIC MAKE_ID('b','s','d','r')

BOOL UIF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 64 && rbe32(&test[testsize-64]) == BBIS_MAGIC;
}

#pragma pack(1)

typedef struct {
	ULONG magic;
	ULONG size;
	ULONG ver;
	ULONG num;
} blhr_t;

typedef struct {
	UQUAD offset;
	ULONG in_size;
	ULONG sector;
	ULONG out_size;
	ULONG type;
} blhr_data_t;

typedef struct {
	ULONG magic;
	ULONG pad1;
	UWORD ver;
	ULONG image_type;
	UWORD pad2;
	ULONG sectors;
	ULONG sector_size;
	ULONG pad3;
	UQUAD blhr;
	ULONG blhr_bbis_size;
	UBYTE  hash[16];
	ULONG pad4[2];
} bbis_t;

#pragma pack()

struct UIFHash {
	blhr_data_t *data;
	ULONG offset;
};

struct UIFImage {
	BPTR file;
	z_stream zs;
	UBYTE *in_buf, *out_buf;
	ULONG in_size, out_size;
	ULONG num;
	blhr_data_t *data, *data_in_buf;
	struct UIFHash *hash;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
	struct Library *zbase;
};

APTR UIF_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct UIFImage *image = NULL;
	bbis_t bbis;
	blhr_t blhr;
	blhr_data_t *data = NULL;
	ULONG i;

	ULONG hash_size;
	struct UIFHash *hash;
	UQUAD offset, next_offset;
	UQUAD hash_offset;

	if (!ChangeFilePosition(file, -(int)sizeof(bbis), OFFSET_END) ||
		Read(file, &bbis, sizeof(bbis)) != sizeof(bbis))
	{
		error = IoErr();
		goto error;
	}
	if (rbe32(&bbis.magic) != BBIS_MAGIC) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	if (!ChangeFilePosition(file, rle64(&bbis.blhr), OFFSET_BEGINNING) ||
		Read(file, &blhr, sizeof(blhr)) != sizeof(blhr))
	{
		error = IoErr();
		goto error;
	}
	if (rbe32(&blhr.magic) != BLHR_MAGIC) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->file = file;
	image->num = rle32(&blhr.num);
	image->block_size = rle32(&bbis.sector_size);

	image->zbase = OpenLibrary("z.library", 1);
	if (!image->zbase || !CheckLib(image->zbase, 1, 6)) {
		error = ERROR_OBJECT_NOT_FOUND;
		error_string = MSG_REQVER;
		error_args[0] = (IPTR)"z.library";
		error_args[1] = 1;
		error_args[2] = 6;
		goto error;
	}

	image->in_size = rle32(&blhr.size);
	image->out_size = sizeof(*data) * image->num;

	image->in_buf = AllocVec(image->in_size, MEMF_ANY);
	image->data = AllocVec(image->out_size, MEMF_ANY);
	if (!image->in_buf || !image->data) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (Read(file, image->in_buf, image->in_size) != image->in_size) {
		error = IoErr();
		goto error;
	}

	if (InflateInit2(&image->zs, 15) != Z_OK) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_ZLIBERR;
		goto error;
	}

	image->zs.next_in = image->in_buf;
	image->zs.next_out = (UBYTE *)image->data;
	image->zs.avail_in = image->in_size;
	image->zs.avail_out = image->out_size;
	if (Inflate(&image->zs, Z_SYNC_FLUSH) != Z_STREAM_END) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_ZLIBERR;
		goto error;
	}

	image->out_size = 0;
	data = image->data;
	for (i = 0; i < image->num; i++, data++) {
		data->offset = rle64(&data->offset);
		data->in_size = rle32(&data->in_size);
		data->sector = rle32(&data->sector);
		data->out_size = rle32(&data->out_size);
		data->type = rle32(&data->type);

		image->total_blocks += data->out_size;
		data->out_size *= image->block_size;

		switch (data->type) {
			case 1:
			case 3:
			case 5:
				break;
			default:
				error = ERROR_BAD_NUMBER;
				goto error;
		}
	}

	image->total_bytes = image->total_blocks * image->block_size;

	hash_size = ((image->total_bytes >> HASH_FUNC) + 1) << 3;
	image->hash = hash = AllocVec(hash_size, MEMF_ANY);
	if (!hash) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	offset =
	next_offset =
	hash_offset = 0;
	data = image->data;
	for (i = 0; i < image->num; i++, data++) {
		next_offset += data->out_size;
		while (next_offset > hash_offset) {
			hash->data = data;
			hash->offset = offset / image->block_size;
			hash++;
			hash_offset += (1 << HASH_FUNC);
		}
		offset = next_offset;
	}

	done = TRUE;

error:
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

void UIF_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct UIFImage *image = image_ptr;
	if (image) {
		if (image->zbase) {
			if (CheckLib(image->zbase, 1, 6)) InflateEnd(&image->zs);
			CloseLibrary(image->zbase);
		}
		FreeVec(image->hash);
		FreeVec(image->data);
		FreeVec(image->in_buf);
		FreeVec(image->out_buf);
		Close(image->file);
		FreeVec(image);
	}
}

LONG UIF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct UIFImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG UIF_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct UIFImage *image = image_ptr;
	BPTR file = image->file;
	UBYTE *buffer;
	UQUAD offset;
	ULONG size;
	struct UIFHash *hash;
	blhr_data_t *data;
	UQUAD read_offs, next_offs;
	ULONG to_skip, to_read;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset >= image->total_bytes) return TDERR_SeekError;

	hash = &image->hash[offset >> HASH_FUNC];

	data = hash->data;
	read_offs = next_offs = hash->offset * image->block_size;
	for (;;) {
		next_offs += data->out_size;
		if (next_offs > offset) break;
		read_offs = next_offs;
		data++;
	}

	to_skip = offset - read_offs;
	while (size) {
		if (read_offs == image->total_bytes) return IOERR_BADLENGTH;

		if (data->in_size > image->in_size) {
			FreeVec(image->in_buf);
			image->in_buf = AllocVec(image->in_size = data->in_size, MEMF_ANY);
			if (!image->in_buf) {
				image->in_size = 0;
				return TDERR_NoMem;
			}
		}
		if (data->out_size > image->out_size) {
			FreeVec(image->out_buf);
			image->out_buf = AllocVec(image->out_size = data->out_size, MEMF_ANY);
			if (!image->out_buf) {
				image->out_size = 0;
				return TDERR_NoMem;
			}
		}

		if (image->data_in_buf != data) {
			image->data_in_buf = NULL;
			if (!ChangeFilePosition(file, data->offset, OFFSET_BEGINNING) ||
				Read(file, image->in_buf, data->in_size) != data->in_size)
			{
				return IPlugin_DOS2IOErr(IoErr());
			}
		}

		to_read = min(data->out_size - to_skip, size);
		switch (data->type) {
			case 1:
				if (data->in_size != data->out_size) {
					return TDERR_NotSpecified;
				}
				CopyMem(image->in_buf + to_skip, buffer, to_read);
				image->data_in_buf = data;
				break;
			case 3:
				memset(buffer, 0, to_read);
				break;
			case 5:
				if (image->data_in_buf != data) {
					InflateReset(&image->zs);
					image->zs.next_in = image->in_buf;
					image->zs.next_out = image->out_buf;
					image->zs.avail_in = data->in_size;
					image->zs.avail_out = data->out_size;
					if (Inflate(&image->zs, Z_SYNC_FLUSH) != Z_STREAM_END) {
						return TDERR_NotSpecified;
					}
					image->data_in_buf = data;
				}
				CopyMem(image->out_buf + to_skip, buffer, to_read);
				break;
		}

		to_skip = 0;
		buffer += to_read;
		size -= to_read;
		io->io_Actual += to_read;

		read_offs += data->out_size;
		data++;
	}
	return IOERR_SUCCESS;
}
