/*
    Copyright © 2011-2019, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef INTELGMA_GALLIUM_H_
#define INTELGMA_GALLIUM_H_

#include "intelgma_winsys.h"

// The object instance data is used as our winsys wrapper
struct HiddIntelGMAGalliumData
{
    struct i915_winsys gma_winsys;
    OOP_Object *gma_obj;
};

#define CLID_Hidd_Gallium_IntelGMA "hidd.gallium.intelgma"
#define IID_Hidd_Gallium_IntelGMA "hidd.gallium.intelgma"

enum
{
    aoHidd_Gallium_IntelGMA_GfxHidd = 0,

    num_Hidd_Gallium_IntelGMA_Attrs
};

#define aHidd_Gallium_IntelGMA_GfxHidd (HiddBitMapIntelGMAAttrBase + aoHidd_Gallium_IntelGMA_GfxHidd)

#define IS_GALLINTELGMA_ATTR(attr, idx) \
    (((idx) = (attr) - HiddGfxIntelGMAAttrBase) < num_Hidd_Gallium_IntelGMA_Attrs)

BOOL InitGalliumClass();

#endif
