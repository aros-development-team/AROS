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
#include "device_locale.h"
#include <string.h>
#include "support.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("GI")

extern struct DiskImagePlugin gi_plugin;

PLUGIN_TABLE(&gi_plugin)

BOOL GI_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL GI_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR GI_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void GI_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG GI_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG GI_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin gi_plugin = {
	PLUGIN_NODE(0, "GI"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	GI_Init,
	NULL,
	GI_CheckImage,
	GI_OpenImage,
	GI_CloseImage,
	GI_Geometry,
	GI_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct Library *UtilityBase;
static struct DIPluginIFace *IPlugin;

BOOL GI_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	UtilityBase = data->UtilityBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL GI_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	int len;
	len = strlen(name)-3;
	if (len > 0) {
		name += len;
		return !Stricmp(name, ".gi");
	}
	return FALSE;
}

struct FileNode {
	struct FileNode *next;
	BPTR file;
	UQUAD offset;
	UQUAD size;
};

struct GIImage {
	struct FileNode *files;
	ULONG block_size;
	UQUAD total_bytes;
	ULONG total_blocks;
};

APTR GI_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct GIImage *image = NULL;
	CONST_STRPTR ext;
	LONG len, i;
	STRPTR namebuf = NULL, extbuf;
	CONST_STRPTR fmt = "%ld.gi";
	struct FileNode *fn;

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	ext = strrchr(name, '.');
	len = ext - name;

	namebuf = AllocVec(len + 8, MEMF_ANY);
	image->files = fn = AllocVec(sizeof(*fn), MEMF_CLEAR);
	if (!namebuf || !fn) {
		error = ERROR_NO_FREE_STORE;
		Close(file);
		goto error;
	}

	CopyMem(name, namebuf, len);
	extbuf = namebuf + len;

	i = 1;
	for (;;) {
		fn->file = file;
		fn->offset = image->total_bytes;
		fn->size = GetFileSize(file);	
		if (fn->size == -1) {
			error = IoErr();
			goto error;
		}
		image->total_bytes += fn->size;

		SNPrintf(extbuf, 8, fmt, i++);
		file = Open(namebuf, MODE_OLDFILE);
		if (!file) {
			error = IoErr();
			if (error != ERROR_OBJECT_NOT_FOUND) {
				goto error;
			}
			error = NO_ERROR;
			break;
		}
		fn->next = AllocVec(sizeof(*fn), MEMF_CLEAR);
		if (!(fn = fn->next)) {
			error = ERROR_NO_FREE_STORE;
			Close(file);
			goto error;
		}
	}

	image->block_size = 2048;
	image->total_blocks = image->total_bytes >> 11;

	done = TRUE;

error:
	FreeVec(namebuf);
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

void GI_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct GIImage *image = image_ptr;
	if (image) {
		struct FileNode *fn, *next;
		fn = image->files;
		while (fn) {
			next = fn->next;
			Close(fn->file);
			FreeVec(fn);
			fn = next;
		}
		FreeVec(image);
	}
}

LONG GI_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct GIImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG GI_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct GIImage *image = image_ptr;
	UBYTE *buffer;
	UQUAD offset, in_offs;
	ULONG size, to_read;
	LONG status;
	struct FileNode *fn;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset >= image->total_bytes) return TDERR_SeekError;

	fn = image->files;
	while (fn && fn->offset + fn->size <= offset) fn = fn->next;
	if (!fn) return TDERR_SeekError;

	in_offs = offset - fn->offset;
	if (in_offs && !ChangeFilePosition(fn->file, in_offs, OFFSET_BEGINNING))	{
		return TDERR_SeekError;
	}

	while (size) {
		if (!fn) return IOERR_BADLENGTH;

		to_read = min(size, fn->size - in_offs);
		status = Read(fn->file, buffer, to_read);
		if (status == -1) {
			return IPlugin_DOS2IOErr(IoErr());
		} else
		if (status != to_read) {
			return IOERR_BADLENGTH;
		}
		in_offs = 0;

		buffer += to_read;
		size -= to_read;
		io->io_Actual += to_read;
		fn = fn->next;
	}
	return IOERR_SUCCESS;
}
