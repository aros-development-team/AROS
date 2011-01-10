/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
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
#include <oop/static_mid.h>

#include <hidd/hidd.h>
#include <hidd/pci.h>

#include <proto/oop.h>

#undef OOPBase
#define OOPBase	(OOP_OOPBASE(obj))

/***************************************************************************/

VOID HIDD_PCI_AddHardwareDriver(OOP_Object *obj, OOP_Class *driver)
{
    STATIC_MID;
    struct pHidd_PCI_AddHardwareDriver p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

    p.mID = static_mid;
    p.driverClass = driver;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

BOOL HIDD_PCI_RemHardwareDriver(OOP_Object *obj, OOP_Class *driver)
{
    STATIC_MID;
    struct pHidd_PCI_RemHardwareDriver p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_RemHardwareDriver);

    p.mID = static_mid;
    p.driverClass = driver;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCI_EnumDevices(OOP_Object *obj, struct Hook *callback, struct TagItem *requirements)
{
    STATIC_MID;
    struct pHidd_PCI_EnumDevices p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices);

    p.mID = static_mid;
    p.callback = callback;
    p.requirements = requirements;

    if (callback->h_Entry)
        OOP_DoMethod(obj, (OOP_Msg) msg);
}

APTR HIDD_PCIDriver_CPUtoPCI(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_CPUtoPCI p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_CPUtoPCI);

    p.mID = static_mid;
    p.address = address;

    return (APTR)OOP_DoMethod(obj, (OOP_Msg) msg);
}

APTR HIDD_PCIDriver_PCItoCPU(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_PCItoCPU p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_PCItoCPU);

    p.mID = static_mid;
    p.address = address;

    return (APTR)OOP_DoMethod(obj, (OOP_Msg) msg);
}

APTR HIDD_PCIDriver_MapPCI(OOP_Object *obj, APTR address, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_MapPCI p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_MapPCI);

    p.mID = static_mid;
    p.PCIAddress = address;
    p.Length = length;

    return (APTR)OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCIDriver_UnmapPCI(OOP_Object *obj, APTR address, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_UnmapPCI p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_UnmapPCI);

    p.mID = static_mid;
    p.CPUAddress = address;
    p.Length = length;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

APTR HIDD_PCIDriver_AllocPCIMem(OOP_Object *obj, ULONG length)
{
    STATIC_MID;
    struct pHidd_PCIDriver_AllocPCIMem p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_AllocPCIMem);

    p.mID = static_mid;
    p.Size = length;

    return (APTR)OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCIDriver_FreePCIMem(OOP_Object *obj, APTR address)
{
    STATIC_MID;
    struct pHidd_PCIDriver_FreePCIMem p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDriver, moHidd_PCIDriver_FreePCIMem);

    p.mID = static_mid;
    p.Address = address;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

UBYTE HIDD_PCIDevice_ReadConfigByte(OOP_Object *obj, UBYTE reg)
{
    STATIC_MID;

    struct pHidd_PCIDevice_ReadConfigByte p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigByte);

    p.mID = static_mid;
    p.reg = reg;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

UWORD HIDD_PCIDevice_ReadConfigWord(OOP_Object *obj, UBYTE reg)
{
    STATIC_MID;

    struct pHidd_PCIDevice_ReadConfigWord p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigWord);

    p.mID = static_mid;
    p.reg = reg;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

ULONG HIDD_PCIDevice_ReadConfigLong(OOP_Object *obj, UBYTE reg)
{
    STATIC_MID;

    struct pHidd_PCIDevice_ReadConfigLong p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_ReadConfigLong);

    p.mID = static_mid;
    p.reg = reg;

    return OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCIDevice_WriteConfigByte(OOP_Object *obj, UBYTE reg, UBYTE val)
{
    STATIC_MID;

    struct pHidd_PCIDevice_WriteConfigByte p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigByte);

    p.mID = static_mid;
    p.reg = reg;
    p.val = val;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCIDevice_WriteConfigWord(OOP_Object *obj, UBYTE reg, UWORD val)
{
    STATIC_MID;

    struct pHidd_PCIDevice_WriteConfigWord p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigWord);

    p.mID = static_mid;
    p.reg = reg;
    p.val = val;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}

VOID HIDD_PCIDevice_WriteConfigLong(OOP_Object *obj, UBYTE reg, ULONG val)
{
    STATIC_MID;

    struct pHidd_PCIDevice_WriteConfigLong p, *msg = &p;

    if (!static_mid) static_mid = OOP_GetMethodID(IID_Hidd_PCIDevice, moHidd_PCIDevice_WriteConfigLong);

    p.mID = static_mid;
    p.reg = reg;
    p.val = val;

    OOP_DoMethod(obj, (OOP_Msg) msg);
}
