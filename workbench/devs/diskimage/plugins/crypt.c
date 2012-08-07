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
#include <libraries/amisslmaster.h>
#include <libraries/amissl.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/amisslmaster.h>
#include <proto/amissl.h>
#include <string.h>
#include "device_locale.h"
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("Crypt")

extern struct DiskImagePlugin crypt_plugin;

PLUGIN_TABLE(&crypt_plugin)

struct CryptImage {
	BPTR file;
	APTR unit;
	QUAD total_bytes;
	ULONG block_size;
	ULONG total_blocks;
	UBYTE *buffer;
	BOOL key_valid;
	IDEA_KEY_SCHEDULE encrypt_ks;
	IDEA_KEY_SCHEDULE decrypt_ks;
	ULONG ivec[2];
	struct Library *amisslmasterbase;
	struct Library *amisslbase;
	LONG amissl_init;
};

BOOL Crypt_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
APTR Crypt_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void Crypt_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG Crypt_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG Crypt_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);
LONG Crypt_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin crypt_plugin = {
	PLUGIN_NODE(-128, "Crypt"),
	PLUGIN_FLAG_USERCHOICE|PLUGIN_FLAG_M68K,
	0,
	ZERO,
	NULL,
	Crypt_Init,
	NULL,
	NULL,
	Crypt_OpenImage,
	Crypt_CloseImage,
	Crypt_Geometry,
	Crypt_Read,
	Crypt_Write,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;
#define AmiSSLMasterBase image->amisslmasterbase
#define AmiSSLBase image->amisslbase

BOOL Crypt_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

static void TrashMem (void *mem, int len) {
	memset(mem, 0xDE, len);
	memset(mem, 0xAD, len);
	memset(mem, 0xBE, len);
	memset(mem, 0xEF, len);
}

BOOL Crypt_RequestPassword (struct DiskImagePlugin *Self, struct CryptImage *image) {
	STRPTR passwd;

	passwd = IPlugin_RequestPassword(image->unit);
	if (passwd && strlen(passwd) >= 10) {
		UBYTE key[16];
		UBYTE *p1, *p2, *p3, ch;
		memset(key, 0, 16);

		p1 = (UBYTE *)passwd;
		p2 = key;
		p3 = p2 + sizeof(key);
		while (ch = *p1++) {
			*p2++ += ch;
			if (p2 >= p3) p2 = key;
		}

		TrashMem(passwd, strlen(passwd)+1);
		idea_set_encrypt_key(key, &image->encrypt_ks);
		TrashMem(key, 16);
		idea_set_decrypt_key(&image->encrypt_ks, &image->decrypt_ks);

		image->key_valid = TRUE;
	}
	FreeVec(passwd);

	return image->key_valid;
}

APTR Crypt_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	struct CryptImage *image = NULL;
	
	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;
	image->unit = unit;

	image->amisslmasterbase = OpenLibrary("amisslmaster.library", AMISSLMASTER_MIN_VERSION);
	if (!image->amisslmasterbase) {
		error = ERROR_OBJECT_NOT_FOUND;
		error_string = MSG_REQVER;
		error_args[0] = (IPTR)"amisslmaster.library";
		error_args[1] = AMISSLMASTER_MIN_VERSION;
		error_args[2] = 1;
		goto error;
	}
	if (InitAmiSSLMaster(AMISSL_CURRENT_VERSION, FALSE) == FALSE) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	image->amisslbase = OpenAmiSSL();
	if (!image->amisslbase) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}
	image->amissl_init = InitAmiSSLA(NULL);
	if (image->amissl_init != NO_ERROR) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	if (!IsCipherAvailable(CIPHER_IDEA)) {
		error = ERROR_OBJECT_NOT_FOUND;
		goto error;
	}

	image->total_bytes = GetFileSize(file);
	if (image->total_bytes == -1) {
		error = IoErr();
		goto error;
	}
	image->block_size = 512;
	image->total_bytes &= ~511;
	image->total_blocks = image->total_bytes >> 9;

	image->buffer = AllocVec(image->block_size, MEMF_ANY);
	if (!image->buffer) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (!Crypt_RequestPassword(Self, image)) {
		error = ERROR_REQUIRED_ARG_MISSING;
		goto error;
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

void Crypt_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct CryptImage *image = image_ptr;
	if (image) {
		if (image->amisslmasterbase) {
			if (image->amisslbase) {
				if (image->amissl_init == NO_ERROR) {
					CleanupAmiSSLA(NULL);
				}
				CloseAmiSSL();
			}
			CloseLibrary(image->amisslmasterbase);
		}
		FreeVec(image->buffer);
		Close(image->file);
		TrashMem(image, sizeof(*image));
		FreeVec(image);
	}
}

LONG Crypt_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct CryptImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG Crypt_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CryptImage *image = image_ptr;
	UQUAD offset;
	UBYTE *buffer;
	ULONG size;
	BPTR file = image->file;
	LONG error = IOERR_SUCCESS;
	LONG status;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 511) return IOERR_BADADDRESS;
	if (size & 511) return IOERR_BADLENGTH;

	if (!ChangeFilePosition(file, offset, OFFSET_BEGINNING)) {
		return TDERR_SeekError;
	}

	if (offset + size > image->total_bytes) {
		size = image->total_bytes - offset;
		error = IOERR_BADLENGTH;
	}

	/* read encrypted blocks */
	status = Read(file, buffer, size);
	if (status == -1) {
		return IPlugin_DOS2IOErr(IoErr());
	} else
	if (status != size) {
		return TDERR_NotSpecified;
	}

	io->io_Actual = size;
	size >>= 9;
	while (size--) {
		/* decrypt block */
		image->ivec[0] = image->ivec[1] = offset;
		idea_cbc_encrypt(buffer, buffer, 512,
			&image->decrypt_ks, (UBYTE *)image->ivec, IDEA_DECRYPT);

		offset += 512;
		buffer += 512;
	}
	return error;
}

LONG Crypt_Write (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct CryptImage *image = image_ptr;
	UQUAD offset;
	UBYTE *buffer;
	ULONG block, size;
	BPTR file = image->file;
	LONG error = IOERR_SUCCESS;
	LONG status;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset & 511) return IOERR_BADADDRESS;
	if (size & 511) return IOERR_BADLENGTH;
	block = offset >> 9;
	size >>= 9;

	if (!ChangeFilePosition(file, offset, OFFSET_BEGINNING)) {
		return TDERR_SeekError;
	}

	if (block + size > image->total_blocks) {
		size = image->total_blocks - block;
		error = IOERR_BADLENGTH;
	}
	
	while (size--) {
		/* encrypt block */
		image->ivec[0] = image->ivec[1] = offset;
		idea_cbc_encrypt(buffer, image->buffer, 512,
			&image->encrypt_ks, (UBYTE *)image->ivec, IDEA_ENCRYPT);

		/* write encrypted block */
		status = Write(file, image->buffer, 512);
		if (status != 512) {
			return IPlugin_DOS2IOErr(IoErr());
		}

		offset += 512;
		buffer += 512;
		io->io_Actual += 512;
	}
	return error;
}
