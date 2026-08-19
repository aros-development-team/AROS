/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 (Raspberry Pi 5) PCIe Host Bridge Bring-Up and Driver Registration.
*/

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/openfirmware.h>
#include <proto/bootloader.h>

#include <aros/bootloader.h>

#define __NOLIBBASE__
#include <proto/kernel.h>

#include <hardware/bcm2708.h>
#include <aros/macros.h>
#include <string.h>

#include "pcie2712.h"
#include <aros/debug.h>

void *OpenFirmwareBase;
APTR  BootLoaderBase;

/* Check if PCIe is enabled in boot arguments (default TRUE) */
static BOOL PCIeEnabled(void)
{
    struct List *list;
    struct Node *node;

    BootLoaderBase = OpenResource("bootloader.resource");
    if (!BootLoaderBase)
        return TRUE;

    list = (struct List *)GetBootInfo(BL_Args);
    if (!list)
        return TRUE;

    ForeachNode(list, node)
    {
        if (strncmp(node->ln_Name, "PCIE=", 5) == 0 &&
            strstr(&node->ln_Name[5], "disable"))
        {
            D(bug("[PCIBcm2712] disabled on command line\n"));
            return FALSE;
        }
    }

    return TRUE;
}

/* Check if running on BCM2712 with PCIe RC0 */
static BOOL PCIeNodeUsable(void)
{
    void *key, *prop;

    OpenFirmwareBase = OpenResource("openfirmware.resource");
    if (!OpenFirmwareBase)
        return TRUE; /* Standalone fallback */

    key = OF_OpenKey("/axi/pcie@1000100000");
    if (!key)
        key = OF_FindNodeByCompatible(NULL, "brcm,bcm2712-pcie");

    if (!key)
    {
        D(bug("[PCIBcm2712] no BCM2712 PCIe node in device tree\n"));
        return FALSE;
    }

    prop = OF_FindProperty(key, "status");
    if (prop)
    {
        const char *val = OF_GetPropValue(prop);
        if (val && strcmp(val, "okay") != 0 && strcmp(val, "ok") != 0)
        {
            D(bug("[PCIBcm2712] node status '%s', leaving bridge alone\n", val));
            return FALSE;
        }
    }

    return TRUE;
}

static inline uint32_t rd32(volatile uint8_t *regs, uint32_t reg)
{
    return *(volatile uint32_t *)(regs + reg);
}

static inline void wr32(volatile uint8_t *regs, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(regs + reg) = val;
}

static int PCIBcm2712_Init(LIBBASETYPEPTR LIBBASE)
{
    struct pci_staticdata *psd = &LIBBASE->psd;

    if (!PCIeEnabled() || !PCIeNodeUsable())
        return TRUE;

    D(bug("[PCIBcm2712] Initializing BCM2712 PCIe host bridge (RPi5 NVMe M.2)\n"));

    psd->kernelBase = (struct Library *)OpenResource("kernel.resource");
    if (!psd->kernelBase)
        return FALSE;

    /* Map DBI/Registers */
    psd->regs = (volatile uint8_t *)KrnMapGlobal(BCM2712_PCIE0_REG_BASE, BCM2712_PCIE0_REG_SIZE, KMAP_IO);
    if (!psd->regs)
    {
        D(bug("[PCIBcm2712] Failed to map PCIe registers\n"));
        return FALSE;
    }

    /* Map ECAM Window (Bus 0 & 1) */
    psd->ecam = (volatile uint8_t *)KrnMapGlobal(BCM2712_PCIE0_ECAM_BASE, BCM2712_PCIE0_ECAM_SIZE, KMAP_IO);

    /* Inbound DMA offset */
    psd->dma_offset = 0;
    psd->dma_exp    = BCM2712_DMA_EXP;

    /* Check PHY link and data link status */
    uint32_t status = rd32(psd->regs, PCIE2712_STATUS);
    psd->link_up = (status & (PCIE2712_STATUS_PHYLINKUP | PCIE2712_STATUS_DL_ACTIVE)) != 0;

    D(bug("[PCIBcm2712] Status=0x%08x link_up=%ld\n", (unsigned)status, (long)psd->link_up));

    /* Obtain OOP Attribute Bases */
    struct TagItem attrTags[] = {
        { aHidd_PCIDriver_DeviceClass, (IPTR)&psd->hiddPCIDriverAB },
        { aHidd_PCIDevice_isBridge,    (IPTR)&psd->hiddPCIDeviceAB },
        { aHidd_Name,                  (IPTR)&psd->hiddAB },
        { TAG_DONE, 0 }
    };
    OOP_ObtainAttrBases(attrTags);

    /* Instantiate Driver Object */
    psd->driverObject = OOP_NewObject(psd->driverClass, NULL, TAG_DONE);
    if (!psd->driverObject)
    {
        D(bug("[PCIBcm2712] Failed to create driver object\n"));
        return FALSE;
    }

    /* Register with PCIBus Class */
    psd->busClass = OOP_FindClass(CLID_Hidd_PCIBus);
    if (psd->busClass)
    {
        struct TagItem busTags[] = {
            { aHidd_PCI_Driver, (IPTR)psd->driverObject },
            { TAG_DONE, 0 }
        };

        psd->busObject = OOP_NewObject(psd->busClass, NULL, (struct TagItem *)&busTags);
        if (psd->busObject)
        {
            D(bug("[PCIBcm2712] Registered PCIe host bridge with pci.hidd bus\n"));
        }
    }

    return TRUE;
}

ADD2INITLIB(PCIBcm2712_Init, 0)
