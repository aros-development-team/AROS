/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Stub functions for PCI subsystem
    Lang: English
*/

#ifndef AROS_USE_OOP
#   define AROS_USE_OOP
#endif

#include <exec/types.h>
#include <exec/libraries.h>

#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <oop/oop.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

#ifdef AROS_CREATE_ROM
#error	Do not use stubs in ROM code!!!
#else
#define	STATIC_MID  static OOP_MethodID mid
#endif

/***************************************************************************/

VOID HIDD_PCI_AddHardwareDriver(OOP_Object *obj, OOP_Class *driver)
{
    STATIC_MID;
    struct pHidd_PCI_AddHardwareDriver p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

    p.mID = mid;
    p.driverClass = driver;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

BOOL HIDD_PCI_RemHardwareDriver(OOP_Object *obj, OOP_Class *driver)
{
    STATIC_MID;
    struct pHidd_PCI_RemHardwareDriver p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);

    p.mID = mid;
    p.driverClass = driver;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID HIDD_PCI_EnumDevices(OOP_Object *obj, struct Hook *callback, struct TagItem *requirements)
{
    STATIC_MID;
    struct pHidd_PCI_EnumDevices p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices);

    p.mID = mid;
    p.callback = callback;
    p.requirements = requirements;

    if (callback->h_Entry)
        OOP_DoMethod(obj, (OOP_Msg) &p);
}

APTR HIDD_PCIDriver_CPUtoPCI(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_CPUtoPCI p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_CPUtoPCI);

    p.mID = mid;
    p.address = address;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}

APTR HIDD_PCIDriver_PCItoCPU(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_PCItoCPU p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_PCItoCPU);

    p.mID = mid;
    p.address = address;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}

APTR HIDD_PCIDriver_MapPCI(OOP_Object *obj, APTR address, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_MapPCI p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);

    p.mID = mid;
    p.PCIAddress = address;
    p.Length = length;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID HIDD_PCIDriver_UnMapPCI(OOP_Object *obj, APTR address, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_UnMapPCI p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_UnMapPCI);

    p.mID = mid;
    p.CPUAddress = address;
    p.Length = length;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

APTR HIDD_PCIDriver_AllocPCIMem(OOP_Object *obj, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_AllocPCIMem p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_AllocPCIMem);

    p.mID = mid;
    p.Size = length;

    return OOP_DoMethod(obj, (OOP_Msg) &p);
}

VOID HIDD_PCIDriver_FreePCIMem(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_FreePCIMem p;

    if (!mid) mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_FreePCIMem);

    p.mID = mid;
    p.Address = address;

    OOP_DoMethod(obj, (OOP_Msg) &p);
}

