#ifndef VMWARESVGA_HIDD_H
#define VMWARESVGA_HIDD_H

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.

    Desc: VMWareSVGA Gfx Hidd data.
*/

#include <exec/interrupts.h>

#include "vmwaresvga_bitmap.h"

#define IID_Hidd_VMWareSVGA  "hidd.gfx.vmwaresvga"
#define CLID_Hidd_VMWareSVGA "hidd.gfx.vmwaresvga"

struct VMWareSVGAHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* VESAGFX_HIDD_H */
