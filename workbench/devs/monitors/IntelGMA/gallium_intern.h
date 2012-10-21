/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _I915_GALLIUM_INTERN_H
#define _I915_GALLIUM_INTERN_H

#include <exec/lists.h>

struct HIDDGalliumData
{
 ULONG dummy;
};

#define IID_Hidd_i915Gallium "hidd.gallium.i915"

VOID init_aros_winsys();
BOOL InitGalliumClass();

#endif
