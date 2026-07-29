#ifndef VCFBGFX_HIDD_H
#define VCFBGFX_HIDD_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: VCFB Gfx Hidd data.
    Lang: English.
*/

#include <exec/interrupts.h>

#include "vcgfx_bitmap.h"
#include "vcgfx_support.h"

#define IID_Hidd_Gfx_VCFB  "hidd.gfx.vcfb"
#define CLID_Hidd_Gfx_VCFB "hidd.gfx.vcfb"

struct VCGfxHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* VCFBGFX_HIDD_H */
