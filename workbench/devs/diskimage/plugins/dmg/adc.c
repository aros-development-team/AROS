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

#include "adc.h"
#include <exec/types.h>
#include <string.h>

LONG adc_decompress(APTR dst, LONG dst_len, APTR src, LONG src_len) {
	UBYTE *in = (UBYTE *)src;
	UBYTE *out = (UBYTE *)dst;
	UBYTE c;
	LONG len, off;
	LONG bytes_written;

	while (dst_len > 0 && src_len > 0) {
		c = *in++;
		src_len--;
		switch (c & 0xc0) {
			case 0x80:
			case 0xc0:
				len = (c & 0x7f) + 1;
				if (len > src_len) len = src_len;
				if (len > dst_len) len = dst_len;
				memcpy(out, in, len);
				in += len;
				out += len;
				src_len -= len;
				dst_len -= len;
				break;
			case 0x00:
				if (src_len < 1) {
					src_len = 0;
					break;
				}
				len = ((c & 0x3c) >> 2) + 3;
				off = (((UWORD)in[0] & 0x03) << 8) + (UWORD)in[1] + 1;
				bytes_written = out - (UBYTE *)dst;
				if (len > dst_len) len = dst_len;
				if (off <= bytes_written) {
					memset(out, out[-off], len);
				} else {
					memset(out, 0, len);
				}
				out += len;
				src_len -= 1;
				dst_len -= len;
				break;
			case 0x40:
				if (src_len < 2) {
					src_len = 0;
					break;
				}
				len = (c & 0x3f) + 4;
				off = ((UWORD)in[1] << 8) + (UWORD)in[2];
				bytes_written = out - (UBYTE *)dst;
				if (len > dst_len) len = dst_len;
				if (off <= bytes_written) {
					memset(out, out[-off], len);
				} else {
					memset(out, 0, len);
				}
				out += len;
				src_len -= 2;
				dst_len -= len;
				break;
		}
	}
	if (dst_len > 0) {
		memset(out, 0, dst_len);
	}
	bytes_written = out - (UBYTE *)dst;
	return bytes_written;
}
