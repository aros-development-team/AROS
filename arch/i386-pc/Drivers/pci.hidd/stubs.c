/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stubs for PCI class
    Lang: english
*/

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <aros/config.h>
#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/oop.h>

#include <utility/tagitem.h>

#include <oop/oop.h>
#include <hidd/pcibus.h>

#include "pci.h"

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#undef OOPBase
#define OOPBase (OOP_OOPBASE(obj))

#define STATIC_MID static OOP_MethodID mid

/***************************************************************/

HIDDT_PCI_Device **HIDD_PCI_FindDevice(OOP_Object *obj, struct TagItem *tags)
{
    STATIC_MID;
    struct pHidd_PCI_FindDevice p;
    
    if(!mid) mid = OOP_GetMethodID(IID_Hidd_PCIBus, moHidd_PCI_FindDevice);
        
    p.mID           = mid;
    p.deviceTags    = tags;

    return((HIDDT_PCI_Device **) OOP_DoMethod(obj, (OOP_Msg) &p));
}

/***************************************************************/

VOID HIDD_PCI_FreeQuery(OOP_Object *obj, HIDDT_PCI_Device **devices)
{
    STATIC_MID;
    struct pHidd_PCI_FreeQuery p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIBus, moHidd_PCI_FreeQuery);

    p.mID           = mid;
    p.devices       = devices;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}
