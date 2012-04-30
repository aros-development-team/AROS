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
#include <string.h>
#include "endian.h"
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("CPC")

extern struct DiskImagePlugin cpc_plugin;

PLUGIN_TABLE(&cpc_plugin)

BOOL CPC_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL CPC_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR CPC_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void CPC_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG CPC_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG CPC_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
LONG CPC_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin cpc_plugin = {
	PLUGIN_NODE(0, "CPC"),
	PLUGIN_FLAG_M68K,
	8,
	ZERO,
	NULL,
	CPC_Init,
	NULL,
	CPC_CheckImage,
	CPC_OpenImage,
	CPC_CloseImage,
	CPC_Geometry,
	CPC_Read,
	CPC_Write,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL CPC_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL CPC_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 8 && (!memcmp(test, "MV - CPC", 8) || !memcmp(test, "EXTENDED", 8));
}

struct CPC {
	BPTR file;
	ULONG *track_offsets;
	UWORD track_size;
	UWORD sector_size;
	UBYTE tracks;
	UBYTE cylinders;
	UBYTE heads;
	UBYTE sectors;
};

APTR CPC_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CPC *image = NULL;
	UBYTE buf[22];
	ULONG *ptr;
	UWORD tracks;
	ULONG track_offs, track_size;

	if (Read(file, buf, 8) != 8) {
		error = IoErr();
		goto error;
	}
	if (!memcmp(buf, "EXTENDED", 8)) {
		error = ERROR_NOT_IMPLEMENTED;
		goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;

	if (!ChangeFilePosition(file, 40, OFFSET_CURRENT) ||
		Read(file, buf, 4) != 4)
	{
		error = IoErr();
		goto error;
	}
	image->cylinders = buf[0];
	image->heads = buf[1];
	image->tracks = image->cylinders * image->heads;
	image->track_size = track_size = rle16(&buf[2]);

	if (!image->tracks || !image->heads || image->track_size < 256) {
		error = ERROR_BAD_NUMBER;
		goto error;
	}
	image->track_size -= 256;

	image->track_offsets = AllocVec(image->tracks << 2, MEMF_ANY);
	if (!image->track_offsets) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (!ChangeFilePosition(file, 204, OFFSET_CURRENT)) {
		error = IoErr();
		goto error;
	}

	tracks = image->tracks;
	ptr = image->track_offsets;
	track_offs = 512;
	while (tracks--) {
		*ptr++ = track_offs;
		track_offs += track_size;

		if (Read(file, buf, 22) != 22) {
			error = IoErr();
			goto error;
		}
		if (memcmp(buf, "Track-Info\r\n", 12)) {
			error = ERROR_OBJECT_WRONG_TYPE;
			goto error;
		}
		if (!image->sectors) {
			image->sectors = buf[21];
			image->sector_size = 128 << buf[20];
		}

		if (!ChangeFilePosition(file, track_size-22, OFFSET_CURRENT)) {
			error = IoErr();
			goto error;
		}
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

void CPC_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct CPC *image = image_ptr;
	if (image) {
		FreeVec(image->track_offsets);
		Close(image->file);
		FreeVec(image);
	}
}

LONG CPC_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct CPC *image = image_ptr;
	dg->dg_SectorSize = image->sector_size;
	dg->dg_Cylinders = image->cylinders;
	dg->dg_Heads = image->heads;
	dg->dg_TrackSectors = image->track_size / image->sector_size;
	dg->dg_CylSectors = dg->dg_TrackSectors * dg->dg_Heads;
	dg->dg_TotalSectors = dg->dg_CylSectors * dg->dg_Cylinders;
	return IOERR_SUCCESS;
}

LONG CPC_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CPC *image = image_ptr;
	BPTR file = image->file;
	UBYTE *buffer;
	ULONG offset;
	ULONG size;
	ULONG track, track_size;
	ULONG to_skip, to_read;
	ULONG *ptr;
	LONG status;

	buffer = io->io_Data;
	offset = io->io_Offset;
	size = io->io_Length;
	io->io_Actual = 0;

	track_size = image->track_size;
	track = offset / track_size;
	to_skip = offset % track_size;
	if (track >= image->tracks) return IOERR_BADADDRESS;

	ptr = image->track_offsets + track;
	io->io_Actual = size;
	while (size) {
		if (track >= image->tracks) {
			io->io_Actual -= size;
			return IOERR_BADLENGTH;
		}

		if (!ChangeFilePosition(file, (*ptr++) + to_skip, OFFSET_BEGINNING)) {
			io->io_Actual -= size;
			return TDERR_SeekError;
		}

		to_read = min(size, track_size - to_skip);
		status = Read(file, buffer, to_read);
		if (status == -1) {
			io->io_Actual -= size;
			return IPlugin_DOS2IOErr(IoErr());
		} else
		if (status != to_read) {
			io->io_Actual -= size;
			return IOERR_BADLENGTH;
		}

		buffer += to_read;
		size -= to_read;
	}
	return IOERR_SUCCESS;
}

LONG CPC_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CPC *image = image_ptr;
	BPTR file = image->file;
	UBYTE *buffer;
	ULONG offset;
	ULONG size;
	ULONG track, track_size;
	ULONG to_skip, to_write;
	ULONG *ptr;
	LONG status;

	buffer = io->io_Data;
	offset = io->io_Offset;
	size = io->io_Length;
	io->io_Actual = 0;

	track_size = image->track_size;
	track = offset / track_size;
	to_skip = offset % track_size;
	if (track >= image->tracks) return IOERR_BADADDRESS;

	ptr = image->track_offsets + track;
	io->io_Actual = size;
	while (size) {
		if (track >= image->tracks) {
			io->io_Actual -= size;
			return IOERR_BADLENGTH;
		}

		if (!ChangeFilePosition(file, (*ptr++) + to_skip, OFFSET_BEGINNING)) {
			io->io_Actual -= size;
			return TDERR_SeekError;
		}

		to_write = min(size, track_size - to_skip);
		status = Write(file, buffer, to_write);
		if (status != to_write) {
			io->io_Actual -= size;
			return IPlugin_DOS2IOErr(IoErr());
		}

		buffer += to_write;
		size -= to_write;
	}
	return IOERR_SUCCESS;
}
