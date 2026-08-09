#ifndef EFIGFX_HIDD_H
#define EFIGFX_HIDD_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFI framebuffer Gfx Hidd data.
*/

#include <exec/interrupts.h>

#include "efigfx_bitmap.h"
#include "efigfx_support.h"

#define IID_Hidd_Gfx_EFI  "hidd.gfx.efi"
#define CLID_Hidd_Gfx_EFI "hidd.gfx.efi"

struct EFIGfxHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* EFIGFX_HIDD_H */
