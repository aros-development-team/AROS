/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Private helpers for generic software LUT8 display modes.
*/

#ifndef GFX_SOFTMODES_H
#define GFX_SOFTMODES_H

#include <exec/types.h>
#include <oop/oop.h>
#include <hidd/gfx.h>

BOOL DMEnum__Internal__IsSoftwareMode(OOP_Class *cl, OOP_Object *o, HIDDT_ModeID modeID);
HIDDT_ModeID DMEnum__Internal__GetBackingMode(OOP_Class *cl, OOP_Object *o, HIDDT_ModeID modeID);
BOOL DMEnum__Internal__HasNativeModes(OOP_Class *cl, OOP_Object *o);
VOID DMEnum__Internal__DisableSoftwareModes(OOP_Class *cl, OOP_Object *o);
BOOL Compositor__Internal__CanHandleSoftware(OOP_Class *cl, OOP_Object *o);
BOOL Compositor__Internal__EnsurePaletteTask(OOP_Class *cl, OOP_Object *o);

#endif /* GFX_SOFTMODES_H */
