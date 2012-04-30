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
#include <libraries/Picasso96.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/Picasso96API.h>
#include <proto/cybergraphics.h>
#include <clib/alib_protos.h>
#include <string.h>

#define IA(x) ((struct Image *)x)

static IPTR PNG_New (Class *cl, Object *o, struct opSet *ops);
static IPTR PNG_Dispose (Class *cl, Object *o, Msg msg);
static IPTR PNG_DomainFrame (Class *cl, Object *o, struct impDomainFrame *imp);
static IPTR PNG_Draw (Class *cl, Object *o, struct impDraw *imp);
static void PreMultiplyAlpha (UBYTE *rgba, WORD w, WORD h);
static void WriteRGBAPixels (struct RastPort *rp, UBYTE *image, WORD x, WORD y, WORD w, WORD h);

IPTR ClassDispatch (REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg)) {
	IPTR res;
	switch (msg->MethodID) {
		case OM_NEW:
			res = PNG_New(cl, o, (struct opSet *)msg);
			break;
		
		case OM_DISPOSE:
			res = PNG_Dispose(cl, o, msg);
			break;
		
		case IM_DOMAINFRAME:
			res = PNG_DomainFrame(cl, o, (struct impDomainFrame *)msg);
			break;
		
		case IM_DRAW:
			res = PNG_Draw(cl, o, (struct impDraw *)msg);
			break;
		
		default:
			res = DoSuperMethodA(cl, o, msg);
			break;
	}
	return res;
}

static IPTR PNG_New (Class *cl, Object *o, struct opSet *ops) {
	IPTR res;
	res = DoSuperMethodA(cl, o, (Msg)ops);
	if (res) {
		struct ClassData *data = INST_DATA(cl, (Object *)res);
		struct Screen *screen;
		const char *filename;
		int i;
		memset(data, 0, sizeof(*data));
		data->screen =
		screen = (struct Screen *)GetTagData(PNG_Screen, (IPTR)NULL, ops->ops_AttrList);
		if (!screen) {
			CoerceMethod(cl, (Object *)res, OM_DISPOSE);
			return (IPTR)NULL;
		}
		if (P96Base) {
			if (p96GetBitMapAttr(screen->RastPort.BitMap, P96BMA_ISP96)) {
				data->rtg = TRUE;
				data->truecolor = IsSupportedRGBFormat_P96(
					p96GetBitMapAttr(screen->RastPort.BitMap, P96BMA_RGBFORMAT)
				);
			}
		} else if (CyberGfxBase) {
			if (GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_ISCYBERGFX)) {
				data->rtg = TRUE;
				data->truecolor = IsSupportedRGBFormat_CGX(
					GetCyberMapAttr(screen->RastPort.BitMap, CYBRMATTR_PIXFMT)
				);
			}
		}
		filename = (const char *)GetTagData(PNG_SourceFile, (IPTR)NULL, ops->ops_AttrList);
		if (!filename || !LoadPNG(data, filename, IMG_NORMAL)) {
			CoerceMethod(cl, (Object *)res, OM_DISPOSE);
			return (IPTR)NULL;
		}
		filename = (const char *)GetTagData(PNG_SelectSourceFile, (IPTR)NULL, ops->ops_AttrList);
		if (filename && !LoadPNG(data, filename, IMG_SELECTED)) {
			CoerceMethod(cl, (Object *)res, OM_DISPOSE);
			return (IPTR)NULL;
		}
		filename = (const char *)GetTagData(PNG_DisabledSourceFile, (IPTR)NULL, ops->ops_AttrList);
		if (filename && !LoadPNG(data, filename, IMG_DISABLED)) {
			CoerceMethod(cl, (Object *)res, OM_DISPOSE);
			return (IPTR)NULL;
		}
		if (data->truecolor) {
			for (i = 0; i < 3; i++) {
				if (data->image[i]) {
					PreMultiplyAlpha(data->image[i], data->width, data->height);
				}
			}
		} else {
			for (i = 0; i < 3; i++) {
				if (data->image[i]) {
					if (!RemapRGBAImage(screen, data->rtg, data->image[i], data->width, data->height,
						data->pen_map, &data->bm[i], &data->mask[i]))
					{
						CoerceMethod(cl, (Object *)res, OM_DISPOSE);
						return (IPTR)NULL;
					}
					FreeVec(data->image[i]);
					data->image[i] = NULL;
				}
			}
		}
		IA(res)->Width = data->width;
		IA(res)->Height = data->height;
	}
	return res;
}

static IPTR PNG_Dispose (Class *cl, Object *o, Msg msg) {
	struct ClassData *data = INST_DATA(cl, o);
	if (data->truecolor) {
		FreeVec(data->image[IMG_NORMAL]);
		FreeVec(data->image[IMG_SELECTED]);
		FreeVec(data->image[IMG_DISABLED]);
	} else {
		FreeBitMap(data->bm[IMG_NORMAL]);
		FreeBitMap(data->bm[IMG_SELECTED]);
		FreeBitMap(data->bm[IMG_DISABLED]);
		FreeVec(data->mask[IMG_NORMAL]);
		FreeVec(data->mask[IMG_SELECTED]);
		FreeVec(data->mask[IMG_DISABLED]);
		PenMap_ReleasePens(data->pen_map, data->screen);
	}
	return DoSuperMethodA(cl, o, msg);
}

static IPTR PNG_DomainFrame (Class *cl, Object *o, struct impDomainFrame *imp) {
	struct ClassData *data = INST_DATA(cl, o);
	IPTR res = 0;
	switch (imp->imp_Which) {
		case IDOMAIN_MINIMUM:
		case IDOMAIN_NOMINAL:
		case IDOMAIN_MAXIMUM:
			imp->imp_Domain.Width = data->width;
			imp->imp_Domain.Height = data->height;
			res = 1;
			break;
	}
	return res;
}

static IPTR PNG_Draw (Class *cl, Object *o, struct impDraw *imp) {
	struct ClassData *data = INST_DATA(cl, o);
	struct RastPort *rp = imp->imp_RPort;
	LONG x, y;
	LONG index;
	x = IA(o)->LeftEdge + imp->imp_Offset.X;
	y = IA(o)->TopEdge + imp->imp_Offset.Y;
	index = IMG_NORMAL;
	switch (imp->imp_State) {
		case IDS_NORMAL:
			break;
		case IDS_SELECTED:
			if (HAS_IMAGE(data, IMG_SELECTED)) {
				index = IMG_SELECTED;
			}
			break;
		case IDS_DISABLED:
		case IDS_INACTIVEDISABLED:
		case IDS_SELECTEDDISABLED:
			if (HAS_IMAGE(data, IMG_DISABLED)) {
				index = IMG_DISABLED;
			}
			break;
	}
	if (data->truecolor) {
		WriteRGBAPixels(rp, data->image[index], x, y, data->width, data->height);
	} else {
		BltMaskBitMapRastPort(data->bm[index], 0, 0, rp, x, y, data->width, data->height,
			ABC|ABNC|ANBC, data->mask[index]);
	}
	return 0;
}

static void PreMultiplyAlpha (UBYTE *rgba, WORD w, WORD h) {
	WORD x, y;
	UWORD a;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			a = rgba[3];
			rgba[3] = 255 - a;
			if (a != 255) {
				rgba[0] = (rgba[0] * a) >> 8;
				rgba[1] = (rgba[1] * a) >> 8;
				rgba[2] = (rgba[2] * a) >> 8;
			}
			rgba += 4;
		}
	}
}

static void WriteRGBAPixels (struct RastPort *rp, UBYTE *image, WORD x, WORD y, WORD w, WORD h) {
	struct HookData data;
	struct Rectangle rect;
	struct Hook hook;
	data.x = x;
	data.y = y;
	data.w = w;
	data.h = h;
	data.image = image;
	rect.MinX = x;
	rect.MinY = y;
	rect.MaxX = x + w - 1;
	rect.MaxY = y + h - 1;
	memset(&hook, 0, sizeof(hook));
	hook.h_Data = &data;
	if (P96Base && p96GetBitMapAttr(rp->BitMap, P96BMA_ISP96)) {
		hook.h_Entry = (HOOKFUNC)WriteRGBAPixels_P96;
		DoHookClipRects(&hook, rp, &rect);
	} else if (CyberGfxBase && GetCyberMapAttr(rp->BitMap, CYBRMATTR_ISCYBERGFX)) {
		hook.h_Entry = (HOOKFUNC)WriteRGBAPixels_CGX;
		DoHookClipRects(&hook, rp, &rect);
	}
}
