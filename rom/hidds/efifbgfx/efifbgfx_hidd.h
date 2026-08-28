#ifndef EFIFBGFX_HIDD_H
#define EFIFBGFX_HIDD_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx Hidd data.
*/

#include <exec/interrupts.h>

#include "efifbgfx_bitmap.h"
#include "efifbgfx_support.h"

#define IID_Hidd_Gfx_EFIFB  "hidd.gfx.efifb"
#define CLID_Hidd_Gfx_EFIFB "hidd.gfx.efifb"

struct EFIFBGfxHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* EFIFBGFX_HIDD_H */
