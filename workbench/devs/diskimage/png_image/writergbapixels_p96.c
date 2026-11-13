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
#include <libraries/Picasso96.h>
#include <proto/Picasso96API.h>

BOOL IsSupportedRGBFormat_P96 (ULONG rgbformat) {
	switch (rgbformat) {
		case RGBFB_R5G6B5:
		case RGBFB_R5G6B5PC:
		case RGBFB_B5G6R5PC:
		case RGBFB_R5G5B5:
		case RGBFB_R5G5B5PC:
		case RGBFB_B5G5R5PC:
		case RGBFB_A8R8G8B8:
		case RGBFB_R8G8B8A8:
		case RGBFB_R8G8B8:
		case RGBFB_B8G8R8A8:
		case RGBFB_A8B8G8R8:
		case RGBFB_B8G8R8:
			return TRUE;

		default:
			return FALSE;
	}
}

IPTR WriteRGBAPixels_P96 (REG(a0, struct Hook *hook), REG(a2, struct RastPort *rp),
	REG(a1, struct BackFillMessage *msg))
{
	struct HookData *data = hook->h_Data;
	WORD x, y, w, h;
	ULONG bpp;
	struct RenderInfo ri;
	LONG lock;
	UBYTE *src, *dst;
	ULONG src_mod, dst_mod;

	w = msg->Bounds.MaxX - msg->Bounds.MinX + 1;
	h = msg->Bounds.MaxY - msg->Bounds.MinY + 1;

	src = data->image + (msg->OffsetY - data->y) * (data->w * 4UL) + (msg->OffsetX - data->x) * 4UL;
	src_mod = (data->w - w) * 4UL;

	lock = p96LockBitMap(rp->BitMap, (UBYTE *)&ri, sizeof(ri));

	bpp = p96GetBitMapAttr(rp->BitMap, P96BMA_BYTESPERPIXEL);
	dst = (UBYTE *)ri.Memory + msg->Bounds.MinY * ri.BytesPerRow + msg->Bounds.MinX * bpp;
	dst_mod = ri.BytesPerRow - w * bpp;

	switch (ri.RGBFormat) {
		case RGBFB_R5G6B5:
			{
				UWORD rgb;
				UWORD a6, a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a6 = src[3] >> 2;
						a5 = src[3] >> 3;
						if (a6 != 63) {
							rgb = rbe16(dst);
							r = (src[0] >> 3) + ((a5 * ((rgb >> 11) & 31)) >> 5);
							g = (src[1] >> 2) + ((a6 * ((rgb >> 5) & 63)) >> 6);
							b = (src[2] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wbe16(dst, (r << 11)|(g << 5)|(b));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_R5G6B5PC:
			{
				UWORD rgb;
				UWORD a6, a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a6 = src[3] >> 2;
						a5 = src[3] >> 3;
						if (a6 != 63) {
							rgb = rle16(dst);
							r = (src[0] >> 3) + ((a5 * ((rgb >> 11) & 31)) >> 5);
							g = (src[1] >> 2) + ((a6 * ((rgb >> 5) & 63)) >> 6);
							b = (src[2] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wle16(dst, (r << 11)|(g << 5)|(b));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_B5G6R5PC:
			{
				UWORD rgb;
				UWORD a6, a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a6 = src[3] >> 2;
						a5 = src[3] >> 3;
						if (a6 != 63) {
							rgb = rle16(dst);
							b = (src[2] >> 3) + ((a5 * ((rgb >> 11) & 31)) >> 5);
							g = (src[1] >> 2) + ((a6 * ((rgb >> 5) & 63)) >> 6);
							r = (src[0] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wle16(dst, (b << 11)|(g << 5)|(r));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_R5G5B5:
			{
				UWORD rgb;
				UWORD a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a5 = src[3] >> 3;
						if (a5 != 31) {
							rgb = rbe16(dst);
							r = (src[0] >> 3) + ((a5 * ((rgb >> 10) & 31)) >> 5);
							g = (src[1] >> 3) + ((a5 * ((rgb >> 5) & 31)) >> 5);
							b = (src[2] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wbe16(dst, (r << 10)|(g << 5)|(b));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_R5G5B5PC:
			{
				UWORD rgb;
				UWORD a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a5 = src[3] >> 3;
						if (a5 != 31) {
							rgb = rle16(dst);
							r = (src[0] >> 3) + ((a5 * ((rgb >> 10) & 31)) >> 5);
							g = (src[1] >> 3) + ((a5 * ((rgb >> 5) & 31)) >> 5);
							b = (src[2] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wle16(dst, (r << 10)|(g << 5)|(b));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_B5G5R5PC:
			{
				UWORD rgb;
				UWORD a5;
				UWORD r, g, b;
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a5 = src[3] >> 3;
						if (a5 != 31) {
							rgb = rle16(dst);
							b = (src[2] >> 3) + ((a5 * ((rgb >> 10) & 31)) >> 5);
							g = (src[1] >> 3) + ((a5 * ((rgb >> 5) & 31)) >> 5);
							r = (src[0] >> 3) + ((a5 * ((rgb) & 31)) >> 5);
							wle16(dst, (b << 10)|(g << 5)|(r));
						}
						src += 4;
						dst += 2;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_A8R8G8B8:
		case RGBFB_R8G8B8A8:
		case RGBFB_R8G8B8:
			{
				BOOL pre, post;
				UWORD a;
				pre = (ri.RGBFormat == RGBFB_A8R8G8B8);
				post = (ri.RGBFormat == RGBFB_R8G8B8A8);
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a = src[3];
						if (pre) dst++;
						if (a != 255) {
							dst[0] = src[0] + ((a * dst[0]) >> 8);
							dst[1] = src[1] + ((a * dst[1]) >> 8);
							dst[2] = src[2] + ((a * dst[2]) >> 8);
						}
						if (post) dst++;
						src += 4;
						dst += 3;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;

		case RGBFB_A8B8G8R8:
		case RGBFB_B8G8R8A8:
		case RGBFB_B8G8R8:
			{
				BOOL pre, post;
				UWORD a;
				pre = (ri.RGBFormat == RGBFB_A8B8G8R8);
				post = (ri.RGBFormat == RGBFB_B8G8R8A8);
				for (y = 0; y < h; y++) {
					for (x = 0; x < w; x++) {
						a = src[3];
						if (pre) dst++;
						if (a != 255) {
							dst[0] = src[2] + ((a * dst[0]) >> 8);
							dst[1] = src[1] + ((a * dst[1]) >> 8);
							dst[2] = src[0] + ((a * dst[2]) >> 8);
						}
						if (post) dst++;
						src += 4;
						dst += 3;
					}
					src += src_mod;
					dst += dst_mod;
				}
			}
			break;
	}
	
	p96UnlockBitMap(rp->BitMap, lock);

	return 0;
}
