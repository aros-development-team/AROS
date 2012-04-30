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

#include "class.h"
#include "endian.h"
#include <libraries/z.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/z.h>
#include <string.h>

#pragma pack(1)

#define PNG_SIGNATURE "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A"

typedef struct {
	ULONG size;
	ULONG id;
} png_chunk_t;

#define ID_IHDR MAKE_ID('I','H','D','R')
#define ID_PLTE MAKE_ID('P','L','T','E')
#define ID_IDAT MAKE_ID('I','D','A','T')
#define ID_IEND MAKE_ID('I','E','N','D')

typedef struct {
	ULONG width;
	ULONG height;
	UBYTE depth;
	UBYTE color_type;
	UBYTE compression;
	UBYTE filter;
	UBYTE interlace;
} png_ihdr_t;

#pragma pack()

static inline WORD abs (WORD x) {
	return x >= 0 ? x : -x;
}

/*static void PrintULONG (ULONG x) {
	ULONG y;
	char buffer[9];
	int i;
	for (i = 0; i < 8; i++) {
		y = (x & 0xf0000000UL) >> 28;
		x <<= 4;
		if (y <= 9)
			buffer[i] = y + '0';
		else if (y <= 15)
			buffer[i] = y + 'A' - 10;
		else
			buffer[i] = '?';
	}
	buffer[8] = '\n';
	Write(Output(), buffer, 9);
}*/

BOOL LoadPNG (struct ClassData *data, const char *filename, LONG index) {
	BOOL ihdr_found = FALSE;
	BOOL done = FALSE;
	BPTR file = 0;
	UBYTE signature[8];
	png_chunk_t chunk;
	png_ihdr_t ihdr;
	ULONG original_crc/*, crc*/;
	const LONG pix_size = 4;
	LONG row_size = 0;
	ULONG data_size = 0;
	ULONG png_data_size = 0;
	ULONG packed_size = 0;
	UBYTE *packed = NULL;
	UBYTE *unpacked = NULL;
	ULONG dest_len, source_len;
	UBYTE *src, *dst;
	int filter;
	int y, x;
	WORD a, b, c, d;
	WORD p, pa, pb, pc;
	
	file = Open(filename, MODE_OLDFILE);
	if (!file) {
		goto error;
	}
	
	if (FRead(file, signature, 1, sizeof(signature)) != sizeof(signature)) {
		goto error;
	}
	if (memcmp(signature, PNG_SIGNATURE, sizeof(signature))) {
		goto error;
	}

	while (!done) {
		if (FRead(file, &chunk, 1, sizeof(chunk)) != sizeof(chunk)) {
			goto error;
		}
		/*crc = CRC32(0, &chunk.id, sizeof(chunk.id));*/
		wbe32(&chunk.size, chunk.size);
		switch (chunk.id) {
			case ID_IHDR:
				ihdr_found = TRUE;
				if (chunk.size != sizeof(ihdr)) {
					goto error;
				}
				if (FRead(file, &ihdr, 1, chunk.size) != chunk.size) {
					goto error;
				}
				if (FRead(file, &original_crc, 1, sizeof(original_crc)) != sizeof(original_crc)) {
					goto error;
				}
				/*crc = CRC32(crc, &ihdr, chunk.size);*/
				/*PrintULONG(crc);
				PrintULONG(original_crc);*/
				/*if (crc != original_crc) {
					goto error;
				}*/
				if (index == IMG_NORMAL) {
					data->width = rbe32(&ihdr.width);
					data->height = rbe32(&ihdr.height);
				} else {
					if (data->width != rbe32(&ihdr.width) || data->height != rbe32(&ihdr.height)) {
						goto error;
					}
				}
				if (ihdr.depth != 8 || ihdr.color_type != 6 || ihdr.compression != 0 ||
					ihdr.filter != 0 || ihdr.interlace != 0)
				{
					goto error;
				}
				row_size = data->width * pix_size;
				data_size = row_size * data->height;
				data->image[index] = AllocVec(data_size, MEMF_ANY);
				if (!data->image[index]) {
					goto error;
				}
				png_data_size = data_size + data->height;
				packed_size = 0;
				packed = AllocVec(png_data_size, MEMF_ANY);
				unpacked = AllocVec(png_data_size, MEMF_ANY);
				if (!packed || !unpacked) {
					goto error;
				}
				break;
			
			case ID_IDAT:
				if (!ihdr_found) {
					goto error;
				}
				if (packed_size + chunk.size > png_data_size) {
					goto error;
				}
				if (FRead(file, packed + packed_size, 1, chunk.size) != chunk.size) {
					goto error;
				}
				if (FRead(file, &original_crc, 1, sizeof(original_crc)) != sizeof(original_crc)) {
					goto error;
				}
				/*crc = CRC32(crc, packed + packed_size, chunk.size);*/
				/*PrintULONG(crc);
				PrintULONG(original_crc);*/
				/*if (crc != original_crc) {
					goto error;
				}*/
				packed_size += chunk.size;
				break;
				
			case ID_IEND:
				if (!ihdr_found || !packed_size) {
					goto error;
				}
				if (chunk.size != 0) {
					goto error;
				}
				if (FRead(file, &original_crc, 1, sizeof(original_crc)) != sizeof(original_crc)) {
					goto error;
				}
				/*PrintULONG(crc);
				PrintULONG(original_crc);*/
				/*if (crc != original_crc) {
					goto error;
				}*/
				dest_len = png_data_size;
				source_len = packed_size;
				if (Uncompress(unpacked, &dest_len, packed, source_len) != Z_OK) {
					goto error;
				}
				if (dest_len != png_data_size) {
					goto error;
				}
				src = unpacked;
				dst = data->image[index];
				for (y = 0; y < data->height; y++) {
					filter = *src++;
					a = b = c = 0;
					for (x = 0; x < row_size; x++) {
						if (x >= pix_size) a = dst[-pix_size];
						if (y >= 1) b = dst[-row_size];
						if (y >= 1 && x >= pix_size) c = dst[-pix_size-row_size];
						d = *src++;
						switch (filter) {
							case 0:
								*dst++ = d;
								break;
							case 1:
								*dst++ = d + a;
								break;
							case 2:
								*dst++ = d + b;
								break;
							case 3:
								*dst++ = d + ((a + b) / 2);
								break;
							case 4:
								p = a + b - c;
								pa = abs(p-a);
								pb = abs(p-b);
								pc = abs(p-c);
								if (pa <= pb && pa <= pc)
									*dst++ = d + a;
								else if (pb <= pc)
									*dst++ = d + b;
								else
									*dst++ = d + c;
								break;
							default:
								goto error;
						}
					}
				}
				done = TRUE;
				break;

			default:
				if (Seek(file, chunk.size + 4, OFFSET_CURRENT) == -1) {
					goto error;
				}
				break;
		}
	}

error:
	FreeVec(packed);
	FreeVec(unpacked);
	if (file) Close(file);
	return done;
}
