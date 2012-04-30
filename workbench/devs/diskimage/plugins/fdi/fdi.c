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
#include "fdi2raw.h"
#include "device_locale.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("FDI")

extern struct DiskImagePlugin fdi_plugin;

PLUGIN_TABLE(&fdi_plugin)

BOOL FDI_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
void FDI_Exit (struct DiskImagePlugin *Self);
BOOL FDI_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR FDI_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void FDI_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG FDI_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG FDI_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin fdi_plugin = {
	PLUGIN_NODE(0, "FDI"),
	PLUGIN_FLAG_M68K,
	27,
	ZERO,
	NULL,
	FDI_Init,
	FDI_Exit,
	FDI_CheckImage,
	FDI_OpenImage,
	FDI_CloseImage,
	FDI_Geometry,
	FDI_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
struct DIPluginIFace *IPlugin;
#ifndef __AROS__
struct Library *MathIeeeDoubBasBase;
#endif

BOOL FDI_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

void FDI_Exit (struct DiskImagePlugin *Self) {
#ifndef __AROS__
	if (MathIeeeDoubBasBase) CloseLibrary(MathIeeeDoubBasBase);
#endif
}

BOOL FDI_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 27 && !memcmp(test, "Formatted Disk Image file\r\n", 27);
}

struct FDIImage {
	BPTR file;
	FDI *fdi;
	ULONG tracks;
	ULONG heads;
	ULONG sectors;
	UWORD *mfmbuf;
	UWORD *tracktiming;
	ULONG track_in_buf;
	ULONG mfmbuf_size;
	ULONG track_size;
	ULONG tracklen;
};

APTR FDI_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct FDIImage *image = NULL;

#ifndef __AROS__
	if (!MathIeeeDoubBasBase) {
		MathIeeeDoubBasBase = OpenLibrary("mathieeedoubbas.library", 0);
		if (!MathIeeeDoubBasBase) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQ;
			error_args[0] = (IPTR)"mathieeedoubbas.library";
			goto error;
		}
	}
#endif
	
	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;

	image->fdi = fdi2raw_header(image->file);
	if (!image->fdi) {
		error = IoErr();
		goto error;
	}

	image->tracks = fdi2raw_get_last_track(image->fdi);
	image->heads = fdi2raw_get_last_head(image->fdi)+1;
	image->sectors = fdi2raw_get_num_sector(image->fdi);
	image->track_size = image->sectors << 9;

	image->mfmbuf_size = 0x8000;
	if (image->sectors == 22) image->mfmbuf_size *= 2;

	image->mfmbuf = AllocVec(image->mfmbuf_size, MEMF_ANY);
	image->tracktiming = AllocVec(image->mfmbuf_size, MEMF_CLEAR);
	if (!image->mfmbuf || !image->tracktiming) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->track_in_buf = ~0;

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

void FDI_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct FDIImage *image = image_ptr;
	if (image) {
		if (image->fdi) fdi2raw_header_free(image->fdi);
		FreeVec(image->mfmbuf);
		FreeVec(image->tracktiming);
		Close(image->file);
		FreeVec(image);
	}
}

LONG FDI_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct FDIImage *image = image_ptr;
	dg->dg_SectorSize = 512;
	dg->dg_Heads = image->heads;
	dg->dg_Cylinders = image->tracks / image->heads;
	dg->dg_TrackSectors = image->sectors;
	dg->dg_CylSectors = image->sectors * image->heads;
	dg->dg_TotalSectors = image->sectors * image->tracks;
	return IOERR_SUCCESS;
}

static LONG read_sector (UBYTE *buf, int track, int sector, UWORD *data, LONG len);
static UWORD getmfmword (const UWORD *mbuf, ULONG shift);
static ULONG getmfmlong (const UWORD *mbuf, ULONG shift);

LONG FDI_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct FDIImage *image = image_ptr;
	UBYTE *buffer;
	ULONG offset, size;
	LONG track, block, sectors;
	unsigned int tracklen;
	FDI *fdi = image->fdi;
	UWORD *mfmbuf = image->mfmbuf;
	LONG error;

	buffer = io->io_Data;
	offset = io->io_Offset;
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 511) return IOERR_BADADDRESS;
	if (size & 511) return IOERR_BADLENGTH;

	offset >>= 9;
	size >>= 9;

	sectors = image->sectors;
	track = offset / sectors;
	block = offset % sectors;

	while (size) {
		if (track != image->track_in_buf) {

			if (track >= image->tracks) return TDERR_SeekError;

			tracklen = 0;
			if (!fdi2raw_loadtrack(fdi, mfmbuf, image->tracktiming,
				track, &tracklen, NULL, NULL, 1) || !tracklen)
			{
				image->track_in_buf = ~0;
				return TDERR_NotSpecified;
			}
			image->tracklen = tracklen >>= 3;
			image->track_in_buf = track;
		} else {
			tracklen = image->tracklen;
		}

		for (; size && block < sectors; block++, size--) {
			error = read_sector(buffer, track, block, mfmbuf, tracklen);
			if (error) {
				/*DebugPrintF("error: %ld, track: %ld, block: %ld, tracklen: %ld\n",
					error, track, block, tracklen);*/
				return error;
			}
			buffer += 512;
			io->io_Actual += 512;
		}

		block = 0;
		track++;
	}
	return IOERR_SUCCESS;
}

static LONG read_sector (UBYTE *buf, int track, int sector, UWORD *data, LONG len) {
	ULONG shift = 0;
	ULONG odd, even, chksum, id, dlong;
	UWORD *end;
	ULONG i;

	end = data + ((len+1) >> 1);
	end -= 540;

	while (data <= end) {
		while (getmfmword(data, shift) != 0x4489) {
			if (data >= end) {
				return TDERR_NoSecHdr;
			}
			shift++;
			if (shift == 16) {
				shift = 0;
				data++;
			}
		}
		while (getmfmword(data, shift) == 0x4489) {
			if (data >= end) return TDERR_BadSecPreamble;
			data++;
		}

		odd = getmfmlong(data, shift);
		even = getmfmlong(data + 2, shift);
		data += 4;
		id = (odd << 1)|even;
		chksum = odd ^ even;
		for (i = 0; i < 4; i++) {
			odd = getmfmlong(data, shift);
			even = getmfmlong(data + 8, shift);
			data += 2;

			dlong = (odd << 1)|even;
			chksum ^= odd ^ even;
		}
		data += 8;
		odd = getmfmlong(data, shift);
		even = getmfmlong(data + 2, shift);
		data += 4;
		if (((odd << 1)|even) != chksum ||
			((id & 0xff000000) >> 24) != 0xff ||
			((id & 0x00ff0000) >> 16) != track)
		{
			return TDERR_BadSecHdr;
		}

		odd = getmfmlong(data, shift);
		even = getmfmlong(data + 2, shift);
		data += 4;
		chksum = (odd << 1)|even;
		if (((id & 0x0000ff00) >> 8) == sector) {
			for (i = 0; i < 128; i++) {
				odd = getmfmlong(data, shift);
				even = getmfmlong(data + 256, shift);
				data += 2;
				dlong = (odd << 1)|even;
				wbe32(buf, dlong);
				buf += 4;
				chksum ^= odd ^ even;
			}
			if (chksum) {
				return TDERR_BadSecSum;
			}
			return IOERR_SUCCESS;
		}
		data += 512;
	}

	return TDERR_NoSecHdr;
}

#define MFMMASK 0x55555555

static UWORD getmfmword (const UWORD *mbuf, ULONG shift) {
    return (rbe16(&mbuf[0]) << shift) | (rbe16(&mbuf[1]) >> (16 - shift));
}

static ULONG getmfmlong (const UWORD *mbuf, ULONG shift) {
    return ((getmfmword (mbuf, shift) << 16) | getmfmword (mbuf + 1, shift)) & MFMMASK;
}
