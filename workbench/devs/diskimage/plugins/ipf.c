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
#include <caps/capsimage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/capsimage.h>
#include "device_locale.h"
#include "endian.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("IPF")

extern struct DiskImagePlugin ipf_plugin;

PLUGIN_TABLE(&ipf_plugin)

struct IPFImage {
	CapsLong id, lock;
	UBYTE track_sectors;
	struct CapsImageInfo image_info;
	struct CapsTrackInfoT1 track_info;
	struct MsgPort *caps_mp;
	struct IORequest *caps_io;
	struct Device *capsimagebase;
	CapsLong caps_init;
};

BOOL IPF_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL IPF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR IPF_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void IPF_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG IPF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG IPF_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
static LONG CAPSError (LONG error);

struct DiskImagePlugin ipf_plugin = {
	PLUGIN_NODE(0, "IPF"),
	PLUGIN_FLAG_M68K,
	4,
	ZERO,
	NULL,
	IPF_Init,
	NULL,
	IPF_CheckImage,
	IPF_OpenImage,
	IPF_CloseImage,
	IPF_Geometry,
	IPF_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static struct Library *SysBase;
static struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;

BOOL IPF_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define IPF_MAGIC MAKE_ID('C','A','P','S')

BOOL IPF_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= sizeof(ULONG) && rbe32(test) == IPF_MAGIC;
}

APTR IPF_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct IPFImage *image = NULL;
	struct Device *CapsImageBase;
	CapsLong caps_err;

	Close(file);

	image = AllocVec(sizeof(*image), MEMF_ANY);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->id = -1;

	image->caps_mp = CreateMsgPort();
	image->caps_io = CreateIORequest(image->caps_mp, sizeof(struct IORequest));
	if (!image->caps_io) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (OpenDevice("capsimage.device", 0, image->caps_io, 0) != IOERR_SUCCESS) {
		error = ERROR_OBJECT_NOT_FOUND;
		error_string = MSG_REQ;
		error_args[0] = (IPTR)"capsimage.device";
		goto error;
	}
	image->capsimagebase = image->caps_io->io_Device;
	CapsImageBase = image->capsimagebase;

	image->caps_init = CAPSInit();
	if (image->caps_init != imgeOk) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->id = CAPSAddImage();
	if (image->id < 0) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->lock = CAPSLockImage(image->id, name);
	if (image->lock != imgeOk) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_CAPSERR;
		goto error;
	}
	caps_err = CAPSGetImageInfo(&image->image_info, image->id);
	if (caps_err != imgeOk) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_CAPSERR;
		goto error;
	}
	image->track_info.cylinder = -1;
	image->track_info.head = -1;
	image->track_sectors = 11;

	done = TRUE;

error:
	if (!done) {
		if (image) {
			Plugin_CloseImage(Self, image);
			image = NULL;
		}
		IPlugin_SetDiskImageErrorA(unit, error, error_string, error_args);
	}
	return image;
}

void IPF_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct IPFImage *image = image_ptr;
	if (image) {
		if (image->capsimagebase) {
			struct Device *CapsImageBase = image->capsimagebase;
			if (image->caps_init == imgeOk) {
				if (image->id >= 0) {
					if (image->lock == imgeOk) {
						CAPSUnlockImage(image->id);
					}
					CAPSRemImage(image->id);
				}
				CAPSExit();
			}
			CloseDevice(image->caps_io);
		}
		DeleteIORequest(image->caps_io);
		DeleteMsgPort(image->caps_mp);
		FreeVec(image);
	}
}

LONG IPF_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct IPFImage *image = image_ptr;
	dg->dg_Cylinders = 80;
	dg->dg_Heads = 2;
	dg->dg_TrackSectors = image->track_sectors;
	dg->dg_CylSectors = dg->dg_TrackSectors << 1;
	dg->dg_TotalSectors = dg->dg_CylSectors * 80;
	return IOERR_SUCCESS;
}

static LONG read_sector (UBYTE *buf, int track, int sector, UWORD *data, LONG len);
static UWORD getmfmword (const UWORD *mbuf, ULONG shift);
static ULONG getmfmlong (const UWORD *mbuf, ULONG shift);

static const CapsULong caps_flags = DI_LOCK_DENVAR|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD|DI_LOCK_TYPE;

LONG IPF_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct IPFImage *image = image_ptr;
	struct Device *CapsImageBase = image->capsimagebase;
	UBYTE *buffer;
	ULONG offset, size;
	LONG track, cyl, head, block, sectors;
	CapsLong id = image->id;
	struct CapsTrackInfoT1 *ti = &image->track_info;
	CapsLong caps_err;
	LONG err;

	buffer = io->io_Data;
	offset = io->io_Offset;
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 511) return IOERR_BADADDRESS;
	if (size & 511) return IOERR_BADLENGTH;

	offset >>= 9;
	size >>= 9;

	sectors = image->track_sectors;
	track = offset / sectors;
	block = offset % sectors;

	while (size) {
		cyl = track >> 1;
		head = track & 1;
		ti->type = 1;
		caps_err = CAPSLockTrack((struct CapsTrackInfo *)ti, id, cyl, head, caps_flags);
		if (caps_err != imgeOk) return CAPSError(caps_err);

		for (; size && block < sectors; block++, size--) {
			err = read_sector(buffer, track, block, (UWORD *)ti->trackbuf, ti->tracklen);
			if (err != IOERR_SUCCESS) {
				CAPSUnlockTrack(id, cyl, head);
				return err;
			}
			buffer += 512;
			io->io_Actual += 512;
		}

		CAPSUnlockTrack(id, cyl, head);
		block = 0;
		track++;
	}
	return IOERR_SUCCESS;
}

static LONG CAPSError (LONG error) {
	switch (error) {
		case imgeOk:
			return IOERR_SUCCESS;
		default:
			return TDERR_NotSpecified;
	}
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
