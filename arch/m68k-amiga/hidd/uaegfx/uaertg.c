
#include <proto/oop.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>

#include "uaegfx.h"
#include "uaegfxbitmap.h"
#include "uaertg.h"
#include "p96call.h"

BOOL FindCard(struct uaegfx_staticdata *csd)
{
    return P96_LC1(BOOL, csd->uaeromvector, 16,
    	AROS_LCA(APTR, csd->boardinfo, A0));
}
BOOL InitCard(struct uaegfx_staticdata *csd)
{
    return P96_LC2(BOOL, csd->uaeromvector, 29,
    	AROS_LCA(APTR, csd->boardinfo, A0),       // For current WinUAEs
    	AROS_LCA(APTR, csd->boardinfo, A2));      // For older E-UAEs
}
BOOL SetDisplay(struct uaegfx_staticdata *csd, BOOL state)
{
    return P96_LC2(BOOL, csd->uaeromvector, 26,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(BOOL, state, D0));
}
BOOL SetSwitch(struct uaegfx_staticdata *csd, BOOL state)
{
    return P96_LC2(BOOL, csd->uaeromvector, 18,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(BOOL, state, D0));
}
void SetColorArray(struct uaegfx_staticdata *csd, UWORD start, UWORD count)
{
    P96_LC3(BOOL, csd->uaeromvector, 19,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(WORD, start, D0),
    	AROS_LCA(WORD, count, D1));
}
void SetDAC(struct uaegfx_staticdata *csd)
{
    P96_LC2(BOOL, csd->uaeromvector, 20,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(ULONG, csd->rgbformat, D7));
}
void SetGC(struct uaegfx_staticdata *csd, struct ModeInfo *mi, BOOL border)
{
    P96_LC3(BOOL, csd->uaeromvector, 21,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, mi, A1),
    	AROS_LCA(BOOL, border, D0));
}
void SetPanning(struct uaegfx_staticdata *csd, UBYTE *video, UWORD width, WORD x, WORD y)
{
    P96_LC6(BOOL, csd->uaeromvector, 22,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, video, A1),
    	AROS_LCA(UWORD, width, D0),
    	AROS_LCA(WORD, x, D1),
    	AROS_LCA(WORD, y, D2),
    	AROS_LCA(ULONG, csd->rgbformat, D7));
}
BOOL FillRect(struct uaegfx_staticdata *csd, struct RenderInfo *ri, WORD x, WORD y, WORD w, WORD h, ULONG pen, UBYTE mask, ULONG rgbformat)
{
    return P96_LC9(BOOL, csd->uaeromvector, 17,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, ri, A1),
    	AROS_LCA(WORD, x, D0),
    	AROS_LCA(WORD, y, D1),
    	AROS_LCA(WORD, w, D2),
    	AROS_LCA(WORD, h, D3),
    	AROS_LCA(ULONG, pen, D4),
    	AROS_LCA(UBYTE, mask, D5),
    	AROS_LCA(ULONG, rgbformat, D7));
}
BOOL InvertRect(struct uaegfx_staticdata *csd, struct RenderInfo *ri, WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    return P96_LC8(BOOL, csd->uaeromvector, 31,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, ri, A1),
    	AROS_LCA(WORD, x, D0),
    	AROS_LCA(WORD, y, D1),
    	AROS_LCA(WORD, w, D2),
    	AROS_LCA(WORD, h, D3),
     	AROS_LCA(UBYTE, mask, D4),
    	AROS_LCA(ULONG, rgbformat, D7));
}
BOOL BlitRectNoMaskComplete(struct uaegfx_staticdata *csd, struct RenderInfo *risrc, struct RenderInfo *ridst,
    WORD sx, WORD sy, WORD dx, WORD dy, WORD w, WORD h, UBYTE opcode, ULONG rgbformat)
{
    return P96_LC11(BOOL, csd->uaeromvector, 28,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, risrc, A1),
    	AROS_LCA(APTR, ridst, A2),
    	AROS_LCA(WORD, sx, D0),
    	AROS_LCA(WORD, sy, D1),
    	AROS_LCA(WORD, dx, D2),
    	AROS_LCA(WORD, dy, D3),
    	AROS_LCA(WORD, w, D4),
    	AROS_LCA(WORD, h, D5),
     	AROS_LCA(UBYTE, opcode, D6),
    	AROS_LCA(ULONG, rgbformat, D7));
};
BOOL BlitTemplate(struct uaegfx_staticdata *csd, struct RenderInfo *ri, struct Template *tmpl,
    WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    return P96_LC9(BOOL, csd->uaeromvector, 27,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(APTR, ri, A1),
    	AROS_LCA(APTR, tmpl, A2),
    	AROS_LCA(WORD, x, D0),
    	AROS_LCA(WORD, y, D1),
    	AROS_LCA(WORD, w, D2),
    	AROS_LCA(WORD, h, D3),
     	AROS_LCA(UBYTE, mask, D4),
    	AROS_LCA(ULONG, rgbformat, D7));
}

WORD CalculateBytesPerRow(struct uaegfx_staticdata *csd, WORD width, ULONG rgbformat)
{
    return P96_LC3(BOOL, csd->uaeromvector, 23,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(UWORD, width, D0),
    	AROS_LCA(ULONG, rgbformat, D7));
}

BOOL SetSprite(struct uaegfx_staticdata *csd, BOOL activate)
{
    return P96_LC3(BOOL, csd->uaeromvector, 36,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(BOOL, activate, D0),
     	AROS_LCA(ULONG, csd->rgbformat, D7));
}	

BOOL SetSpritePosition(struct uaegfx_staticdata *csd)
{
    return P96_LC2(BOOL, csd->uaeromvector, 37,
    	AROS_LCA(APTR, csd->boardinfo, A0),
     	AROS_LCA(ULONG, csd->rgbformat, D7));
}	

BOOL SetSpriteImage(struct uaegfx_staticdata *csd)
{
    return P96_LC2(BOOL, csd->uaeromvector, 38,
    	AROS_LCA(APTR, csd->boardinfo, A0),
     	AROS_LCA(ULONG, csd->rgbformat, D7));
}

BOOL SetSpriteColor(struct uaegfx_staticdata *csd, UBYTE idx, UBYTE r, UBYTE g, UBYTE b)
{
    return P96_LC6(BOOL, csd->uaeromvector, 39,
    	AROS_LCA(APTR, csd->boardinfo, A0),
    	AROS_LCA(UBYTE, idx, D0),
    	AROS_LCA(UBYTE, r, D1),
    	AROS_LCA(UBYTE, g, D2),
    	AROS_LCA(UBYTE, b, D3),
    	AROS_LCA(ULONG, csd->rgbformat, D7));
}	

WORD getrtgdepth(ULONG rgbformat)
{
    if (rgbformat & RGBFF_CLUT)
    	return 8;
    if (rgbformat & (RGBFF_R5G5B5PC | RGBFF_R5G5B5 | RGBFF_B5G5R5PC))
	return 15;
    if (rgbformat & (RGBFF_R5G6B5PC | RGBFF_R5G6B5 | RGBFF_B5G6R5PC))
	return 16;
    if (rgbformat & (RGBFF_R8G8B8 | RGBFF_B8G8R8))
	return 24;
    if (rgbformat & (RGBFF_A8R8G8B8 | RGBFF_A8B8G8R8 | RGBFF_R8G8B8A8 | RGBFF_B8G8R8A8))
	return 32;
    return 0;
}

ULONG getrtgformat(struct uaegfx_staticdata *csd, OOP_Object *pixfmt)
{
    IPTR depth, redmask, greenmask, bluemask;

    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask, &redmask);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_GreenMask, &greenmask);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask, &bluemask);

    if (depth == 8)
    	return RGBFB_CLUT;
    if (depth == 16)
    	return RGBFB_R5G6B5PC;
    if (redmask == 0x0000ff00)
    	return RGBFB_B8G8R8A8;
    if (redmask == 0xff000000)
    	return RGBFB_R8G8B8A8;
    if (redmask == 0x000000ff)
    	return RGBFB_A8B8G8R8;
    if (redmask == 0x00ff0000)
    	return RGBFB_A8R8G8B8;
    return RGBFB_NONE;
}

void makerenderinfo(struct uaegfx_staticdata *csd, struct RenderInfo *ri, struct bm_data *bm)
{
    ri->Memory = bm->VideoData;
    ri->BytesPerRow = bm->bytesperline;
    ri->RGBFormat = bm->rgbformat;
}

void getrtgmodeinfo(struct uaegfx_staticdata *csd, OOP_Object *sync, OOP_Object *pixfmt, struct ModeInfo *modeinfo)
{
    IPTR width, height, depth;

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);

    modeinfo->Width = width;
    modeinfo->Height = height;
    modeinfo->Depth = depth;
    // UAE RTG driver does not need anything else
}

const UBYTE modetable[16] =
	{  0, 8, 4, 12,  2, 10, 6, 14,  7, 9, 5, 13,  3, 11, 1, 15 };

ULONG gl(UBYTE *p)
{
    return ((ULONG*)p)[0];
}
UWORD gw(UBYTE *p)
{
    return ((UWORD*)p)[0];
}
void pl(UBYTE *p, ULONG l)
{
    ((ULONG*)p)[0] = l;
}
void pw(UBYTE *p, WORD w)
{
    ((WORD*)p)[0] = w;
}
void pb(UBYTE *p, BYTE b)
{
    ((BYTE*)p)[0] = b;
}
