#ifndef FBGFX_HIDD_H
#define FBGFX_HIDD_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FB Gfx Hidd data.
    Lang: English.
*/

#include <exec/interrupts.h>

#include "fbgfx_bitmap.h"
#include "fbgfx_support.h"

#define IID_Hidd_Gfx_FB  "hidd.gfx.vcfb"
#define CLID_Hidd_Gfx_FB "hidd.gfx.vcfb"

struct FBGfxHiddData
{
    struct Interrupt ResetInterrupt;
};

#endif /* FBGFX_HIDD_H */
