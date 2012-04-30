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
#include <libraries/expat.h>
#include <libraries/z.h>
#include <libraries/bz2.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/expat.h>
#include <proto/z.h>
#include <proto/bz2.h>
#include "endian.h"
#include "base64.h"
#include "adc.h"
#include "device_locale.h"
#include <string.h>
#include <SDI_compiler.h>
#include "rev/diskimage.device_rev.h"

PLUGIN_VERSTAG("DMG")

extern struct DiskImagePlugin dmg_plugin;

PLUGIN_TABLE(&dmg_plugin)

struct DMGPart {
	ULONG type;
	ULONG in_offs;
	ULONG in_size;
	ULONG out_size;
};

struct DMGHash {
	struct MinNode *pnode;
	struct DMGPart *part;
	UQUAD offset;
};

#define HASH_FUNC 20

struct DMGImage {
	BPTR file;
	BYTE uses_adc;
	BYTE uses_zlib;
	BYTE uses_bzlib;
	struct List *plist;
	struct DMGHash *hash;
	UBYTE *in_buf, *out_buf;
	ULONG in_size, out_size;
	struct DMGPart *part_in_buf;
	ULONG block_size;
	ULONG total_blocks;
	UQUAD total_bytes;
	struct Library *expatbase;
	struct Library *zbase;
	struct Library *bz2base;
};

BOOL DMG_Init (struct DiskImagePlugin *Self, const struct PluginData *data);
BOOL DMG_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize);
APTR DMG_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file, CONST_STRPTR name);
void DMG_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr);
LONG DMG_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg);
LONG DMG_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io);

struct DiskImagePlugin dmg_plugin = {
	PLUGIN_NODE(0, "DMG"),
	PLUGIN_FLAG_FOOTER|PLUGIN_FLAG_M68K,
	512,
	ZERO,
	NULL,
	DMG_Init,
	NULL,
	DMG_CheckImage,
	DMG_OpenImage,
	DMG_CloseImage,
	DMG_Geometry,
	DMG_Read,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

struct Library *SysBase;
struct Library *DOSBase;
static struct DIPluginIFace *IPlugin;
#define ZBase image->zbase
#define BZ2Base image->bz2base

BOOL DMG_Init (struct DiskImagePlugin *Self, const struct PluginData *data) {
	SysBase = data->SysBase;
	DOSBase = data->DOSBase;
	IPlugin = data->IPlugin;
	return TRUE;
}

#define ID_koly MAKE_ID('k','o','l','y')
#define ID_mish MAKE_ID('m','i','s','h')

BOOL DMG_CheckImage (struct DiskImagePlugin *Self, BPTR file, CONST_STRPTR name, QUAD file_size,
	const UBYTE *test, LONG testsize)
{
	return testsize >= 512 && rbe32(&test[testsize-512]) == ID_koly;
}

#pragma pack(1)

typedef struct {
	ULONG id;
	ULONG version;
	ULONG header_size;
	ULONG flags;
	UQUAD running_data_fork_offset;
	UQUAD data_fork_offset;
	UQUAD data_fork_length;
	UQUAD rsrc_fork_offset;
	UQUAD rsrc_fork_length;
	ULONG segment_number;
	ULONG segment_count;
	ULONG segment_id1;
	ULONG segment_id2;
	ULONG segment_id3;
	ULONG segment_id4;
	ULONG data_fork_checksum_type;
	ULONG reserved1;
	ULONG data_fork_checksum;
	ULONG reserved2[31];
	UQUAD xml_offset;
	UQUAD xml_length;
	ULONG reserved3[30];
	ULONG master_checksum_type;
	ULONG reserved4;
	ULONG master_checksum;
	ULONG reserved5[31];
	ULONG image_variant;
	UQUAD sector_count;
	ULONG reserved6[3];
} dmg_koly_t;

typedef struct {
	ULONG id;
	ULONG version;
	UQUAD first_sector;
	UQUAD sector_count;
	UQUAD data_start;
	ULONG decompressed_buffer_size;
	ULONG blocks_descriptor;
	ULONG reserved1[6];
	ULONG checksum_type;
	ULONG reserved2;
	ULONG checksum;
	ULONG reserved3[31];
	ULONG blocks_run_count;
} dmg_mish_t;

#define PT_ZERO		0x00000000
#define PT_COPY		0x00000001
#define PT_IGNORE	0x00000002
#define PT_COMMENT	0x7ffffffe
#define PT_ADC		0x80000004
#define PT_ZLIB		0x80000005
#define PT_BZLIB	0x80000006
#define PT_END		0xffffffff

#pragma pack()

typedef struct {
	struct Library *expatbase;
	XML_Parser parser;
	struct DMGImage *image;
	LONG current_tag_depth;
	BYTE is_in_data_tag;
	STRPTR data;
	LONG len, size;
	LONG error;
	LONG error_string;
	IPTR error_args[4];
} XML_Parser_Data;

static LONG add_to_plist (struct DMGImage *image, CONST_STRPTR src, LONG len);

static void xml_start_element_handler (void *user_data,
	const char *name, const char **attrs);
static void xml_end_element_handler (void *user_data,
	const char *name);
static void xml_character_data_handler (void *user_data,
	const char *s, int len);

APTR DMG_OpenImage (struct DiskImagePlugin *Self, APTR unit, BPTR file,
	CONST_STRPTR name)
{
	LONG done = FALSE;
	LONG error = NO_ERROR;
	LONG error_string = NO_ERROR_STRING;
	IPTR error_args[4] = {0};
	dmg_koly_t *koly = NULL;
	STRPTR data = NULL;
	struct DMGImage *image = NULL;
	struct MinNode *pnode;
	struct DMGPart *part;
	ULONG type;
	ULONG hash_size;
	struct DMGHash *hash;
	UQUAD offset, next_offset;
	UQUAD hash_offset;

	koly = AllocVec(sizeof(*koly), MEMF_ANY);
	if (!koly) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	if (!ChangeFilePosition(file, -(int)sizeof(*koly), OFFSET_END) ||
		Read(file, koly, sizeof(*koly)) != sizeof(*koly))
	{
		error = IoErr();
		goto error;
	}

	if (rbe32(&koly->id) != ID_koly) {
		error = ERROR_OBJECT_WRONG_TYPE;
		goto error;
	}

	image = AllocVec(sizeof(*image), MEMF_CLEAR);
	if (!image) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->file = file;

	image->plist = AllocVec(sizeof(struct MinList), MEMF_ANY);
	if (!image->plist) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}
	image->plist->lh_Head = (struct Node *)&image->plist->lh_Tail;
	image->plist->lh_Tail = NULL;
	image->plist->lh_TailPred = (struct Node *)&image->plist->lh_Head;

	if (koly->xml_offset && koly->xml_length) {
		UQUAD xml_offset;
		ULONG xml_length;
		XML_Parser parser;
		XML_Parser_Data parser_data;
		int xml_error;
		struct Library *ExpatBase;

		xml_offset = rbe64(&koly->xml_offset);
		xml_length = rbe64(&koly->xml_length);

		data = AllocVec(xml_length, MEMF_ANY);
		if (!data) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}

		if (!ChangeFilePosition(file, xml_offset, OFFSET_BEGINNING) ||
			Read(file, data, xml_length) != xml_length)
		{
			error = IoErr();
			goto error;
		}

		image->expatbase = OpenLibrary("expat.library", 4);
		if (!image->expatbase) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQVER;
			error_args[0] = (IPTR)"expat.library";
			error_args[1] = 4;
			error_args[2] = 1;
			goto error;
		}
		ExpatBase = image->expatbase;

		parser = XML_ParserCreate(NULL);
		if (!parser) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}

		memset(&parser_data, 0, sizeof(parser_data));
		parser_data.parser = parser;
		parser_data.image = image;

		XML_SetUserData(parser, &parser_data);
		XML_SetElementHandler(parser,
			xml_start_element_handler,
			xml_end_element_handler);
		XML_SetCharacterDataHandler(parser,
			xml_character_data_handler);
		if (!XML_Parse(parser, data, xml_length, TRUE)) {
			xml_error = XML_GetErrorCode(parser);
		} else {
			xml_error = XML_ERROR_NONE;
		}
		XML_ParserFree(parser);
		if (parser_data.error) {
			error = parser_data.error;
			error_string = parser_data.error_string;
			CopyMem(parser_data.error_args, error_args, sizeof(error_args));
			goto error;
		}
		if (xml_error != XML_ERROR_NONE) {
			if (xml_error == XML_ERROR_NO_MEMORY) {
				error = ERROR_NO_FREE_STORE;
			} else {
				error = ERROR_OBJECT_WRONG_TYPE;
				error_string = MSG_EXPATERR;
			}
			goto error;
		}

		CloseLibrary(image->expatbase);
		image->expatbase = NULL;
	} else if (koly->rsrc_fork_offset && koly->rsrc_fork_length) {
		UQUAD rsrc_offset;
		ULONG rsrc_length;
		CONST_STRPTR src;
		LONG len;
		dmg_mish_t *mish;
		LONG num_parts;

		rsrc_offset = rbe64(&koly->rsrc_fork_offset);
		rsrc_length = rbe64(&koly->rsrc_fork_length);

		data = AllocVec(rsrc_length, MEMF_ANY);
		if (!data) {
			error = ERROR_NO_FREE_STORE;
			goto error;
		}

		if (!ChangeFilePosition(file, rsrc_offset, OFFSET_BEGINNING) ||
			Read(file, data, rsrc_length) != rsrc_length)
		{
			error = IoErr();
			goto error;
		}

		src = data + 0x104;
		len = rsrc_length - 0x104;
		while (len >= 0xcc) {
			mish = (dmg_mish_t *)src;
			if (rbe32(&mish->id) != ID_mish) {
				error = ERROR_OBJECT_WRONG_TYPE;
				goto error;
			}

			num_parts = rbe32(&mish->blocks_run_count);
			error = add_to_plist(image, src + 0xcc, 0x28*num_parts);
			if (error != NO_ERROR) goto error;

			src += (0xcc + (0x28*num_parts) + 0x04);
			len -= (0xcc + (0x28*num_parts) + 0x04);
		}
	} else {
		error = ERROR_NOT_IMPLEMENTED;
		goto error;
	}

	if (image->uses_zlib) {
		image->zbase = OpenLibrary("z.library", 1);
		if (!image->zbase || !CheckLib(image->zbase, 1, 6)) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQVER;
			error_args[0] = (IPTR)"z.library";
			error_args[1] = 1;
			error_args[2] = 6;
			goto error;
		}
	}

	if (image->uses_bzlib) {
		image->bz2base = OpenLibrary("bz2.library", 1);
		if (!image->bz2base) {
			error = ERROR_OBJECT_NOT_FOUND;
			error_string = MSG_REQVER;
			error_args[0] = (IPTR)"bz2.library";
			error_args[1] = 1;
			error_args[2] = 1;
			goto error;
		}
	}

	if (image->total_bytes == 0) {
		goto error;
	}

	image->block_size = 512;
	image->total_blocks = image->total_bytes / image->block_size;

	hash_size = ((image->total_bytes >> HASH_FUNC) + 1) * sizeof(*hash);
	image->hash = hash = AllocVec(hash_size, MEMF_ANY);
	if (!hash) {
		error = ERROR_NO_FREE_STORE;
		goto error;
	}

	offset = next_offset = hash_offset = 0;
	pnode = (struct MinNode *)image->plist->lh_Head;
	while (pnode->mln_Succ) {
		part = (struct DMGPart *)(pnode + 1);
		do {
			type = part->type;
			switch (type) {
				case PT_ZERO:
				case PT_COPY:
				case PT_IGNORE:
				case PT_ADC:
				case PT_ZLIB:
				case PT_BZLIB:
					next_offset += part->out_size;
					while (next_offset > hash_offset) {
						hash->pnode = pnode;
						hash->part = part;
						hash->offset = offset;
						hash++;
						hash_offset += (1 << HASH_FUNC);
					}
					offset = next_offset;
					break;
			}
			part++;
		} while (type != PT_END);
		pnode = pnode->mln_Succ;
	}

	done = TRUE;

error:
	FreeVec(data);
	FreeVec(koly);
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

static LONG add_to_plist (struct DMGImage *image, CONST_STRPTR src, LONG len) {
	struct MinNode *pnode;
	LONG num_parts = len / 0x28;
	struct DMGPart *part;
	ULONG type;

	if (num_parts <= 0) {
		return NO_ERROR;
	}

	pnode = AllocVec(sizeof(*pnode) + (num_parts+1)*sizeof(*part), MEMF_CLEAR);
	if (!pnode) {
		return ERROR_NO_FREE_STORE;
	}

	AddTail(image->plist, (struct Node *)pnode);

	part = (struct DMGPart *)(pnode + 1);
	do {
		part->type = type = rbe32(src);
		part->in_offs = rbe32(src+28);
		part->in_size = rbe32(src+36);
		part->out_size = rbe32(src+20) << 9;
		switch (type) {
			case PT_ZERO:
			case PT_COPY:
			case PT_IGNORE:
				image->total_bytes += part->out_size;
				break;
			case PT_COMMENT:
				break;
			case PT_ADC:
				image->uses_adc = TRUE;
				image->total_bytes += part->out_size;
				break;
			case PT_ZLIB:
				image->uses_zlib = TRUE;
				image->total_bytes += part->out_size;
				break;
			case PT_BZLIB:
				image->uses_bzlib = TRUE;
				image->total_bytes += part->out_size;
				break;
			case PT_END:
				break;
			default:
				return ERROR_NOT_IMPLEMENTED;
				break;
		}
		src += 0x28;
		part++;
	} while (type != PT_END && --num_parts);
	part->type = PT_END;
	part->in_offs = 0;
	part->in_size = 0;
	part->out_size = 0;

	return NO_ERROR;
}

static const char *XML_GetAttrVal (const char *attr, const char **attrs, const char *defVal) {
	while (*attrs) {
		if (!strcmp(*attrs, attr)) {
			return attrs[1];
		}
		attrs += 2;
	}
	return defVal;
}

static void xml_start_element_handler (void *user_data,
	const char *name, const char **attrs)
{
	XML_Parser_Data *data = user_data;
	struct Library *ExpatBase = data->expatbase;
	if (data->error) return;

	if (data->current_tag_depth == 0) {
		if (strcmp(name, "plist") ||
			strcmp(XML_GetAttrVal("version", attrs, ""), "1.0"))
		{
			data->error = ERROR_OBJECT_WRONG_TYPE;
			XML_StopParser(data->parser, XML_TRUE);
			return;
		}
	} else {
		if (!strcmp(name, "data")) {
			if (data->is_in_data_tag) {
				data->error = ERROR_TOO_MANY_LEVELS;
				XML_StopParser(data->parser, XML_TRUE);
				return;
			}
			data->is_in_data_tag = TRUE;
			data->len = 0;
		}
	}
	data->current_tag_depth++;
}

static void xml_end_element_handler (void *user_data,
	const char *name)
{
	XML_Parser_Data *data = user_data;
	struct Library *ExpatBase = data->expatbase;
	if (data->error) return;

	data->current_tag_depth--;
	if (data->current_tag_depth == 0) {
		if (strcmp(name, "plist")) {
			data->error = ERROR_OBJECT_WRONG_TYPE;
			XML_StopParser(data->parser, XML_TRUE);
			return;
		}
	} else {
		if (!strcmp(name, "data")) {
			if (!data->is_in_data_tag) {
				data->error = ERROR_TOO_MANY_LEVELS;
				XML_StopParser(data->parser, XML_TRUE);
				return;
			}
			data->is_in_data_tag = FALSE;
			if (data->len > 0) {
				struct DMGImage *image = data->image;
				LONG len;

				cleanup_base64(data->data);
				len = decode_base64(data->data, data->data);

				data->error = add_to_plist(image, data->data + 0xcc, len - 0xcc);
				if (data->error != NO_ERROR) {
					XML_StopParser(data->parser, XML_TRUE);
					return;
				}
			}
		}
	}
}

static void xml_character_data_handler (void *user_data,
	const char *s, int len)
{
	XML_Parser_Data *data = user_data;
	struct Library *ExpatBase = data->expatbase;
	if (data->error) return;

	if (data->is_in_data_tag && len > 0) {
		if (len <= (data->size - data->len)) {
			CopyMem(s, data->data + data->len, len);
			data->len += len;
			data->data[data->len] = 0;
		} else {
			STRPTR new_data;
			new_data = AllocVec(data->len + len + 1, MEMF_ANY);
			if (!new_data) {
				data->error = ERROR_NO_FREE_STORE;
				XML_StopParser(data->parser, XML_TRUE);
				return;
			}
			if (data->data) {
				CopyMem(data->data, new_data, data->len);
				FreeVec(data->data);
			}
			CopyMem(s, new_data + data->len, len);
			data->data = new_data;
			data->len += len;
			data->size = data->len;
			data->data[data->len] = 0;
		}
	}
}

void DMG_CloseImage (struct DiskImagePlugin *Self, APTR image_ptr) {
	struct DMGImage *image = image_ptr;
	if (image) {
		if (image->bz2base) CloseLibrary(image->bz2base);
		if (image->zbase) CloseLibrary(image->zbase);
		if (image->expatbase) CloseLibrary(image->expatbase);
		FreeVec(image->hash);
		if (image->plist) {
			struct Node *pnode;
			while ((pnode = RemHead(image->plist))) {
				FreeVec(pnode);
			}
			FreeVec(image->plist);
		}
		FreeVec(image->in_buf);
		FreeVec(image->out_buf);
		Close(image->file);
		FreeVec(image);
	}
}

LONG DMG_Geometry (struct DiskImagePlugin *Self, APTR image_ptr, struct DriveGeometry *dg) {
	struct DMGImage *image = image_ptr;
	dg->dg_SectorSize = image->block_size;
	dg->dg_Heads =
	dg->dg_TrackSectors =
	dg->dg_CylSectors = 1;
	dg->dg_Cylinders =
	dg->dg_TotalSectors = image->total_blocks;
	return IOERR_SUCCESS;
}

LONG DMG_Read (struct DiskImagePlugin *Self, APTR image_ptr, struct IOStdReq *io) {
	struct DMGImage *image = image_ptr;
	BPTR file = image->file;
	UBYTE *buffer;
	UQUAD offset;
	ULONG size;
	struct DMGHash *hash;
	struct MinNode *pnode;
	struct DMGPart *part;
	UQUAD read_offs, next_offs;
	ULONG to_skip, to_read;

	buffer = io->io_Data;
	offset = ((UQUAD)io->io_Offset)|((UQUAD)io->io_Actual << 32);
	size = io->io_Length;
	io->io_Actual = 0;

	if (offset >= image->total_bytes) {
		return TDERR_SeekError;
	}

	hash = &image->hash[offset >> HASH_FUNC];
	pnode = hash->pnode;
	part = hash->part;
	read_offs = next_offs = hash->offset;
	for (;;) {
		switch (part->type) {
			case PT_ZERO:
			case PT_COPY:
			case PT_IGNORE:
			case PT_ADC:
			case PT_ZLIB:
			case PT_BZLIB:
				next_offs += part->out_size;
				part++;
				break;
			case PT_END:
				pnode = (struct MinNode *)GetSucc((struct Node *)pnode);
				if (!pnode) {
					return TDERR_SeekError;
				}
				part = (struct DMGPart *)(pnode + 1);
				break;
			default:
				part++;
				break;
		}
		if (next_offs > offset) break;
		read_offs = next_offs;
	}

	part--;
	to_skip = offset - read_offs;
	while (size) {
		to_read = max((LONG)min(size, part->out_size - to_skip), 0);

		switch (part->type) {

			case PT_ADC:
				if (!part->out_size) break;
				if (image->part_in_buf != part) {
					image->part_in_buf = NULL;

					if (!(image->in_buf = ReAllocBuf(image->in_buf, &image->in_size, part->in_size)) ||
						!(image->out_buf = ReAllocBuf(image->out_buf, &image->out_size, part->out_size)))
					{
						return TDERR_NoMem;
					}

					if (!ChangeFilePosition(file, part->in_offs, OFFSET_BEGINNING)) {
						return TDERR_SeekError;
					}
					if (Read(file, image->in_buf, part->in_size) != part->in_size) {
						LONG error;
						error = IoErr();
						return error ? IPlugin_DOS2IOErr(error) : IOERR_BADLENGTH;
					}

					adc_decompress(image->out_buf, part->out_size, image->in_buf, part->in_size);

					image->part_in_buf = part;
				}
				CopyMem(image->out_buf + to_skip, buffer, to_read);
				part++;
				break;

			case PT_ZLIB:
				if (!part->out_size) break;
				if (image->part_in_buf != part) {
					LONG status;
					ULONG out_len;
					image->part_in_buf = NULL;

					if (!(image->in_buf = ReAllocBuf(image->in_buf, &image->in_size, part->in_size)) ||
						!(image->out_buf = ReAllocBuf(image->out_buf, &image->out_size, part->out_size)))
					{
						return TDERR_NoMem;
					}

					if (!ChangeFilePosition(file, part->in_offs, OFFSET_BEGINNING)) {
						return TDERR_SeekError;
					}
					if (Read(file, image->in_buf, part->in_size) != part->in_size) {
						return IPlugin_DOS2IOErr(IoErr());
					}

					out_len = part->out_size;
					if ((status = Uncompress(image->out_buf, &out_len, image->in_buf,
						part->in_size)) != Z_OK)
					{
						return TDERR_NotSpecified;
					}

					image->part_in_buf = part;
				}
				CopyMem(image->out_buf + to_skip, buffer, to_read);
				part++;
				break;

			case PT_BZLIB:
				if (!part->out_size) break;
				if (image->part_in_buf != part) {
					LONG status;
					ULONG out_len;
					image->part_in_buf = NULL;

					if (!(image->in_buf = ReAllocBuf(image->in_buf, &image->in_size, part->in_size)) ||
						!(image->out_buf = ReAllocBuf(image->out_buf, &image->out_size, part->out_size)))
					{
						return TDERR_NoMem;
					}

					if (!ChangeFilePosition(file, part->in_offs, OFFSET_BEGINNING)) {
						return TDERR_SeekError;
					}
					if (Read(file, image->in_buf, part->in_size) != part->in_size) {
						return IPlugin_DOS2IOErr(IoErr());
					}

					out_len = part->out_size;
					if ((status = BZ2_bzBuffToBuffDecompress(image->out_buf, &out_len,
						image->in_buf, part->in_size, 0, 0)) != BZ_OK)
					{
						return TDERR_NotSpecified;
					}

					image->part_in_buf = part;
				}
				CopyMem(image->out_buf + to_skip, buffer, to_read);
				part++;
				break;

			case PT_COPY:
				if (!part->out_size) break;
				if (!ChangeFilePosition(file, part->in_offs + to_skip, OFFSET_BEGINNING)) {
					return TDERR_SeekError;
				}
				if (Read(file, buffer, to_read) != to_read) {
					LONG error;
					error = IoErr();
					return error ? IPlugin_DOS2IOErr(error) : IOERR_BADLENGTH;
				}
				part++;
				break;

			case PT_ZERO:
			case PT_IGNORE:
				memset(buffer, 0, to_read);
				part++;
				break;

			case PT_END:
				to_read = 0;
				pnode = (struct MinNode *)GetSucc((struct Node *)pnode);
				if (!pnode) return IOERR_BADLENGTH;
				part = (struct DMGPart *)(pnode + 1);
				break;

			default:
				to_read = 0;
				part++;
				break;

		}
		to_skip = 0;
		buffer += to_read;
		size -= to_read;
		io->io_Actual += to_read;
	}
	return IOERR_SUCCESS;
}
