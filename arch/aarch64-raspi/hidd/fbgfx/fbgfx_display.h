#ifndef FBGFX_DISPLAY_H
#define FBGFX_DISPLAY_H

/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: FB Gfx display class data.
    Lang: English.
*/

#include <exec/types.h>

#define CLID_Hidd_Display_FB	"hidd.display.vcfb"
#define IID_Hidd_Display_FB	"hidd.display.vcfb"

struct FBGfxDisplayData
{
    void *pad;
};

#endif /* FBGFX_DISPLAY_H */
