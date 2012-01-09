/*
    Copyright © 2004-2011, The AROS Development Team. All rights reserved.
    $Id: lowlevel.c 34191 2010-08-17 16:19:51Z neil $

    Desc: PCI bus driver for ata.device
    Lang: English
*/

#define DEBUG 0

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <hardware/ahci.h>
#include <hidd/irq.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <string.h>

#include "ahci_intern.h"
#include "ahci.h"
#include "pci.h"

typedef struct 
{
    struct AHCIBase *AHCIBase;
    struct List	    devices;
    OOP_AttrBase    HiddPCIDeviceAttrBase;
    OOP_MethodID    HiddPCIDriverMethodBase;
} EnumeratorArgs;

int ahci_attach(device_t dev)
{
    struct ahci_softc *sc = device_get_softc(dev);

    sc->sc_ad = ahci_lookup_device(dev);
    if (sc->sc_ad == NULL)
        return ENXIO; /* WTF? This matched during the probe... */

    return sc->sc_ad->ad_attach(dev);
}

/*
 * PCI BUS ENUMERATOR
 *   collect all SATA devices and spawn concurrent tasks.
 */

static
AROS_UFH3(void, ahci_PCIEnumerator_h,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(OOP_Object *,     Device, A2),
    AROS_UFHA(APTR,             message,A1))
{
    AROS_USERFUNC_INIT

    device_t dev;
    const struct ahci_device *ad;
    EnumeratorArgs *a = hook->h_Data;
    struct AHCIBase *AHCIBase = a->AHCIBase;

    dev = AllocPooled(AHCIBase->ahci_MemPool, sizeof(*dev) + sizeof(*(dev->dev_softc)));
    if (dev == NULL)
        return;

    dev->dev_AHCIBase = AHCIBase;
    dev->dev_Object   = Device;
    dev->dev_softc    = (void *)&dev[1];

    D(bug("[PCI-AHCI] ahci_PCIEnumerator_h: Scan device %04x:%04x\n", pci_get_vendor(dev), pci_get_device(dev)));

    ad = ahci_lookup_device(dev);
    if (!ad) {
        FreePooled(AHCIBase->ahci_MemPool, dev, sizeof(*dev) + sizeof(*(dev->dev_softc)));
        return;
    }

    D(bug("[PCI-AHCI] ahci_PCIEnumerator_h: Found device %04x:%04x\n", pci_get_vendor(dev), pci_get_device(dev)));

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

static int ahci_pci_scan(struct AHCIBase *AHCIBase)
{
    OOP_Object *pci;
    EnumeratorArgs Args;
    device_t dev;

    D(bug("[PCI-AHCI] ahci_scan: Enumerating devices\n"));

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

    D(bug("[PCI-AHCI] ahci_scan: Registering Probed Hosts..\n"));

    while ((dev = (device_t)RemHead(&Args.devices)) != NULL) {
        if (ahci_attach(dev) != 0) {
            bug("** OH SNAP! ahci_attach() failed!\n");
            FreePooled(AHCIBase->ahci_MemPool, dev, sizeof(*dev) + sizeof(*(dev->dev_softc)));
        }
    }

    return TRUE;
}

/*
 * ahci.device main code has two init routines with 0 and 127 priorities.
 * All bus scanners must run between them.
 */
ADD2INITLIB(ahci_pci_scan, 30)
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
