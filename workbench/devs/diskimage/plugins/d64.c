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
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("D64")

extern struct DiskImagePlugin d64_plugin;

PLUGIN_TABLE(&d64_plugin)

struct D64Image {
	BPTR file;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
};

BOOL D64_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL D64_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR D64_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void Generic_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG Generic_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG Generic_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
LONG Generic_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin d64_plugin = {
	PLUGIN_NODE(-126, "D64"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	D64_Init,
	NULL,
	D64_CheckImage,
	D64_OpenImage,
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

BOOL D64_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL D64_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	switch (file_size) {
		case 174848:
		case 175531:
		case 196608:
		case 197376:
			return TRUE;
		default:
			return FALSE;
	}
}

APTR D64_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	QUAD file_size;
	struct D64Image *image = NULL;

	file_size = GetFileSize(file);
	if (file_size == -1) {
		error = IoErr();
		goto error;
	}

	/* some .d64 disk images have extra data at the end of the file */
	switch (file_size) {
		case 174848:
		case 175531:
			file_size = 174848;
			break;
		case 196608:
		case 197376:
			file_size = 196608;
			break;
		default:
			error = ERROR_OBJECT_WRONG_TYPE;
			goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;
	image->total_bytes = file_size;

	image->block_size = 256;
	image->total_blocks = image->total_bytes >> 8;

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
