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

#ifndef CLASS_H
#define CLASS_H

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif
#ifndef INTUITION_IMAGECLASS_H
#include <intuition/imageclass.h>
#endif
#ifndef IMAGES_BITMAP_H
#include <images/bitmap.h>
#endif
#ifndef TYPES_H
#include <types.h>
#endif
#ifndef SDI_COMPILER_H
#include <SDI_compiler.h>
#endif

struct ClassBase {
	struct Library libNode;
	UWORD pad;
	Class *class;
	BPTR seglist;
};

#define PNG_Screen             BITMAP_Screen
#define PNG_SourceFile         BITMAP_SourceFile
#define PNG_SelectSourceFile   BITMAP_SelectSourceFile
#define PNG_DisabledSourceFile (BITMAP_Dummy + 19)

enum {
	IMG_NORMAL = 0,
	IMG_SELECTED,
	IMG_DISABLED
};

struct ClassData {
	struct Screen *screen;
	WORD width, height;
	UBYTE *image[3];
	ULONG pen_map[8];
	struct BitMap *bm[3];
	PLANEPTR mask[3];
	BOOL rtg;
	BOOL truecolor;
};

#define HAS_IMAGE(d,x) ((d)->truecolor ? (IPTR)(d)->image[x] : \
	(IPTR)((d)->bm[x] && (d)->mask[x]))

struct HookData {
	LONG x, y, w, h;
	UBYTE *image;
};

struct BackFillMessage {
	struct Layer     *Layer;
	struct Rectangle  Bounds;
	LONG              OffsetX;
	LONG              OffsetY;
};

/* class.c */
IPTR ClassDispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg));

/* loadpng.c */
BOOL LoadPNG (struct ClassData *data, const char *filename, LONG index);

/* remap.c */
void PenMap_ReleasePens (ULONG *pen_map, struct Screen *screen);
BOOL RemapRGBAImage (struct Screen *screen, BOOL rtg, UBYTE *rgba, WORD w, WORD h,
	ULONG *pen_map, struct BitMap **bm_p, PLANEPTR *mask_p);

/* writergbapixels_p96.c */
BOOL IsSupportedRGBFormat_P96 (ULONG rgbformat);
IPTR WriteRGBAPixels_P96 (REG(a0, struct Hook *hook), REG(a2, struct RastPort *rp),
	REG(a1, struct BackFillMessage *msg));

/* writergbapixels_cgx.c */
BOOL IsSupportedRGBFormat_CGX (ULONG rgbformat);
IPTR WriteRGBAPixels_CGX (REG(a0, struct Hook *hook), REG(a2, struct RastPort *rp),
	REG(a1, struct BackFillMessage *msg));

#endif
