/*
    Copyright © 2003-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

#include <exec/execbase.h>
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/libraries.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <exec/semaphores.h>
#include <dos/bptr.h>

#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <aros/debug.h>

#include "pci.h"

#undef HWAttrBase
#undef HWBase
#undef OOPBase
#define HWAttrBase (LIBBASE->psd.hwAttrBase)
#define HWBase     (LIBBASE->psd.hwMethodBase)
#define OOPBase    (LIBBASE->psd.oopBase)

static int PCI_Init(struct pcibase *LIBBASE)
{
    D(bug("[PCI] Initializing PCI system\n"));

    LIBBASE->psd.kernelBase = OpenResource("kernel.resource");
    if (!LIBBASE->psd.kernelBase)
        return FALSE;

    LIBBASE->psd.utilityBase = OpenLibrary("utility.library", 36);
    if (!LIBBASE->psd.utilityBase)
        return FALSE;
        
    LIBBASE->psd.hiddPCIAB = OOP_ObtainAttrBase(IID_Hidd_PCI);
    LIBBASE->psd.hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    LIBBASE->psd.hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    LIBBASE->psd.hwAttrBase = OOP_ObtainAttrBase(IID_HW);
    LIBBASE->psd.hiddPCIDriverMB = OOP_GetMethodID(IID_Hidd_PCIDriver, 0);
    LIBBASE->psd.hwMethodBase = OOP_GetMethodID(IID_HW, 0);

    if (LIBBASE->psd.hiddPCIAB && LIBBASE->psd.hiddPCIDeviceAB &&
        LIBBASE->psd.hiddPCIDriverAB && LIBBASE->psd.hwAttrBase)
    {
        OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);

        InitSemaphore(&LIBBASE->psd.dev_lock);
        NEWLIST(&LIBBASE->psd.devices);

        if (HW_AddDriver(root, LIBBASE->psd.pciClass, NULL))
        {
            D(bug("[PCI] Everything OK\n"));
            return TRUE;
        }
    }

    return FALSE;
}

static int PCI_Expunge(struct pcibase *LIBBASE)
{
    D(bug("[PCI] Base Class destruction\n"));

    /*
      * FIXME: This class can be loaded on hosted systems for debugging
      * purposes. However, expunging it is totally broken and will
      * introduce memory leak.
      * Before expunging we must make sure we have no drivers.
      */
    if (LIBBASE->psd.pciObject)
    {
        IPTR used;
        
        OOP_GetAttr(LIBBASE->psd.pciObject, aHW_InUse, &used);
        if (used)
        {
            D(bug("[PCI] In use, cannot expunge\n"));
            return FALSE;
        }
        else
        {
            OOP_Object *root = OOP_NewObject(NULL, CLID_HW_Root, NULL);
            OOP_MethodID disp_msg = OOP_GetMethodID(IID_Root, moRoot_Dispose);

            HW_RemoveDriver(root, LIBBASE->psd.pciObject);
            /*
             * HW_RemoveDriver() will try to dispose us, but since we are
             * singletone, our Dispose() method does nothing. So, we have to
             * manually dispose ourselves after detaching from the root object.
             */
            OOP_DoSuperMethod(LIBBASE->psd.pciClass, LIBBASE->psd.pciObject, &disp_msg);
        }
    }

    if (LIBBASE->psd.hiddPCIAB)
        OOP_ReleaseAttrBase(IID_Hidd_PCI);
    if (LIBBASE->psd.hiddPCIDeviceAB)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    if (LIBBASE->psd.hiddPCIDriverAB)
        OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    if (LIBBASE->psd.hwAttrBase)
        OOP_ReleaseAttrBase(IID_HW);

    return TRUE;
}

ADD2INITLIB(PCI_Init, 0)
ADD2EXPUNGELIB(PCI_Expunge, 0)
