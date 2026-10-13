/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/memory.h>
#include <graphics/monitor.h>
#include <graphics/sprite.h>
#include <hidd/amigavideo.h>
#include <hidd/gfx.h>
#include <proto/exec.h>

#include "intuition_intern.h"

void InitDefaultPreferences(struct Preferences *prefs)
{
    prefs->ViewInitX = STANDARD_VIEW_X;
    prefs->ViewInitY = STANDARD_VIEW_Y;
}

void PreparePointerSprite(struct ExtSprite *sprite, struct BitMap *bitmap,
    BOOL legacy, IPTR width, struct IntuitionBase *IntuitionBase)
{
    /* Keep eligible caller-owned DMA data alongside the converted bitmap
     * for applications which change pixels and control words in place. */
    if (legacy && width == 16 && (TypeOfMem(bitmap) & MEMF_CHIP))
        sprite->es_SimpleSprite.posctldata = (UWORD *)bitmap;
}

BOOL SetDisplayPointerShape(OOP_Object *display,
    struct SharedPointer *pointer, struct BitMap *bitmap,
    struct IntuitionBase *IntuitionBase)
{
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddDisplayAttrBase = GetPrivIBase(IntuitionBase)->HiddDisplayAttrBase;
    OOP_MethodID HiddDisplayBase = GetPrivIBase(IntuitionBase)->ib_HiddDisplayBase;

    if (pointer->sprite->es_SimpleSprite.posctldata)
    {
        OOP_Object *gfx = NULL;

        OOP_GetAttr(display, aHidd_Display_GfxHidd, (IPTR *)&gfx);
        if (gfx && OOP_OCLASS(gfx) == OOP_FindClass(CLID_Hidd_Gfx_AmigaVideo))
        {
            OOP_MethodID HiddAmigaGfxBase = OOP_GetMethodID(IID_Hidd_AmigaGfx, 0);

            if (HIDD_AMIGAGFX_SetClassicCursorData(gfx,
                pointer->sprite->es_SimpleSprite.posctldata,
                pointer->sprite->es_SimpleSprite.height,
                pointer->xoffset, pointer->yoffset))
                return TRUE;
        }
    }

    return HIDD_Display_SetCursorShape(display,
        HIDD_BM_OBJ(bitmap), pointer->xoffset, pointer->yoffset);
}
