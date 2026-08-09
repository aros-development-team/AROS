#ifndef EFIGFX_DISPLAY_H
#define EFIGFX_DISPLAY_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx display class data.
*/

#include <exec/types.h>

#define CLID_Hidd_Display_EFI	"hidd.display.efi"
#define IID_Hidd_Display_EFI	"hidd.display.efi"

struct EFIGfxDisplayData
{
    void *pad;
};

#endif /* EFIGFX_DISPLAY_H */
