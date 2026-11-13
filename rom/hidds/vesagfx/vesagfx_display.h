#ifndef VESAGFX_DISPLAY_H
#define VESAGFX_DISPLAY_H

/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: VESA Gfx display class data.
    Lang: English.
*/

#include <exec/types.h>

#define CLID_Hidd_Display_VESA	"hidd.display.vesa"
#define IID_Hidd_Display_VESA	"hidd.display.vesa"

struct VESAGfxDisplayData
{
    void *pad;
};

#endif /* VESAGFX_DISPLAY_H */
