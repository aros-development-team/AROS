#ifndef _VMWARESVGA_INTERN_H
#define _VMWARESVGA_INTERN_H

/*
    Copyright 2010-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <hidd/gallium.h>

#include "svga/svga_winsys.h"
#include "svga/svga_public.h"

#define CLID_Hidd_Gallium_VMWareSVGA  "hidd.gallium.vmwaresvga"

struct HIDDGalliumVMWareSVGAData
{
    struct svga_winsys_screen wsi;
};

#endif
