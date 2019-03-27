/*
    Copyright © 1995-2019, The AROS Development Team. All rights reserved.
    $Id$
    
    Desc: p96 RTG support code.
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

const UBYTE modetable[16] =
        {  0, 8, 4, 12,  2, 10, 6, 14,  7, 9, 5, 13,  3, 11, 1, 15 };

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
