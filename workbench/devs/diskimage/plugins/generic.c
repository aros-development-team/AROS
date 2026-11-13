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

#ifdef __AROS__
#define __EXEC_NOLIBBASE__
#define __DOS_NOLIBBASE__
#endif

#define USED_PLUGIN_API_VERSION 8
#include <devices/diskimage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <string.h>
#include <SDI_compiler.h>
#include "device_locale.h"
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("Generic")

extern struct DiskImagePlugin generic_plugin;

PLUGIN_TABLE(&generic_plugin)

struct GenericImage {
	BPTR file;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
};

BOOL Generic_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL Generic_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR Generic_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void Generic_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG Generic_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG Generic_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
LONG Generic_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin generic_plugin = {
	PLUGIN_NODE(-127, "Generic"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	Generic_Init,
	NULL,
	Generic_CheckImage,
	Generic_OpenImage,
	Generic_CloseImage,
	Generic_Geometry,
	Generic_Read,
	Generic_Write,
	NULL,
	NULL,
	NULL,
	NULL
};

static struct Library *SysBase;
static struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL Generic_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL Generic_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return TRUE;
}

APTR Generic_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	QUAD file_size;
	struct GenericImage *image = NULL;

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

	image->block_size = 512;
	image->total_blocks = image->total_bytes >> 9;

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

void Generic_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct GenericImage *image = image_ptr;
	if (image) {
		Close(image->file);
		FreeVec(image);
	}
}

LONG Generic_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct GenericImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG Generic_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct GenericImage *image = image_ptr;
	BPTR file = image->file;
	UQUAD offset;
	UBYTE *buffer;
	LONG size, read_size;

	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	buffer = io->io_Data;
	size = io->io_Length;
	io->io_Actual = 0;

	if (!ChangeFilePosition(file, offset, OFFSET_BEGINNING)) {
		return TDERR_SeekError;
	}

	io->io_Actual = size;
	while (size) {
		read_size = Read(file, buffer, size);
		if (read_size == 0) {
			io->io_Actual -= size;
			return IOERR_BADLENGTH;
		} else if (read_size == -1) {
			io->io_Actual -= size;
			return IPlugin_DOS2IOErr(IoErr());
		}
		buffer += read_size;
		size -= read_size;
	}
	return IOERR_SUCCESS;
}

LONG Generic_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct GenericImage *image = image_ptr;
	BPTR file = image->file;
	UQUAD offset;
	UBYTE *buffer;
	LONG size, write_size;

	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	buffer = io->io_Data;
	size = io->io_Length;
	io->io_Actual = 0;

	if (!ChangeFilePosition(file, offset, OFFSET_BEGINNING)) {
		return TDERR_SeekError;
	}

	io->io_Actual = size;
	while (size) {
		write_size = Write(file, buffer, size);
		if (write_size == -1) {
			io->io_Actual -= size;
			return IPlugin_DOS2IOErr(IoErr());
		}
		buffer += write_size;
		size -= write_size;
	}
	return IOERR_SUCCESS;
}
