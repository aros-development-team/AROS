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
#include <proto/diskimage.h>
#include <proto/exec.h>
#include <proto/dos.h>

#ifdef __AROS__
#  include <zlib.h>
#  define InflateInit2 inflateInit2
#  define Inflate inflate
#  define InflateEnd inflateEnd
#  define InflateReset inflateReset
#else
#  include <libraries/z.h>
#  include <proto/z.h>
#endif

#include <SDI_compiler.h>
#include "endian.h"
#include "device_locale.h"
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("CISO")

extern struct DiskImagePlugin ciso_plugin;

PLUGIN_TABLE(&ciso_plugin)

struct CISOImage {
	BPTR file;
	ULONG block_size;
	ULONG total_blocks;
	UBYTE align;
	UBYTE *block_buf;
	z_stream zs;
	ULONG *index_buf;
	struct Library *zbase;
};

BOOL CISO_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL CISO_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR CISO_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CISO_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CISO_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CISO_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin ciso_plugin = {
	PLUGIN_NODE(0, "CISO"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	CISO_Init,
	NULL,
	CISO_CheckImage,
	CISO_OpenImage,
	CISO_CloseImage,
	CISO_Geometry,
	CISO_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;
#ifndef __AROS__
#define ZBase image->zbase
#else
struct Library *Z1Base;
#endif

BOOL CISO_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define CISO_MAGIC MAKE_ID('C','I','S','O')

BOOL CISO_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= sizeof(ULONG) && rbe32(test) == CISO_MAGIC;
}

#pragma pack(1)

typedef struct {
	ULONG magic;
	ULONG header_size;
	UQUAD total_bytes;
	ULONG block_size;
	UBYTE version;
	UBYTE align;
	UBYTE reserved[2];
} ciso_t;

#pragma pack()

APTR CISO_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CISOImage *image = NULL;
	ciso_t ciso;
	ULONG block_size;
	ULONG total_blocks;
	ULONG index_size;
	ULONG i;

	if (Read(file, &ciso, sizeof(ciso)) != sizeof(ciso)) {
		error = IoErr();
		goto error;
	}
	if (ciso.block_size == 0 || ciso.total_bytes == 0) {
		error = ERROR_BAD_NUMBER;
		goto error;
	}

	block_size = rle32(&ciso.block_size);
	total_blocks = rle64(&ciso.total_bytes) / block_size;
	index_size = (total_blocks + 1) << 2;

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->file = file;
	image->block_size = block_size;
	image->total_blocks = total_blocks;
	image->align = ciso.align;

#ifdef __AROS__
	image->zbase = OpenLibrary("z1.library", 1);
	Z1Base = image->zbase;
#else
	image->zbase = OpenLibrary("z.library", 1);
#endif
	if (!image->zbase || !CheckLib(image->zbase, 1, 6)) {
		error = ERROR_OBJECT_NOT_FOUND;
		error_string = MSG_REQVER;
		error_args[0] = (IPTR)"z.library";
		error_args[1] = 1;
		error_args[2] = 6;
		goto error;
	}

	image->index_buf = AllocVec(index_size, MEMF_ANY);
	image->block_buf = AllocVec(block_size << 1, MEMF_ANY);
	if (!image->index_buf || !image->block_buf) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (InflateInit2(&image->zs, -15) != Z_OK) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_ZLIBERR;
		goto error;
	}

	if (Read(file, image->index_buf, index_size) != index_size) {
		error = IoErr();
		goto error;
	}

	for (i = 0; i <= total_blocks; i++) {
		image->index_buf[i] = rle32(&image->index_buf[i]);
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

void CISO_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct CISOImage *image = image_ptr;
	if (image) {
		if (image->zbase) {
			if (CheckLib(image->zbase, 1, 6)) InflateEnd(&image->zs);
			CloseLibrary(image->zbase);
		}
		FreeVec(image->block_buf);
		FreeVec(image->index_buf);
		Close(image->file);
		FreeVec(image);
	}
}

LONG CISO_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct CISOImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG CISO_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CISOImage *image = image_ptr;
	UBYTE *buffer;
	UQUAD offset;
	ULONG size;
	ULONG block_size = image->block_size;
	UBYTE align = image->align;
	UBYTE *block_buf = image->block_buf;
	ULONG *index_buf, index, index2, plain;
	UBYTE *read_buf;
	ULONG read_pos, read_size;
	LONG error = IOERR_SUCCESS;
	BPTR file = image->file;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset % block_size) return IOERR_BADADDRESS;
	if (size % block_size) return IOERR_BADLENGTH;
	
	offset /= block_size;
	size /= block_size;
	if (offset >= image->total_blocks) {
		return IOERR_BADADDRESS;
	}
	if (offset + size > image->total_blocks) {
		size = image->total_blocks - offset;
		error = IOERR_BADLENGTH;
	}

	index_buf = image->index_buf + offset;
	while (size--) {
		index = *index_buf++;
		plain = index & 0x80000000;
		index -= plain;
		read_pos = index << align;
		if (plain) {
			read_size = block_size;
			read_buf = buffer;
		} else {
			index2 = *index_buf & 0x7fffffff;
			read_size = (index2 - index) << align;
			read_buf = block_buf;
		}
		if (!ChangeFilePosition(file, read_pos, OFFSET_BEGINNING) ||
			Read(file, read_buf, read_size) != read_size)
		{
			return IPlugin_DOS2IOErr(IoErr());
		}
		if (!plain) {
			InflateReset(&image->zs);
			image->zs.next_in = read_buf;
			image->zs.avail_in = read_size;
			image->zs.next_out = buffer;
			image->zs.avail_out = block_size;
			if (Inflate(&image->zs, Z_SYNC_FLUSH) != Z_STREAM_END) {
				return TDERR_NotSpecified;
			}
		}
		buffer += block_size;
		io->io_Actual += block_size;
	}
	return error;
}

