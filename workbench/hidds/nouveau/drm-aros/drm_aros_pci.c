/*
    Copyright (C) 2009-2026, The AROS Development Team. All rights reserved.
*/

#include <aros/libcall.h>
#include <aros/debug.h>

#include <proto/oop.h>

#include <hidd/pci.h>
#include <hidd/hidd.h>

#include <drm-compat/drm_compat_pci.h>
#include <drm-aros/drm_aros_pci.h>

APTR HIDDNouveauAlloc(ULONG size);

OOP_AttrBase HiddPCIDeviceAttrBase  = 0;
OOP_Object *pciDriver               = NULL;
OOP_Object *pciBus                  = NULL;

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
    OOP_Object *driver;
    IPTR AGPCap = 0, PCIECap = 0;
    struct pci_dev **ppdev = (struct pci_dev **)hook->h_Data;
    struct pci_dev *pdev;

    if (*ppdev != NULL)
    {
        D(bug("Device already found\n"));
        return;
    }

    /* Check interrupt line. If it is not set, just skip the device */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_INTLine, &INTLine);
    if ((INTLine == 0) || (INTLine >= 255))
    {
        bug("INT line is not set. Skipping device.\n");
        return;
    }

    /* Get the Device's ProductID */
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_ProductID,           &ProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_VendorID,            &VendorID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubsystemID,         &SubSystemProductID);
    OOP_GetAttr(pciDevice, aHidd_PCIDevice_SubsystemVendorID,   &SubSystemVendorID);

    D(bug("VendorID: %x, ProductID: %x\n", VendorID, ProductID));

    struct TagItem attrs[] =
    {
        { aHidd_PCIDevice_isIO,     FALSE },    /* Don't listen IO transactions */
        { aHidd_PCIDevice_isMEM,    TRUE },     /* Listen to MEM transactions */
        { aHidd_PCIDevice_isMaster, TRUE },     /* Can work in BusMaster */
        { TAG_DONE, 0UL },
    };

    D(bug("Found!\n"));

    pdev = (struct pci_dev *)HIDDNouveauAlloc(sizeof(struct pci_dev));

    /* Filling out device properties */
    pdev->vendor            = (UWORD)VendorID;
    pdev->device            = (UWORD)ProductID;
    pdev->subsystem_vendor  = (UWORD)SubSystemVendorID;
    pdev->subsystem_device  = (UWORD)SubSystemProductID;
    pdev->irq               = (UWORD)INTLine;
    pdev->oopdev            = pciDevice;

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

    pdev->isAGP = (AGPCap != 0);
    pdev->isPCIE = (PCIECap != 0);

    D(bug("Acquired pcidriver\n"));

    (*ppdev) = pdev;

    return;

    AROS_USERFUNC_EXIT
}

static struct pci_dev *drm_aros_pci_find_card()
{
    struct pci_dev *_return = NULL;

    if (pciBus)
    {
        struct Hook FindHook =
        {
            h_Entry:    (IPTR (*)())Enumerator,
            h_Data:     &_return,
        };

        struct TagItem Requirements[] =
        {
            { tHidd_PCI_Interface,  0x00 },
            { tHidd_PCI_Class,      0x03 }, /* Display controller */
            { tHidd_PCI_SubClass,   0x00 },
            { tHidd_PCI_VendorID,   0x10de }, /* Nvidia */
            { TAG_DONE,             0UL }
        };
    
        struct pHidd_PCI_EnumDevices enummsg =
        {
            mID:        OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
            callback:   &FindHook,
            requirements:   (struct TagItem*)&Requirements,
        }, *msg = &enummsg;
        D(bug("Calling search Hook\n"));
        OOP_DoMethod(pciBus, (OOP_Msg)msg);
    }

    return _return;
}

struct pci_dev *drm_aros_pci_find_supported_video_card()
{
    struct pci_dev *pdev = NULL;
    pciDriver = NULL;
    
    pdev = drm_aros_pci_find_card();

    /* If objects are set, detection was successful */
    if (pciBus && pciDriver && pdev)
    {
        D(bug("Detected card: 0x%x/0x%x - %s%s%s\n",
            pdev->vendor, pdev->device,
            (!pdev->isAGP) && (!pdev->isPCIE) ? "PCI" : "",
            pdev->isAGP ? "AGP" : "",
            pdev->isPCIE ? "PCIe" : ""));
        return pdev;
    }
    else
    {
        D(bug("Failed detecting card for VendorID: 0x10de\n"));
        drm_aros_pci_shutdown();
        return NULL;
    }
}

LONG drm_aros_pci_init()
{
    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (!pciBus)
    {
        pciBus = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        if (!pciBus) return -1;
    }
    
    return 0;
}

VOID drm_aros_pci_shutdown()
{
    /* Release AROS-specific PCI objects. Should be called at driver shutdown */

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
}

