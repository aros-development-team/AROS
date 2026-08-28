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

    APTR KernelBase = OpenResource("kernel.resource");
    psd->kernelBase = (struct Library *)KernelBase;
    if (!KernelBase)
        return FALSE;

    /* Map DBI/Registers (identity map, Device memory) */
    if (!KrnMapGlobal((void *)(IPTR)BCM2712_PCIE0_REG_BASE,
                      (void *)(IPTR)BCM2712_PCIE0_REG_BASE, BCM2712_PCIE0_REG_SIZE,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
    {
        D(bug("[PCIBcm2712] Failed to map PCIe registers\n"));
        return FALSE;
    }
    psd->regs = (volatile uint8_t *)(IPTR)BCM2712_PCIE0_REG_BASE;

    /* Map ECAM Window (Bus 0 & 1) */
    if (KrnMapGlobal((void *)(IPTR)BCM2712_PCIE0_ECAM_BASE,
                     (void *)(IPTR)BCM2712_PCIE0_ECAM_BASE, BCM2712_PCIE0_ECAM_SIZE,
                     MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
        psd->ecam = (volatile uint8_t *)(IPTR)BCM2712_PCIE0_ECAM_BASE;
    else
        psd->ecam = NULL;

    /* Inbound DMA offset */
    psd->dma_offset = 0;
    psd->dma_exp    = BCM2712_DMA_EXP;

    /* Check PHY link and data link status */
    uint32_t status = rd32(psd->regs, PCIE2712_STATUS);
    psd->link_up = (status & (PCIE2712_STATUS_PHYLINKUP | PCIE2712_STATUS_DL_ACTIVE)) != 0;

    D(bug("[PCIBcm2712] Status=0x%08x link_up=%ld\n", (unsigned)status, (long)psd->link_up));

    /* Obtain OOP Attribute Bases */
    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (psd->hiddPCIDriverAB == 0 || psd->hiddPCIDeviceAB == 0 || psd->hiddAB == 0)
    {
        D(bug("[PCIBcm2712] ObtainAttrBases failed\n"));
        return FALSE;
    }

    /* Instantiate Driver Object */
    psd->driverObject = OOP_NewObject(psd->driverClass, NULL, TAG_DONE);
    if (!psd->driverObject)
    {
        D(bug("[PCIBcm2712] Failed to create driver object\n"));
        return FALSE;
    }

    /* Register the driver with pci.hidd */
    {
        struct pHidd_PCI_AddHardwareDriver msg;
        OOP_Object *pci;

        msg.driverClass = psd->driverClass;
        msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);

        pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
        if (pci)
        {
            OOP_DoMethod(pci, (OOP_Msg)&msg);
            OOP_DisposeObject(pci);
            D(bug("[PCIBcm2712] driver registered\n"));
        }
    }

    return TRUE;
}

ADD2INITLIB(PCIBcm2712_Init, 0)
