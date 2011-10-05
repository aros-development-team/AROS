/*
    Copyright 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <proto/oop.h>

#include <aros/symbolsets.h>

#include "pcimock_intern.h"

#undef HiddPCIDriverAttrBase
#define HiddPCIDriverAttrBase   (SD(cl)->hiddPCIDriverAB)

#undef HiddAttrBase
#define HiddAttrBase            (SD(cl)->hiddAB)

#undef HiddPCIMockHardwareAttrBase
#define HiddPCIMockHardwareAttrBase (SD(cl)->hiddPCIMockHardwareAB)

OOP_Object * METHOD(PCIMock, Root, New)
{
    struct pRoot_New mymsg;

    struct TagItem mytags[] = 
    {
        { aHidd_Name, (IPTR)"PCIMock" },
        { aHidd_HardwareName, (IPTR)"Mock PCI Driver" },
        { TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }

    msg = &mymsg;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return o;
}

ULONG METHOD(PCIMock, Hidd_PCIDriver, ReadConfigLong)
{
    IPTR pciconfigspace;
    OOP_Object * mockHardware = NULL;
    ULONG val;
    
    if (!((msg->bus == 0) && (msg->sub == 0)))
        return 0;

    if ((mockHardware = SD(cl)->mockHardwareBus0[msg->dev]) == NULL)
        return 0;

    OOP_GetAttr(mockHardware, aHidd_PCIMockHardware_ConfigSpaceAddr, &pciconfigspace);


    val = *((ULONG *)(pciconfigspace + msg->reg));

    /* Inform mock device that value in its address space region has been read */
    {
        struct pHidd_PCIMockHardware_MemoryReadAtAddress mraa =
        {
            mID : OOP_GetMethodID(IID_Hidd_PCIMockHardware, moHidd_PCIMockHardware_MemoryReadAtAddress),
            memoryaddress : pciconfigspace + msg->reg
        };
        
        OOP_DoMethod(mockHardware, (OOP_Msg)&mraa);
    }

    return val;
}

VOID METHOD(PCIMock, Hidd_PCIDriver, WriteConfigLong)
{
    IPTR pciconfigspace;
    OOP_Object * mockHardware = NULL;
    
    if (!((msg->bus == 0) && (msg->sub == 0)))
        return;

    if ((mockHardware = SD(cl)->mockHardwareBus0[msg->dev]) == NULL)
        return;

    OOP_GetAttr(mockHardware, aHidd_PCIMockHardware_ConfigSpaceAddr, &pciconfigspace);
    
    *((ULONG *)(pciconfigspace + msg->reg)) = msg->val;
    
    /* Inform mock device that value in its address space region has changed */
    {
        struct pHidd_PCIMockHardware_MemoryChangedAtAddress mcaa =
        {
            mID : OOP_GetMethodID(IID_Hidd_PCIMockHardware, moHidd_PCIMockHardware_MemoryChangedAtAddress),
            memoryaddress : pciconfigspace + msg->reg
        };
        
        OOP_DoMethod(mockHardware, (OOP_Msg)&mcaa);
    }
}

static int PCIMock_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    ULONG i;

    D(bug("[PCIMock] deleting classes\n"));

    for (i = 0; i < MAX_BUS0_DEVICES; i++)
        if (LIBBASE->sd.mockHardwareBus0[i] != NULL)
        {
            OOP_DisposeObject(LIBBASE->sd.mockHardwareBus0[i]);
            LIBBASE->sd.mockHardwareBus0[i] = NULL;
        }

    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd);
    OOP_ReleaseAttrBase(IID_Hidd_PCIMockHardware);
    


    return TRUE;
}

static int PCIMock_InitClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_Object *pci = NULL;
    ULONG i;

    D(bug("[PCIMock] Driver initialization\n"));

    LIBBASE->sd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->sd.hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    LIBBASE->sd.hiddPCIMockHardwareAB = OOP_ObtainAttrBase(IID_Hidd_PCIMockHardware);
    for (i = 0; i < MAX_BUS0_DEVICES; i++)
        LIBBASE->sd.mockHardwareBus0[i] = NULL;

    ADD_DEVICE((&LIBBASE->sd), 0, CLID_Hidd_PCIMockHardware_SIS661FX);
    ADD_DEVICE((&LIBBASE->sd), 2, CLID_Hidd_PCIMockHardware_NV44A);
//    ADD_DEVICE((&LIBBASE->sd), 3, CLID_Hidd_PCIMockHardware_NVG86);
//    ADD_DEVICE((&LIBBASE->sd), 4, CLID_Hidd_PCIMockHardware_NVGTS250);
//    ADD_DEVICE((&LIBBASE->sd), 5, CLID_Hidd_PCIMockHardware_NVGF100);

    if (LIBBASE->sd.hiddPCIDriverAB)
    {
        /*
        * The class may be added to the system. Add the driver
        * to PCI subsystem as well
        */
        struct pHidd_PCI_AddHardwareDriver msg;

        /*
        * PCI is suppose to destroy the class on its Dispose
        */
        msg.driverClass = LIBBASE->sd.driverClass;
        msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

        pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        OOP_DoMethod(pci, (OOP_Msg)&msg);
        OOP_DisposeObject(pci);
    }
    else
        return FALSE;

    D(bug("[PCIMock] Driver ClassPtr = %x\n", psd->driverClass));

    return TRUE;
}

ADD2INITLIB(PCIMock_InitClass, 0)
ADD2EXPUNGELIB(PCIMock_ExpungeClass, 0)
