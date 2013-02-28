/*
    Copyright © 2009-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "drmP.h"
#include "drm_aros.h"

#include <aros/libcall.h>

#include <proto/oop.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

OOP_AttrBase HiddPCIDeviceAttrBase  = 0;
struct Library * OOPBase_DRM        = NULL;
OOP_Object * pciDriver              = NULL;
OOP_Object * pciBus                 = NULL;

AROS_UFH3(void, Enumerator,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, pciDevice, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT

    IPTR ProductID;
    IPTR VendorID;
    IPTR SubSystemProductID;
    IPTR SubSystemVendorID;
    IPTR INTLine;
    struct drm_driver *drv = (struct drm_driver *)hook->h_Data;
    struct drm_pciid *sup = drv->PciIDs;
    
    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID, &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID, &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubsystemID, &SubSystemProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubsystemVendorID, &SubSystemVendorID);

    DRM_DEBUG("VendorID: %x, ProductID: %x\n", VendorID, ProductID);
    
    /* Check interrupt line. If it is not set, just skip the device */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &INTLine);
    if ((INTLine == 0) || (INTLine >= 255))
    {
        DRM_DEBUG("INT line is not set. Skipping device.\n");
        return;
    }
    
    while (sup->VendorID)
    {
        if (sup->VendorID == VendorID && sup->ProductID == ProductID)
        {
            OOP_Object *driver;
            IPTR AGPCap = 0, PCIECap = 0;
            
            struct TagItem attrs[] = {
            { aHidd_PCIDevice_isIO,     FALSE },    /* Don't listen IO transactions */
            { aHidd_PCIDevice_isMEM,    TRUE },     /* Listen to MEM transactions */
            { aHidd_PCIDevice_isMaster, TRUE },     /* Can work in BusMaster */
            { TAG_DONE, 0UL },
            };
            
            DRM_DEBUG("Found!\n");
            /* Filling out device properties */
            drv->VendorID = (UWORD)VendorID;
            drv->ProductID = (UWORD)ProductID;
            drv->SubSystemVendorID = (UWORD)SubSystemVendorID;
            drv->SubSystemProductID = (UWORD)SubSystemProductID;
            drv->pciDevice = pciDevice;

            /*
            Fix PCI device attributes (perhaps already set, but if the 
            NVidia would be the second card in the system, it may stay
            uninitialized.
            */
            OOP_SetAttrs(pciDevice, (struct TagItem*)&attrs);
            
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_Driver, (APTR)&driver);
            pciDriver = driver;

            /* Check AGP/PCIE capabilities */
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_CapabilityAGP, (APTR)&AGPCap);
            OOP_GetAttr(pciDevice, aHidd_PCIDevice_CapabilityPCIE, (APTR)&PCIECap);
            
            drv->IsAGP = (AGPCap != 0);
            drv->IsPCIE = (PCIECap != 0);

            DRM_DEBUG("Acquired pcidriver\n");
            
            return;
        }
        
        sup++;
    }
    
    AROS_USERFUNC_EXIT
}

static void 
drm_aros_pci_find_card(struct drm_driver *drv)
{
    if (pciBus)
    {
        struct Hook FindHook = {
        h_Entry:    (IPTR (*)())Enumerator,
        h_Data:     drv,
        };

        struct TagItem Requirements[] = {
        { tHidd_PCI_Interface,  0x00 },
        { tHidd_PCI_Class,      0x03 },
        { tHidd_PCI_SubClass,   0x00 },
        { tHidd_PCI_VendorID,   drv->VendorID },
        { TAG_DONE,             0UL }
        };
    
        struct pHidd_PCI_EnumDevices enummsg = {
        mID:        OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
        callback:   &FindHook,
        requirements:   (struct TagItem*)&Requirements,
        }, *msg = &enummsg;
        DRM_DEBUG("Calling search Hook\n");
        OOP_DoMethod(pciBus, (OOP_Msg)msg);
    }
}

LONG drm_aros_pci_find_supported_video_card(struct drm_driver *drv)
{
    drv->pciDevice = NULL;
    drv->ProductID = 0x0;
    drv->IsAGP = FALSE;
    drv->IsPCIE = FALSE;
    pciDriver = NULL;
    
    drm_aros_pci_find_card(drv);

    /* If objects are set, detection was successful */
    if (pciBus && drv->pciDevice && pciDriver)
    {
        DRM_INFO("Detected card: 0x%x/0x%x - %s%s%s\n", 
            drv->VendorID, drv->ProductID,
            (!drv->IsAGP) && (!drv->IsPCIE) ? "PCI" : "",
            drv->IsAGP ? "AGP" : "",
            drv->IsPCIE ? "PCIe" : "");
        return 0;
    }
    else
    {
        DRM_INFO("Failed detecting card for VendorID: 0x%x\n", drv->VendorID);
        drm_aros_pci_shutdown(drv);
        return -1;
    }
}

LONG drm_aros_pci_init(struct drm_driver * drv)
{
    if (!OOPBase_DRM)
    {
        if ((OOPBase_DRM = OpenLibrary("oop.library", 0)) == NULL)
        {
            /* Failure */
            return -1;
        }
    }

    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (!pciBus)
    {
        pciBus = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        if (!pciBus)
            return -1;
    }
    
    return 0;
}

VOID drm_aros_pci_shutdown(struct drm_driver *drv)
{
    /* Release AROS-specific PCI objects. Should be called at driver shutdown */
    
    if (drv)
    {
        drv->pciDevice = NULL;
    }

    if (pciBus)
    {
        OOP_DisposeObject(pciBus);
        pciBus = NULL;
    }

    pciDriver = NULL;
    
    if (HiddPCIDeviceAttrBase != 0)
    {
        OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
        HiddPCIDeviceAttrBase = 0;
    }
    
    if (OOPBase_DRM)
    {
        CloseLibrary(OOPBase_DRM);
        OOPBase_DRM = NULL;
    }
}

