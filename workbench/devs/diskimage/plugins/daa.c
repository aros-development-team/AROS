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
#include <string.h>

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

#include "endian.h"
#include "device_locale.h"
#include "support.h"
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("DAA")

extern struct DiskImagePlugin daa_plugin;

PLUGIN_TABLE(&daa_plugin)

typedef struct daa_file_s {
	struct daa_file_s *next;
	BPTR file;
	UQUAD size;
	ULONG offset;
} daa_file_t;

typedef struct {
	daa_file_t *file;
	ULONG size;
	UQUAD offset;
} daa_data_t;

struct DAAImage {
	daa_file_t *files;
	daa_data_t *data;
	z_stream zs;
	UBYTE *in_buf, *out_buf;
	ULONG in_size, out_size;
	daa_data_t *data_in_buf;
	UQUAD total_bytes;
	ULONG block_size;
	ULONG total_blocks;
	struct Library *zbase;
};

BOOL DAA_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL DAA_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR DAA_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void DAA_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG DAA_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG DAA_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin daa_plugin = {
	PLUGIN_NODE(0, "DAA"),
	PLUGIN_FLAG_M68K,
	8,
	ZERO,
	NULL,
	DAA_Init,
	NULL,
	DAA_CheckImage,
	DAA_OpenImage,
	DAA_CloseImage,
	DAA_Geometry,
	DAA_Read,
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
#ifndef __AROS__
#define ZBase image->zbase
#else
struct Library *Z1Base;
#endif

BOOL DAA_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	UtilityBase = data->UtilityBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

BOOL DAA_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 8 && (!strcmp(test, "DAA") || !strcmp(test, "DAA VOL"));
}

#pragma pack(1)

typedef struct {
    UBYTE   sign[16];       // 00: DAA
    ULONG  size_offset;    // 10: where starts the list of sizes of the zipped chunks
    ULONG  b100;           // 14: must be 0x100
    ULONG  data_offset;    // 18: where starts the zipped chunks
    ULONG  b1;             // 1C: must be 1
    ULONG  b0;             // 20: ever 0
    ULONG  chunksize;      // 24: size of each output chunk
    UQUAD  isosize;        // 28: total size of the output ISO
    UQUAD  filesize;       // 30: total size of the DAA file
    UBYTE   zero[16];       // 38: it's ever zero
    ULONG  crc;            // 48: checksum calculated on the first 0x48 bytes
} daa_t;

#pragma pack()

static CONST_STRPTR find_ext (CONST_STRPTR fname, CONST_STRPTR ext) {
	LONG len, extlen;
	CONST_STRPTR ret;
	len = strlen(fname);
	extlen = strlen(ext);
	ret = fname+len-extlen;
	if (len >= extlen && !Stricmp(ret, ext)) {
		return ret;
	}
	return NULL;
}

APTR DAA_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name) {
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct DAAImage *image = NULL;
	daa_t daa;
	UQUAD offset;
	ULONG size_offset;
	ULONG type, len;
	LONG multi = FALSE, multinum = 0;
	daa_file_t *fn;
	STRPTR namebuf = NULL, extbuf = NULL;
	CONST_STRPTR fmt = NULL;
	ULONG daa_size, daas, i;
	UBYTE *daa_data = NULL, *src;
	daa_data_t *data;

	if (FRead(file, &daa, 1, sizeof(daa)) != sizeof(daa)) {
		error = IoErr();
		goto error;
	}
	if (!strcmp(daa.sign, "DAA VOL")) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_WRONGDAA;
		goto error;
	}
	if (strcmp(daa.sign, "DAA") ||
		rle32(&daa.b100) != 0x100 ||
		rle32(&daa.b1) != 1)
	{
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	offset = sizeof(daa);
	size_offset = rle32(&daa.size_offset);
	while (offset < size_offset) {
		if (FRead(file, &type, 1, 4) != 4 ||
			FRead(file, &len, 1, 4) != 4)
		{
			error = IoErr();
			goto error;
		}

		switch (type) {
			case 1:
				multi = TRUE;
				break;
			case 3:
				error = ERROR_OBJECT_WRONG_TYPE;
				goto error;
		}

		if (!ChangeFilePosition(file, len-8, OFFSET_CURRENT)) {
			error = IoErr();
			goto error;
		}
		offset += len;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	image->files =
	fn = AllocVec(sizeof(*fn), MEMF_CLEAR);
	if (!fn) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	fn->file = file;
	fn->size = GetFileSize(file);
	fn->offset = rle32(&daa.data_offset);
	if (fn->size == (UQUAD)-1) {
		error = IoErr();
		goto error;
	}

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

	if (multi || fn->size != rle64(&daa.filesize)) {
		CONST_STRPTR p;
		if ((p = find_ext(name, "001.daa"))) {
			multi = 1;
			multinum = 2;
		} else
		if ((p = find_ext(name, "01.daa"))) {
			multi = 2;
			multinum = 2;
		} else {
			multi = 3;
			multinum = 0;
			p = strrchr(name, '.');
			if (!p) p = name + strlen(name);
		}
		len = p - name;
		namebuf = AllocVec(len + 16, MEMF_ANY);
		if (!namebuf) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}
		memcpy(namebuf, name, len);
		extbuf = namebuf + len;
		switch (multi) {
			case 1: fmt = "%03ld.daa"; break;
			case 2: fmt = "%02ld.daa"; break;
			default: fmt = ".d%02ld"; break;
		}
	}

	daa_size = rle32(&daa.data_offset) - size_offset;
	daas = daa_size / 3;
	image->out_size = rle32(&daa.chunksize);
	image->block_size = 2048;
	image->total_bytes = daas * image->out_size;
	image->total_blocks = image->total_bytes >> 11;

	daa_data = AllocVec(daa_size, MEMF_ANY);
	image->data = data = AllocVec(daas * sizeof(*image->data), MEMF_ANY);
	image->out_buf = AllocVec(image->out_size, MEMF_ANY);
	if (!daa_data || !data || !image->out_buf) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (!ChangeFilePosition(file, size_offset - offset, OFFSET_CURRENT) ||
		FRead(file, daa_data, 1, daa_size) != daa_size)
	{
		error = IoErr();
		goto error;
	}

	offset = fn->offset;
	src = daa_data;
	for (i = 0; i < daas; i++) {
		while (offset >= fn->size) {
			offset -= fn->size;

			if (!multi) {
				error = ERROR_OBJECT_WRONG_TYPE;
				goto error;
			}

			fn->next = AllocVec(sizeof(*fn), MEMF_CLEAR);
			if (!(fn = fn->next)) {
				error = ERROR_NO_FREE_STORE;
				goto error;
			}

			SNPrintf(extbuf, 16, fmt, multinum++);
			fn->file = file = Open(namebuf, MODE_OLDFILE);
			if (!file ||
				(fn->size = GetFileSize(file)) == (UQUAD)-1 ||
				Read(file, &daa, sizeof(daa)) != sizeof(daa))
			{
				error = IoErr();
				goto error;
			}
			if (strcmp(daa.sign, "DAA VOL")) {
				error = ERROR_OBJECT_WRONG_TYPE;
				goto error;
			}

			fn->offset = rle32(&daa.size_offset);
			offset += fn->offset;
			if (!ChangeFilePosition(file, fn->offset, OFFSET_BEGINNING)) {
				error = IoErr();
				goto error;
			}
		}

		len = (src[0] << 16)|(src[2] << 8)|(src[1]);
		src += 3;

		data->file = fn;
		data->size = len;
		data->offset = offset;
		offset += len;
		data++;
	}

	if (InflateInit2(&image->zs, -15) != Z_OK) {
		error = ERROR_OBJECT_WRONG_TYPE;
		error_string = MSG_ZLIBERR;
		goto error;
	}

	done = TRUE;

error:
	FreeVec(daa_data);
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

void DAA_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct DAAImage *image = image_ptr;

	if (image) {
		daa_file_t *fn, *next;
		if (image->zbase) {
			if (CheckLib(image->zbase, 1, 6)) InflateEnd(&image->zs);
			CloseLibrary(image->zbase);
		}
		FreeVec(image->in_buf);
		FreeVec(image->out_buf);
		for (fn = image->files; fn; fn = next) {
			next = fn->next;
			Close(fn->file);
			FreeVec(fn);
		}
		FreeVec(image->data);
		FreeVec(image);
	}
}

LONG DAA_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct DAAImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG DAA_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct DAAImage *image = image_ptr;
	UBYTE *buffer;
	UQUAD offset;
	ULONG block, to_read, to_skip;
	ULONG size;
	ULONG out_size = image->out_size;
	daa_data_t *data;
	daa_file_t *fn;
	LONG error = IOERR_SUCCESS;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset >= image->total_bytes) return TDERR_SeekError;
	if (offset + size > image->total_bytes) {
		error = IOERR_BADLENGTH;
		size = image->total_bytes - offset;
	}

	block = offset / out_size;
	to_skip = offset % out_size;
	data = image->data + block;
	while (size) {
		to_read = min(size, out_size - to_skip);

		if (image->data_in_buf != data) {
			UBYTE *in_buf;
			UQUAD read_offs;
			ULONG bytes_left, read_size;

			image->data_in_buf = NULL;

			image->in_buf = ReAllocBuf(image->in_buf, &image->in_size, data->size);
			if (!image->in_buf) {
				return TDERR_NoMem;
			}

			in_buf = image->in_buf;
			fn = data->file;
			bytes_left = data->size;
			read_offs = data->offset;
			while (fn) {
				if (!ChangeFilePosition(fn->file, data->offset, OFFSET_BEGINNING)) {
					return TDERR_SeekError;
				}
				read_size = min(bytes_left, fn->size - read_offs);
				if (Read(fn->file, in_buf, read_size) != read_size) {
					return IPlugin_DOS2IOErr(IoErr());
				}
				bytes_left -= read_size;
				if (bytes_left == 0) break;
				in_buf += read_size;
				fn = fn->next;
			}

			InflateReset(&image->zs);
			image->zs.next_in = image->in_buf;
			image->zs.next_out = image->out_buf;
			image->zs.avail_in = data->size;
			image->zs.avail_out = out_size;
			if (Inflate(&image->zs, Z_SYNC_FLUSH) != Z_STREAM_END) {
				return TDERR_NotSpecified;
			}

			image->data_in_buf = data;
		}
		CopyMem(image->out_buf + to_skip, buffer, to_read);
		to_skip = 0;

		buffer += to_read;
		size -= to_read;
		io->io_Actual += to_read;
		data++;
	}
	return error;
}
