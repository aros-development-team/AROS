/*
    Copyright © 2011-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _I915_GALLIUM_INTERN_H
#define _I915_GALLIUM_INTERN_H

#include <exec/lists.h>

struct HIDDGalliumData
{
 ULONG dummy;
};

#define CLID_Hidd_Gallium_IntelGMA "hidd.gallium.intelgma"
#define IID_Hidd_Gallium_IntelGMA "hidd.gallium.ntelgma"

VOID init_aros_winsys();
BOOL InitGalliumClass();

#endif
