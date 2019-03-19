/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: Stubs used to call into a p96 card driver.
*/

#include <aros/debug.h>
#include <proto/oop.h>
#include <hidd/hidd.h>
#include <hidd/gfx.h>
#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <oop/oop.h>

#include "p96gfx_intern.h"
#include "p96gfx_bitmap.h"
#include "p96_rtg.h"
#include "p96call.h"

static APTR gptr(struct p96gfx_staticdata *csd, WORD offset)
{
    APTR code = (APTR)((ULONG*)(((UBYTE*)(csd->boardinfo)) + offset))[0];
    D(bug("->RTG off=%d code=%p\n", (offset - (PSSO_BoardInfo_AllocCardMem)) / 4, code));
#if 0
    UBYTE *board = gl(csd->boardinfo + PSSO_BoardInfo_MemoryBase);
    bug("%08x: %08x %08x %08x %08x\n",
        board,
        gl(board + 0),
        gl(board + 4),
        gl(board + 8),
        gl(board + 12));
#endif
    pw (csd->boardinfo + PSSO_BoardInfo_AROSFlag, 1);
    return code;
}

static AROS_UFH1(ULONG, RTGCall_Default,
    AROS_UFHA(APTR, boardinfo, A0))
{ 
    AROS_USERFUNC_INIT

    pw (boardinfo + PSSO_BoardInfo_AROSFlag, 0);
    return 0;

    AROS_USERFUNC_EXIT
}

/* Set fallback functions */
void InitRTG(APTR boardinfo)
{
    UWORD i;

    for (i = PSSO_BoardInfo_AllocCardMem; i <= PSSO_BoardInfo_DeleteFeature; i += 4)
        pl(boardinfo + i, (ULONG)RTGCall_Default);
}

BOOL FindCard(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_LVO_CALL1(BOOL,
            AROS_LCA(APTR, csd->boardinfo, A0),
            struct Library*, csd->CardBase, 5, );
    else
        return P96_LC1(BOOL, csd->p96romvector, 16,
            AROS_LCA(APTR, csd->boardinfo, A0));
}
BOOL InitCard(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_LVO_CALL2(BOOL,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, NULL, A1),
            struct Library*, csd->CardBase, 6, );
   else
        return P96_LC2(BOOL, csd->p96romvector, 29,
            AROS_LCA(APTR, csd->boardinfo, A0),       // For current WinP96s
            AROS_LCA(APTR, csd->boardinfo, A2));      // For older E-P96s
}

void WaitBlitter(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        AROS_CALL1NR(void, gptr(csd, PSSO_BoardInfo_WaitBlitter),
            AROS_LCA(APTR, csd->boardinfo, A0),
            struct Library*, csd->CardBase);
}

void SetInterrupt(struct p96gfx_staticdata *csd, ULONG state)
{
    if (csd->CardBase)
        AROS_CALL2(ULONG, gptr(csd, PSSO_BoardInfo_SetInterrupt),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, state, D0),
            struct Library*, csd->CardBase);
}

ULONG GetPixelClock(struct p96gfx_staticdata *csd, struct ModeInfo *mi, ULONG index, ULONG rgbformat)
{
    if (csd->CardBase)
        return AROS_CALL4(ULONG, gptr(csd, PSSO_BoardInfo_GetPixelClock),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, mi, A1),
            AROS_LCA(ULONG, index, D0),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        return -2;
}

void SetMemoryMode(struct p96gfx_staticdata *csd, ULONG rgbformat)
{
    if (csd->CardBase)
        AROS_CALL2(ULONG, gptr(csd, PSSO_BoardInfo_SetMemoryMode),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
}

ULONG ResolvePixelClock(struct p96gfx_staticdata *csd, struct ModeInfo *mi, ULONG pixelclock, ULONG rgbformat)
{
    if (csd->CardBase)
        return AROS_CALL4(ULONG, gptr(csd, PSSO_BoardInfo_ResolvePixelClock),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, mi, A1),
            AROS_LCA(ULONG, pixelclock, D0),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        return -2;
}
ULONG SetClock(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_CALL1(ULONG, gptr(csd, PSSO_BoardInfo_SetClock),
            AROS_LCA(APTR, csd->boardinfo, A0),
            struct Library*, csd->CardBase);
    else
        return -2;
}
BOOL SetDisplay(struct p96gfx_staticdata *csd, BOOL state)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, gptr(csd, PSSO_BoardInfo_SetDisplay),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, state, D0),
            struct Library*, csd->CardBase);
    else
        return P96_LC2(BOOL, csd->p96romvector, 26,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, state, D0));
}
BOOL SetSwitch(struct p96gfx_staticdata *csd, BOOL state)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, gptr(csd, PSSO_BoardInfo_SetSwitch),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, state, D0),
            struct Library*, csd->CardBase);
    else
        return P96_LC2(BOOL, csd->p96romvector, 18,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, state, D0));
}
void SetColorArray(struct p96gfx_staticdata *csd, UWORD start, UWORD count)
{
    if (csd->CardBase)
        AROS_CALL3(BOOL, gptr(csd, PSSO_BoardInfo_SetColorArray),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(WORD, start, D0),
            AROS_LCA(WORD, count, D1),
            struct Library*, csd->CardBase);
    else
        P96_LC3(BOOL, csd->p96romvector, 19,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(WORD, start, D0),
            AROS_LCA(WORD, count, D1));
}
void SetDAC(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        AROS_CALL2(BOOL, gptr(csd, PSSO_BoardInfo_SetDAC),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        P96_LC2(BOOL, csd->p96romvector, 20,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, csd->rgbformat, D7));
}
void SetGC(struct p96gfx_staticdata *csd, struct ModeInfo *mi, BOOL border)
{
    if (csd->CardBase)
        AROS_CALL3(BOOL, gptr(csd, PSSO_BoardInfo_SetGC),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, mi, A1),
            AROS_LCA(BOOL, border, D0),
            struct Library*, csd->CardBase);
    else
        P96_LC3(BOOL, csd->p96romvector, 21,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, mi, A1),
            AROS_LCA(BOOL, border, D0));
}
void SetPanning(struct p96gfx_staticdata *csd, UBYTE *video, UWORD width, WORD x, WORD y)
{
    if (csd->CardBase)
        AROS_CALL6(BOOL, gptr(csd, PSSO_BoardInfo_SetPanning),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, video, A1),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(WORD, x, D1),
            AROS_LCA(WORD, y, D2),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        P96_LC6(BOOL, csd->p96romvector, 22,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, video, A1),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(WORD, x, D1),
            AROS_LCA(WORD, y, D2),
            AROS_LCA(ULONG, csd->rgbformat, D7));
}

BOOL DrawLine(struct p96gfx_staticdata *csd, struct RenderInfo *ri,
    struct Line * line, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL4(BOOL, gptr(csd, PSSO_BoardInfo_DrawLine),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(struct Line *, line, A2),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } 
#if (0)
    else
        return P96_LC4(BOOL, csd->p96romvector, 28,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(struct Line *, line, A2),
            AROS_LCA(ULONG, rgbformat, D7));
#else
    return FALSE;
#endif
};

BOOL BlitRect(struct p96gfx_staticdata *csd, struct RenderInfo *ri,
    WORD sx, WORD sy, WORD dx, WORD dy, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL10(BOOL, gptr(csd, PSSO_BoardInfo_BlitRect),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(WORD, sx, D0),
            AROS_LCA(WORD, sy, D1),
            AROS_LCA(WORD, dx, D2),
            AROS_LCA(WORD, dy, D3),
            AROS_LCA(WORD, w, D4),
            AROS_LCA(WORD, h, D5),
            AROS_LCA(UBYTE, mask, D6),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    }
#if (0)
    else
        return P96_LC10(BOOL, csd->p96romvector, 28,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(WORD, sx, D0),
            AROS_LCA(WORD, sy, D1),
            AROS_LCA(WORD, dx, D2),
            AROS_LCA(WORD, dy, D3),
            AROS_LCA(WORD, w, D4),
            AROS_LCA(WORD, h, D5),
            AROS_LCA(UBYTE, mask, D6),
            AROS_LCA(ULONG, rgbformat, D7));
#else
    return FALSE;
#endif
};

BOOL FillRect(struct p96gfx_staticdata *csd, struct RenderInfo *ri, WORD x, WORD y, WORD w, WORD h, ULONG pen, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL9(BOOL, gptr(csd, PSSO_BoardInfo_FillRect),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(ULONG, pen, D4),
            AROS_LCA(UBYTE, mask, D5),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } else
        return P96_LC9(BOOL, csd->p96romvector, 17,
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
BOOL InvertRect(struct p96gfx_staticdata *csd, struct RenderInfo *ri, WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL8(BOOL, gptr(csd, PSSO_BoardInfo_InvertRect),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(UBYTE, mask, D4),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } else
        return P96_LC8(BOOL, csd->p96romvector, 31,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(UBYTE, mask, D4),
            AROS_LCA(ULONG, rgbformat, D7));
}
BOOL BlitRectNoMaskComplete(struct p96gfx_staticdata *csd, struct RenderInfo *risrc, struct RenderInfo *ridst,
    WORD sx, WORD sy, WORD dx, WORD dy, WORD w, WORD h, UBYTE opcode, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL11(BOOL, gptr(csd, PSSO_BoardInfo_BlitRectNoMaskComplete),
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
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } else
        return P96_LC11(BOOL, csd->p96romvector, 28,
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
BOOL BlitPattern(struct p96gfx_staticdata *csd, struct RenderInfo *ri, struct Pattern *pat,
    WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL9(BOOL, gptr(csd, PSSO_BoardInfo_BlitPattern),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(APTR, pat, A2),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(UBYTE, mask, D4),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } else
        return P96_LC9(BOOL, csd->p96romvector, 30,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(APTR, pat, A2),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(UBYTE, mask, D4),
            AROS_LCA(ULONG, rgbformat, D7));
}

BOOL BlitTemplate(struct p96gfx_staticdata *csd, struct RenderInfo *ri, struct Template *tmpl,
    WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL9(BOOL, gptr(csd, PSSO_BoardInfo_BlitTemplate),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, ri, A1),
            AROS_LCA(APTR, tmpl, A2),
            AROS_LCA(WORD, x, D0),
            AROS_LCA(WORD, y, D1),
            AROS_LCA(WORD, w, D2),
            AROS_LCA(WORD, h, D3),
            AROS_LCA(UBYTE, mask, D4),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
        return gw (csd->boardinfo + PSSO_BoardInfo_AROSFlag);
    } else
        return P96_LC9(BOOL, csd->p96romvector, 27,
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

WORD CalculateBytesPerRow(struct p96gfx_staticdata *csd, WORD width, ULONG rgbformat)
{
    if (csd->CardBase)
        return AROS_CALL3(BOOL, gptr(csd, PSSO_BoardInfo_CalculateBytesPerRow),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        return P96_LC3(BOOL, csd->p96romvector, 23,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(ULONG, rgbformat, D7));
}

BOOL SetSprite(struct p96gfx_staticdata *csd, BOOL activate)
{
    if (csd->CardBase)
        return AROS_CALL3(BOOL, gptr(csd, PSSO_BoardInfo_SetSprite),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, activate, D0),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC3(BOOL, csd->p96romvector, 36,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(BOOL, activate, D0),
        AROS_LCA(ULONG, csd->rgbformat, D7));
}	

BOOL SetSpritePosition(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, gptr(csd, PSSO_BoardInfo_SetSpritePosition),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC2(BOOL, csd->p96romvector, 37,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(ULONG, csd->rgbformat, D7));
}	

BOOL SetSpriteImage(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, gptr(csd, PSSO_BoardInfo_SetSpriteImage),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC2(BOOL, csd->p96romvector, 38,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(ULONG, csd->rgbformat, D7));
}

BOOL SetSpriteColor(struct p96gfx_staticdata *csd, UBYTE idx, UBYTE r, UBYTE g, UBYTE b)
{
    if (csd->CardBase)
        return AROS_CALL6(BOOL, gptr(csd, PSSO_BoardInfo_SetSpriteColor),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(UBYTE, idx, D0),
            AROS_LCA(UBYTE, r, D1),
            AROS_LCA(UBYTE, g, D2),
            AROS_LCA(UBYTE, b, D3),
            AROS_LCA(ULONG, csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC6(BOOL, csd->p96romvector, 39,
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

ULONG getrtgformat(struct p96gfx_staticdata *csd, OOP_Object *pixfmt)
{
    IPTR depth, redmask, bluemask, endianswitch;

    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_RedMask, &redmask);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_BlueMask, &bluemask);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_SwapPixelBytes, &endianswitch);

    if (depth == 8)
        return RGBFB_CLUT;
    if (depth == 15) {
        if (redmask == 0x00007c00 && !endianswitch)
            return RGBFB_R5G5B5;
        if (redmask == 0x00007c00 && endianswitch)
            return RGBFB_R5G5B5PC;
        if (redmask == 0x0000003e && bluemask == 0x0000f800)
            return RGBFB_B5G5R5PC;
    }
    if (depth == 16) {
        if (redmask == 0x0000f800 && !endianswitch)
            return RGBFB_R5G6B5;
        if (redmask == 0x0000f800 && endianswitch)
            return RGBFB_R5G6B5PC;
        if (redmask == 0x0000001f && bluemask == 0x0000f800)
            return RGBFB_B5G6R5PC;
    }
    if (depth == 32) {
        if (redmask == 0x0000ff00)
           return RGBFB_B8G8R8A8;
        if (redmask == 0xff000000)
           return RGBFB_R8G8B8A8;
        if (redmask == 0x000000ff)
           return RGBFB_A8B8G8R8;
        if (redmask == 0x00ff0000)
           return RGBFB_A8R8G8B8;
    } else if (depth == 24) {
        if (redmask == 0x000000ff)
           return RGBFB_B8G8R8;
        if (redmask == 0x00ff0000)
           return RGBFB_R8G8B8;
    }
    D(bug("getrtgformat RGBFB_NONE!? %d %08x %08x\n", depth, redmask, bluemask));
    return RGBFB_NONE;
}

void makerenderinfo(struct p96gfx_staticdata *csd, struct RenderInfo *ri, struct bm_data *bm)
{
    ri->Memory = bm->VideoData;
    ri->BytesPerRow = bm->bytesperline;
    ri->RGBFormat = bm->rgbformat;
}

struct ModeInfo *getrtgmodeinfo(struct p96gfx_staticdata *csd, OOP_Object *sync, OOP_Object *pixfmt, struct ModeInfo *modeinfo)
{
    struct LibResolution *node;
    IPTR width, height, depth;

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &width);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &height);
    OOP_GetAttr(pixfmt, aHidd_PixFmt_Depth, &depth);

    D(bug("getrtgmodeinfo %dx%dx%d\n", width, height, depth));
    // P96 RTG driver does not need anything else
    // but real RTG does
    ForeachNode((csd->boardinfo + PSSO_BoardInfo_ResolutionsList), node) {
        if (node->Width == width && node->Height == height) {
            UBYTE index = (depth + 7) / 8;
            if (node->Modes[index]) {
                D(bug("RTG ModeInfo found %p\n", node->Modes[index]));
                return node->Modes[index];
            }
        }
    }
    D(bug("using fake modeinfo\n"));
    modeinfo->Width = width;
    modeinfo->Height = height;
    modeinfo->Depth = depth;
    return modeinfo;    
}

const UBYTE modetable[16] =
        {  0, 8, 4, 12,  2, 10, 6, 14,  7, 9, 5, 13,  3, 11, 1, 15 };

APTR gp(UBYTE *p)
{
    return ((APTR*)p)[0];
}
ULONG gl(UBYTE *p)
{
    return ((ULONG*)p)[0];
}
UWORD gw(UBYTE *p)
{
    return ((UWORD*)p)[0];
}
void pp(UBYTE *p, APTR a)
{
    ((APTR*)p)[0] = a;
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
