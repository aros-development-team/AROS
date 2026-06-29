#ifndef VMWARESVGA_HIDD_H
#define VMWARESVGA_HIDD_H

/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.

    Desc: VMWareSVGA Gfx Hidd data.
*/

#include <exec/interrupts.h>

#include "vmwaresvga_bitmap.h"

#define IID_Hidd_VMWareSVGA  "hidd.gfx.vmwaresvga"
#define CLID_Hidd_VMWareSVGA "hidd.gfx.vmwaresvga"
#define CLID_Hidd_Display_VMWareSVGA "hidd.display.vmwaresvga"

struct VMWareSVGAHiddData
{
    struct Interrupt ResetInterrupt;
};

struct VMWareSVGADisplayData
{
};

#endif /* VESAGFX_HIDD_H */
