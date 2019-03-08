/*
    Copyright © 2004-2018, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI bus driver for ahci.device
    Lang: English
*/

#include <aros/debug.h>

#include <proto/exec.h>

/* We want all other bases obtained from our base */
#define __NOLIBBASE__

#include <proto/oop.h>

#include <aros/asmcall.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <hardware/ahci.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <string.h>

#include "ahci.h"
#include "pci.h"

typedef struct 
{
    struct AHCIBase *AHCIBase;
    struct List	    devices;
    OOP_AttrBase    HiddPCIDeviceAttrBase;
    OOP_MethodID    HiddPCIDriverMethodBase;
} EnumeratorArgs;

CONST_STRPTR ahciDeviceName = "ahci.device";
CONST_STRPTR ahciControllerName = "PCI AHCI Controller";

int ahci_attach(device_t dev)
{
    struct ahci_softc *sc = device_get_softc(dev);

    if (sc->sc_ad == NULL)
    {
        sc->sc_ad = ahci_lookup_device(dev);
        if (sc->sc_ad == NULL)
                return ENXIO; /* WTF? This matched during the probe... */
    }
    return sc->sc_ad->ad_attach(dev);
}

void ahci_release(device_t dev)
{
    struct AHCIBase *AHCIBase = dev->dev_AHCIBase;
    OOP_MethodID HiddPCIDeviceBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;

    HIDD_PCIDevice_Release(dev->dev_Object);
    FreePooled(AHCIBase->ahci_MemPool, dev, sizeof(*dev) + sizeof(*(dev->dev_softc)));
}    

/*
 * PCI BUS ENUMERATOR
 *   collect all SATA devices and spawn concurrent tasks.
 *
 * ahci.device unit numbers are as follows:
 *   First AHCI device:  unit   0..31
 *   Second device:      units 32..63
 *   etc..
 */

static
AROS_UFH3(void, ahci_PCIEnumerator_h,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    device_t dev;

    EnumeratorArgs *a = hook->h_Data;
    struct AHCIBase *AHCIBase = a->AHCIBase;
    OOP_MethodID HiddPCIDeviceBase = AHCIBase->ahci_HiddPCIDeviceMethodBase;
    CONST_STRPTR owner;

    dev = AllocPooled(AHCIBase->ahci_MemPool, sizeof(*dev) + sizeof(*(dev->dev_softc)));
    if (dev == NULL)
        return;

    dev->dev_AHCIBase = AHCIBase;
    dev->dev_Object   = Device;
    dev->dev_softc    = (void *)&dev[1];
    dev->dev_HostID   = AHCIBase->ahci_HostCount;

    D(bug("[AHCI:PCI] %s: Checking PCI device @ 0x%p\n", __PRETTY_FUNCTION__, Device));

    dev->dev_softc->sc_ad = ahci_lookup_device(dev);
    if (!dev->dev_softc->sc_ad) {
        D(bug("[AHCI:PCI] %s: Device not supported\n", __PRETTY_FUNCTION__));
        FreePooled(AHCIBase->ahci_MemPool, dev, sizeof(*dev) + sizeof(*(dev->dev_softc)));
        return;
    }

    D(bug("[AHCI:PCI] %s: Found '%s'\n", __PRETTY_FUNCTION__, dev->dev_softc->sc_ad->name));

    owner = HIDD_PCIDevice_Obtain(Device, AHCIBase->ahci_Device.dd_Library.lib_Node.ln_Name);
    if (owner)
    {
        D(bug("[AHCI:PCI] Device is already in use by %s\n", __PRETTY_FUNCTION__, owner));
        FreePooled(AHCIBase->ahci_MemPool, dev, sizeof(*dev) + sizeof(*(dev->dev_softc)));
        return;
    }        

    AHCIBase->ahci_HostCount++;
	
    AddTail(&a->devices, (struct Node *)dev);

    return;
    AROS_USERFUNC_EXIT
}

static const struct TagItem Requirements[] =
{
    {tHidd_PCI_Class,     PCI_CLASS_MASSSTORAGE},
    {tHidd_PCI_SubClass,  PCI_SUBCLASS_SATA},
    {tHidd_PCI_Interface, 1},
    {TAG_DONE }
};

static int ahci_bus_Detect(struct AHCIBase *AHCIBase)
{
    OOP_Object *pci;
    EnumeratorArgs Args;
    device_t dev;
    struct TagItem ahci_tags[] =
    {
        {aHidd_Name             , (IPTR)ahciDeviceName          },
        {aHidd_HardwareName     , (IPTR)ahciControllerName      },
        {aHidd_Producer		, 0                             },
#define AHCI_TAG_VEND 2
        {aHidd_Product		, 0                             },
#define AHCI_TAG_PROD 3
        {aHidd_DriverData	, 0                             },
#define AHCI_TAG_DATA 4
        {TAG_DONE               , 0                             }
    };

    D(bug("[AHCI:PCI] %s: Enumerating PCI Devices\n", __PRETTY_FUNCTION__));

    Args.AHCIBase                 = AHCIBase;
    NEWLIST(&Args.devices);

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);

    if (pci)
    {
        struct Hook FindHook =
        {
            h_Entry:    (IPTR (*)())ahci_PCIEnumerator_h,
            h_Data:     &Args
        };

        struct pHidd_PCI_EnumDevices enummsg =
        {
            mID:            OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_EnumDevices),
            callback:       &FindHook,
            requirements:   Requirements,
        };

        OOP_DoMethod(pci, &enummsg.mID);
        OOP_DisposeObject(pci);
    }

    D(bug("[AHCI:PCI] %s: Registering Detected Hosts..\n", __PRETTY_FUNCTION__));
	
    while ((dev = (device_t)RemHead(&Args.devices)) != NULL) {
        if ((ahci_tags[AHCI_TAG_VEND].ti_Data = dev->dev_softc->sc_ad->ad_vendor) == 0)
            ahci_tags[AHCI_TAG_VEND].ti_Data = pci_get_vendor(dev);
        if ((ahci_tags[AHCI_TAG_PROD].ti_Data = dev->dev_softc->sc_ad->ad_product) == 0)
            ahci_tags[AHCI_TAG_PROD].ti_Data = pci_get_device(dev);
        ahci_tags[AHCI_TAG_DATA].ti_Data = (IPTR)dev;
        HW_AddDriver(AHCIBase->storageRoot, AHCIBase->ahciClass, ahci_tags);
        D(bug("[AHCI:PCI] %s: AHCI Controller Object @ 0x%p\n", __PRETTY_FUNCTION__, dev->dev_Controller));
        if (dev->dev_Controller)
        {
            if (ahci_attach(dev) != 0) {
                ahci_release(dev);
                return FALSE;
            }
        }
    }

    return TRUE;
}

/*
 * ahci.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(ahci_bus_Detect, 30)
