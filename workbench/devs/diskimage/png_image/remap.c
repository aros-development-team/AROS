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
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <string.h>

#define RGB8to32(x) ((x)*0x01010101UL)

static inline BOOL PenMap_SetPen (ULONG *pen_map, LONG pen) {
	ULONG l = pen >> 5;
	ULONG b = pen & 31;
	if (pen_map[l] & (1UL << b)) {
		return TRUE;
	} else {
		pen_map[l] |= (1UL << b);
	}
	return FALSE;
}

void PenMap_ReleasePens (ULONG *pen_map, struct Screen *screen) {
	struct ColorMap *cmap = screen->ViewPort.ColorMap;
	ULONG l, b;
	LONG pen;
	for (pen = l = 0; l < 8; l++) {
		for (b = 0; b < 31; b++, pen++) {
			if (pen_map[l] & (1UL << b)) {
				ReleasePen(cmap, pen);
			}
		}
		pen_map[l] = 0;
	}
}

BOOL RemapRGBAImage (struct Screen *screen, BOOL rtg, UBYTE *rgba, WORD w, WORD h,
	ULONG *pen_map, struct BitMap **bm_p, PLANEPTR *mask_p)
{
	static const struct TagItem tags[] = {
		{ OBP_Precision, PRECISION_IMAGE },
		{ TAG_END,       0               },
	};
	struct ColorMap *cmap = screen->ViewPort.ColorMap;
	struct BitMap *screen_bm = screen->RastPort.BitMap;
	WORD depth = rtg ? 8 : screen_bm->Depth;
	UBYTE *clut = NULL, *src, *dst, a;
	ULONG clut_bpr, clut_mod;
	LONG pen;
	WORD x, y;
	struct BitMap *bm = NULL, *temp_bm = NULL, mask_bm;
	PLANEPTR mask = NULL;
	struct RastPort rp, temp_rp;
	BOOL interleaved;
	WORD mask_depth, i;
	ULONG mask_bpr;

	/* allocate space for chunky image */
	clut_bpr = (w + 15) & 0xFFF0;
	clut = AllocVec(clut_bpr * h, MEMF_ANY);
	if (!clut) {
		goto error;
	}

	/* remap image to chunky buffer */
	src = rgba;
	dst = clut;
	clut_mod = clut_bpr - w;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			pen = ObtainBestPenA(cmap, RGB8to32(src[0]), RGB8to32(src[1]), RGB8to32(src[2]), tags);
			if (!(pen >= 0 && pen <= 255)) {
				goto error;
			}
			if (PenMap_SetPen(pen_map, pen)) {
				ReleasePen(cmap, pen);
			}
			src += 4;
			*dst++ = pen;
		}
		dst += clut_mod;
	}
	
	/* allocate bitmap in same format as screen */
	bm = AllocBitMap(w, h, depth, 0, screen_bm);
	if (!bm) {
		goto error;
	}
	
	if (GfxBase->LibNode.lib_Version < 40) {
		/* allocate temp bitmap and rp for WritePixelArray8 */
		temp_bm = AllocBitMap(w, 1, depth, 0, bm);
		if (!temp_bm) {
			goto error;
		}
		InitRastPort(&temp_rp);
		temp_rp.BitMap = temp_bm;
	}

	/* init rastport and copy graphics to bitmap */
	InitRastPort(&rp);
	rp.BitMap = bm;
	if (GfxBase->LibNode.lib_Version >= 40) {
		WriteChunkyPixels(&rp, 0, 0, w-1, h-1, clut, clut_bpr);
	} else {
		WritePixelArray8(&rp, 0, 0, w-1, h-1, clut, &temp_rp);
	}
	
	/* allocate memory for mask */
	interleaved = !rtg && (bm->Flags & BMF_INTERLEAVED);
	mask_depth = interleaved ? depth : 1;
	mask_bpr = rtg ? (((w + 15) >> 3) & 0xFFFE) : (bm->BytesPerRow / mask_depth);
	mask = AllocVec(mask_bpr * h * mask_depth, (rtg ? MEMF_ANY : MEMF_CHIP)|MEMF_CLEAR);
	if (!mask) {
		goto error;
	}
	
	/* init mask bitmap structure */
	memset(&mask_bm, 0, sizeof(mask_bm));
	mask_bm.BytesPerRow = mask_bpr * mask_depth;
	mask_bm.Rows = h;
	mask_bm.Flags = interleaved ? BMF_INTERLEAVED : 0;
	mask_bm.Depth = mask_depth;
	src = (UBYTE *)mask;
	for (i = 0; i < mask_depth; i++) {
		mask_bm.Planes[i] = (PLANEPTR)src;
		src += mask_bpr;
	}
	
	/* draw mask into chunky buffer first */
	pen = (1UL << mask_depth) - 1;
	src = rgba;
	dst = clut;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			a = src[3];
			*dst++ = a >= 128 ? pen : 0;
			src += 4;
		}
		dst += clut_mod;
	}
	
	/* init rastport and copy mask to bitmap */
	InitRastPort(&rp);
	rp.BitMap = &mask_bm;
	if (GfxBase->LibNode.lib_Version >= 40) {
		WriteChunkyPixels(&rp, 0, 0, w-1, h-1, clut, clut_bpr);
	} else {
		WritePixelArray8(&rp, 0, 0, w-1, h-1, clut, &temp_rp);
	}

	FreeBitMap(temp_bm);
	FreeVec(clut);
	
	/* success */
	*bm_p = bm;
	*mask_p = mask;
	return TRUE;
	
error:
	FreeBitMap(temp_bm);
	FreeBitMap(bm);
	FreeVec(mask);
	FreeVec(clut);
	
	/* failure */
	*bm_p = NULL;
	*mask_p = NULL;
	return FALSE;
}
