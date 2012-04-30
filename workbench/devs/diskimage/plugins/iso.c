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

/*
** This plugin handles 2352 bytes per block .iso files as Generic plugin is no
** longer able to handle these
*/

#define USED_PLUGIN_API_VERSION 8
#include <devices/diskimage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("ISO")

extern struct DiskImagePlugin iso_plugin;

PLUGIN_TABLE(&iso_plugin)

struct ISOImage {
	BPTR file;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
	ULONG sector_size;
	ULONG header_part, ecc_part;
};

BOOL ISO_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL ISO_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR ISO_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void Generic_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG Generic_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG ISO_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin iso_plugin = {
	PLUGIN_NODE(-125, "ISO"),
	PLUGIN_FLAG_M68K,
	16,
	ZERO,
	NULL,
	ISO_Init,
	NULL,
	ISO_CheckImage,
	ISO_OpenImage,
	Generic_CloseImage,
	Generic_Geometry,
	ISO_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static struct Library *SysBase;
static struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL ISO_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

static const UBYTE sync_header[16] = {
	0x00, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0x00,
	0x00, 0x02, 0x00, 0x01
};

BOOL ISO_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 16 && !memcmp(test, sync_header, 16);
}

APTR ISO_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	QUAD file_size;
	struct ISOImage *image = NULL;

	file_size = GetFileSize(file);
	if (file_size == -1) {
		error = IoErr();
		goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;
	image->total_bytes = file_size;

	image->block_size = 2048;
	image->sector_size = 2352;
	image->header_part = 16;
	image->ecc_part = image->sector_size - image->header_part - image->block_size;
	image->total_blocks = image->total_bytes / image->sector_size;

	done = TRUE;

error:
	if (!done) {
		if (image) {
			Plugin_CloseImage(Self, image);
			image = NULL;
		} else {
			Close(file);
		}
		IPlugin_SetDiskImageError(unit, error, 0);
	}
	return image;
}

LONG ISO_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct ISOImage *image = image_ptr;
	BPTR file = image->file;
	UQUAD offset;
	UBYTE *buffer;
	LONG size, read_size;
	ULONG start_blk;
	ULONG num_blks;

	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	buffer = io->io_Data;
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 0x7ff) return IOERR_BADADDRESS;
	if (size & 0x7ff) return IOERR_BADLENGTH;

	start_blk = offset >> 11;
	num_blks = size >> 11;

	if (!ChangeFilePosition(file, start_blk*image->sector_size + image->header_part, OFFSET_BEGINNING)) {
		return TDERR_SeekError;
	}

	while (num_blks--) {
		size = 2048;
		while (size) {
			read_size = FRead(file, buffer, 1, size);
			if (read_size == 0) {
				return IOERR_BADLENGTH;
			} else if (read_size == -1) {
				return IPlugin_DOS2IOErr(IoErr());
			}
			buffer += read_size;
			size -= read_size;
			io->io_Actual += read_size;
		}
		if (num_blks && !ChangeFilePosition(file, image->ecc_part + image->header_part, OFFSET_CURRENT)) {
			return TDERR_SeekError;
		}
	}
	return IOERR_SUCCESS;
}
