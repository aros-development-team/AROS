#ifndef EFIFBGFX_DISPLAY_H
#define EFIFBGFX_DISPLAY_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx display class data.
*/

#include <exec/types.h>

#define CLID_Hidd_Display_EFIFB	"hidd.display.efifb"
#define IID_Hidd_Display_EFIFB	"hidd.display.efifb"

struct EFIFBGfxDisplayData
{
    void *pad;
};

#endif /* EFIFBGFX_DISPLAY_H */
