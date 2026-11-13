/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <graphics/sprite.h>
#include <hidd/gfx.h>

#include "intuition_intern.h"

/* Targets may replace these defaults through build_archspecific. */
void InitDefaultPreferences(struct Preferences *prefs)
{
}

void PreparePointerSprite(struct ExtSprite *sprite, struct BitMap *bitmap,
    BOOL legacy, IPTR width, struct IntuitionBase *IntuitionBase)
{
}

BOOL SetDisplayPointerShape(OOP_Object *display,
    struct SharedPointer *pointer, struct BitMap *bitmap,
    struct IntuitionBase *IntuitionBase)
{
    OOP_MethodID HiddDisplayBase = GetPrivIBase(IntuitionBase)->ib_HiddDisplayBase;

    return HIDD_Display_SetCursorShape(display,
        HIDD_BM_OBJ(bitmap), pointer->xoffset, pointer->yoffset);
}
