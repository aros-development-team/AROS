/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
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
#include "p96gfx_rtg.h"
#include "p96call.h"

static inline APTR __cardFunc(struct p96gfx_staticdata *csd, WORD offset)
{
    APTR code = (APTR)((ULONG*)(((UBYTE*)(csd->boardinfo)) + offset))[0];
    D(bug("->RTG off=%d code=%p\n", (offset - (PSSO_BoardInfo_AllocCardMem)) / 4, code));
    pw (csd->boardinfo + PSSO_BoardInfo_AROSFlag, 1);
    return code;
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
        AROS_CALL1NR(void, __cardFunc(csd, PSSO_BoardInfo_WaitBlitter),
            AROS_LCA(APTR, csd->boardinfo, A0),
            struct Library*, csd->CardBase);
}

void SetInterrupt(struct p96gfx_staticdata *csd, ULONG state)
{
    if (csd->CardBase)
        AROS_CALL2(ULONG, __cardFunc(csd, PSSO_BoardInfo_SetInterrupt),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, state, D0),
            struct Library*, csd->CardBase);
}

ULONG GetPixelClock(struct p96gfx_staticdata *csd, struct ModeInfo *mi, ULONG index, ULONG rgbformat)
{
    if (csd->CardBase)
        return AROS_CALL4(ULONG, __cardFunc(csd, PSSO_BoardInfo_GetPixelClock),
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
        AROS_CALL2(ULONG, __cardFunc(csd, PSSO_BoardInfo_SetMemoryMode),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, rgbformat, D7),
            struct Library*, csd->CardBase);
}

ULONG ResolvePixelClock(struct p96gfx_staticdata *csd, struct ModeInfo *mi, ULONG pixelclock, ULONG rgbformat)
{
    if (csd->CardBase)
        return AROS_CALL4(ULONG, __cardFunc(csd, PSSO_BoardInfo_ResolvePixelClock),
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
        return AROS_CALL1(ULONG, __cardFunc(csd, PSSO_BoardInfo_SetClock),
            AROS_LCA(APTR, csd->boardinfo, A0),
            struct Library*, csd->CardBase);
    else
        return -2;
}

BOOL SetDisplay(struct p96gfx_staticdata *csd, BOOL state)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetDisplay),
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
        return AROS_CALL2(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetSwitch),
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
        AROS_CALL3(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetColorArray),
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
        AROS_CALL2(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetDAC),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        P96_LC2(BOOL, csd->p96romvector, 20,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, *csd->rgbformat, D7));
}

void SetGC(struct p96gfx_staticdata *csd, struct ModeInfo *mi, BOOL border)
{
    if (csd->CardBase)
        AROS_CALL3(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetGC),
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
        AROS_CALL6(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetPanning),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, video, A1),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(WORD, x, D1),
            AROS_LCA(WORD, y, D2),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    else
        P96_LC6(BOOL, csd->p96romvector, 22,
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(APTR, video, A1),
            AROS_LCA(UWORD, width, D0),
            AROS_LCA(WORD, x, D1),
            AROS_LCA(WORD, y, D2),
            AROS_LCA(ULONG, *csd->rgbformat, D7));
}

BOOL DrawLine(struct p96gfx_staticdata *csd, struct RenderInfo *ri,
    struct Line * line, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL4(BOOL, __cardFunc(csd, PSSO_BoardInfo_DrawLine),
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
}

BOOL BlitRect(struct p96gfx_staticdata *csd, struct RenderInfo *ri,
    WORD sx, WORD sy, WORD dx, WORD dy, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL10(BOOL, __cardFunc(csd, PSSO_BoardInfo_BlitRect),
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
}

BOOL FillRect(struct p96gfx_staticdata *csd, struct RenderInfo *ri, WORD x, WORD y, WORD w, WORD h, ULONG pen, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL9(BOOL, __cardFunc(csd, PSSO_BoardInfo_FillRect),
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
        AROS_CALL8(BOOL, __cardFunc(csd, PSSO_BoardInfo_InvertRect),
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
        AROS_CALL11(BOOL, __cardFunc(csd, PSSO_BoardInfo_BlitRectNoMaskComplete),
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
}

BOOL BlitPattern(struct p96gfx_staticdata *csd, struct RenderInfo *ri, struct Pattern *pat,
    WORD x, WORD y, WORD w, WORD h, UBYTE mask, ULONG rgbformat)
{
    if (csd->CardBase) {
        AROS_CALL9(BOOL, __cardFunc(csd, PSSO_BoardInfo_BlitPattern),
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
        AROS_CALL9(BOOL, __cardFunc(csd, PSSO_BoardInfo_BlitTemplate),
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
        return AROS_CALL3(BOOL, __cardFunc(csd, PSSO_BoardInfo_CalculateBytesPerRow),
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
        return AROS_CALL3(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetSprite),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(BOOL, activate, D0),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC3(BOOL, csd->p96romvector, 36,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(BOOL, activate, D0),
        AROS_LCA(ULONG, *csd->rgbformat, D7));
}	

BOOL SetSpritePosition(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetSpritePosition),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC2(BOOL, csd->p96romvector, 37,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(ULONG, *csd->rgbformat, D7));
}	

BOOL SetSpriteImage(struct p96gfx_staticdata *csd)
{
    if (csd->CardBase)
        return AROS_CALL2(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetSpriteImage),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC2(BOOL, csd->p96romvector, 38,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(ULONG, *csd->rgbformat, D7));
}

BOOL SetSpriteColor(struct p96gfx_staticdata *csd, UBYTE idx, UBYTE r, UBYTE g, UBYTE b)
{
    if (csd->CardBase)
        return AROS_CALL6(BOOL, __cardFunc(csd, PSSO_BoardInfo_SetSpriteColor),
            AROS_LCA(APTR, csd->boardinfo, A0),
            AROS_LCA(UBYTE, idx, D0),
            AROS_LCA(UBYTE, r, D1),
            AROS_LCA(UBYTE, g, D2),
            AROS_LCA(UBYTE, b, D3),
            AROS_LCA(ULONG, *csd->rgbformat, D7),
            struct Library*, csd->CardBase);
    return P96_LC6(BOOL, csd->p96romvector, 39,
        AROS_LCA(APTR, csd->boardinfo, A0),
        AROS_LCA(UBYTE, idx, D0),
        AROS_LCA(UBYTE, r, D1),
        AROS_LCA(UBYTE, g, D2),
        AROS_LCA(UBYTE, b, D3),
        AROS_LCA(ULONG, *csd->rgbformat, D7));
}	
