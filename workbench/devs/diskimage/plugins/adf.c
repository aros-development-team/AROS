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
#include <SDI_compiler.h>
#include "device_locale.h"
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("ADF")

extern struct DiskImagePlugin adf_plugin;

PLUGIN_TABLE(&adf_plugin)

struct ADFImage {
	BPTR file;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
};

BOOL ADF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR Generic_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void Generic_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG ADF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG Generic_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
LONG Generic_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin adf_plugin = {
	PLUGIN_NODE(-126, "ADF"),
	PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	NULL,
	NULL,
	ADF_CheckImage,
	Generic_OpenImage,
	Generic_CloseImage,
	ADF_Geometry,
	Generic_Read,
	Generic_Write,
	NULL,
	NULL,
	NULL,
	NULL
};

BOOL ADF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	switch (file_size) {
		case 737280: /* DD disk (PC) */
		case 819200: /* DD disk (Sam Coupe) */
		case 901120: /* DD disk (Amiga) */
		case 1474560: /* HD disk (PC) */
		case 1802240: /* HD disk (Amiga) */
			return TRUE;
		default:
			return FALSE;
	}
}

LONG ADF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct ADFImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads = 2;
	dg->dg_Cylinders = 80;
	dg->dg_TotalSectors = image->total_blocks;
	dg->dg_CylSectors = dg->dg_TotalSectors / dg->dg_Cylinders;
	dg->dg_TrackSectors = dg->dg_CylSectors / dg->dg_Heads;
	return IOERR_SUCCESS;
}
