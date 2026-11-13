#ifndef VESAGFX_HIDD_H
#define VESAGFX_HIDD_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VESA Gfx Hidd data.
    Lang: English.
*/

#include <exec/interrupts.h>

#include "vesagfx_bitmap.h"
#include "vesagfx_support.h"

#define IID_Hidd_Gfx_VESA  "hidd.gfx.vesa"
#define CLID_Hidd_Gfx_VESA "hidd.gfx.vesa"

struct VESAGfxHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* VESAGFX_HIDD_H */
