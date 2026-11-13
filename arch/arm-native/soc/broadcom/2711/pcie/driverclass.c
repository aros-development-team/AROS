/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2711 PCIe host bridge driver.
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#define __NOLIBBASE__
#include <proto/kernel.h>
/* KrnMapGlobal() and friends resolve through the driver's own copy. */
#define KernelBase      (PSD(cl)->kernelBase)

#include <aros/symbolsets.h>

#include <string.h>

#include <hardware/pci.h>

#include "pcie.h"

#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase   (PSD(cl)->hiddPCIDriverAB)
#define HiddPCIDeviceAttrBase   (PSD(cl)->hiddPCIDeviceAB)
#define HiddAttrBase            (PSD(cl)->hiddAB)

static inline uint32_t rd32(struct pci_staticdata *psd, uint32_t reg)
{
    return *(volatile uint32_t *)(psd->regs + reg);
}

static inline void wr32(struct pci_staticdata *psd, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(psd->regs + reg) = val;
}

OOP_Object *PCIBcm2711__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;

    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCINative" },
        { aHidd_HardwareName, (IPTR)"BCM2711 PCIe host bridge" },
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

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * Devices on this bus get the subclass below, which adds message
 * signalled interrupts; everything else is the base class's answer.
 */
VOID PCIBcm2711__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_PCIDRV_ATTR(msg->attrID, idx) && (idx == aoHidd_PCIDriver_DeviceClass))
    {
        *msg->storage = (IPTR)PSD(cl)->deviceClass;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * Configuration space access. The root complex header is memory mapped
 * at offset 0 of the register block; everything on the external bus is
 * reached through the indexed window. The link is a single point to
 * point lane, so only device 0 exists on the external bus.
 */
static ULONG ReadConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG val = 0xffffffff;

    if (bus == 0)
    {
        if (dev == 0 && sub == 0)
            val = rd32(psd, reg & 0xffc);
    }
    else if (dev == 0)
    {
        Disable();
        wr32(psd, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(bus, dev, sub));
        val = rd32(psd, PCIE_EXT_CFG_DATA + (reg & 0xffc));
        Enable();
    }

    /*
     * INT_LINE is left holding whatever the endpoint powered up with.
     * Interrupts on this bus are message signalled and handed out by
     * ObtainVectors; there is no pin routing to report.
     */
    return val;
}

static void WriteConfigLong(struct pci_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    if (bus == 0)
    {
        if (dev == 0 && sub == 0)
            wr32(psd, reg & 0xffc, val);
    }
    else if (dev == 0)
    {
        /* The endpoint runs no code until its driver enables it; that
           enable is the agreed moment to have its firmware loaded. */
        if ((reg & 0xffc) == PCI_CMD && (val & (PCI_CMD_MEMORY | PCI_CMD_MASTER)))
            EnsureEndpointFirmware(psd);

        Disable();
        wr32(psd, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(bus, dev, sub));
        wr32(psd, PCIE_EXT_CFG_DATA + (reg & 0xffc), val);
        Enable();
    }
}

ULONG PCIBcm2711__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

UWORD PCIBcm2711__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);

    return (val >> ((msg->reg & 2) * 8)) & 0xffff;
}

UBYTE PCIBcm2711__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);

    return (val >> ((msg->reg & 3) * 8)) & 0xff;
}

void PCIBcm2711__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

void PCIBcm2711__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    ULONG shift = (msg->reg & 2) * 8;

    val = (val & ~(0xffffUL << shift)) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, val);
}

void PCIBcm2711__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
    ULONG shift = (msg->reg & 3) * 8;

    val = (val & ~(0xffUL << shift)) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, val);
}

/*
 * A bus master reaches system memory through the inbound window, which
 * the firmware does not place at address zero: what the CPU calls
 * address X, the bus calls X plus this offset. Drivers ask for the
 * translation through these two methods.
 */
APTR PCIBcm2711__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    return (APTR)((uintptr_t)msg->address + PSD(cl)->dma_offset);
}

APTR PCIBcm2711__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_PCItoCPU *msg)
{
    return (APTR)((uintptr_t)msg->address - PSD(cl)->dma_offset);
}

/*
 * BAR addresses live on the PCI bus at BCM2711_PCIE_PCI_WIN; the CPU
 * reaches them through the outbound window. The window is already
 * mapped by the bootstrap.
 */
void *PCIBcm2711__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    uintptr_t addr = (uintptr_t)msg->PCIAddress;

    if (addr >= BCM2711_PCIE_PCI_WIN &&
        addr - BCM2711_PCIE_PCI_WIN < BCM2711_PCIE_WIN_SIZE)
        return (void *)(BCM2711_PCIE_CPU_WIN + (addr - BCM2711_PCIE_PCI_WIN));

    return (void *)addr;
}

/*
 * Descriptor memory a bus master shares with us. The PCIe masters on this
 * SoC are not cache coherent, and a ring the controller writes while the
 * CPU reads it cannot be made to work with cache maintenance alone, so the
 * memory is mapped Normal Non-Cacheable for as long as we hold it.
 *
 * That attribute belongs to whole pages, so the allocation takes whole
 * pages and hands back the head of them: anything sharing the page would
 * otherwise be made uncacheable too, silently and for good.
 *
 * FreePCIMem() is given only an address, so the raw pointer and the mapped
 * length are kept in the two words ahead of the one returned - which is
 * also why the raw allocation carries a page of slack.
 */
#define PCIMEM_PAGE     4096
#define PCIMEM_ROUND(x) (((x) + PCIMEM_PAGE - 1) & ~(uintptr_t)(PCIMEM_PAGE - 1))

APTR PCIBcm2711__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    uintptr_t mapped = PCIMEM_ROUND(msg->Size);
    APTR raw, addr;

    if (!msg->Size)
        return NULL;

    raw = AllocMem(mapped + PCIMEM_PAGE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return NULL;

    addr = (APTR)PCIMEM_ROUND((uintptr_t)raw + 2 * sizeof(APTR));
    ((APTR *)addr)[-1] = raw;
    ((APTR *)addr)[-2] = (APTR)mapped;

    /*
     * Write the pages back before they stop being cacheable: a dirty line
     * left behind would land in memory later, on top of whatever the
     * controller had put there.
     */
    CacheClearE(addr, mapped, CACRF_ClearD);

    if (!KrnMapGlobal(addr, KrnVirtualToPhysical(addr), mapped,
                      MAP_Readable | MAP_Writable | MAP_WriteThrough))
    {
        bug("[PCIBcm2711] could not map %u bytes uncached\n", (unsigned)mapped);
        FreeMem(raw, mapped + PCIMEM_PAGE);
        return NULL;
    }

    D(bug("[PCIBcm2711] AllocPCIMem(%u) = %p (%u bytes uncached)\n",
          (unsigned)msg->Size, addr, (unsigned)mapped));

    return addr;
}

VOID PCIBcm2711__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_FreePCIMem *msg)
{
    APTR addr = msg->Address;
    APTR raw;
    uintptr_t mapped;

    if (!addr)
        return;

    raw    = ((APTR *)addr)[-1];
    mapped = (uintptr_t)((APTR *)addr)[-2];

    /* Cacheable again before it goes back, or the next owner gets
       uncached memory without having asked for it. */
    KrnMapGlobal(addr, KrnVirtualToPhysical(addr), mapped,
                 MAP_Readable | MAP_Writable);

    FreeMem(raw, mapped + PCIMEM_PAGE);
}

/*
 * Message signalled interrupts for a device on this bus.
 *
 * The bridge funnels every vector into one interrupt output and records
 * which vector arrived in INTR2_INT_STATUS, so the "interrupt" a driver
 * is handed is that single output. Drivers sharing it tell themselves
 * apart by looking at their own hardware, exactly as they would on a
 * shared pin.
 */
BOOL PCIBcm2711Dev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ObtainVectors *msg)
{
    struct pci_staticdata *psd = PSD(cl);
    struct PCIBcm2711DevData *data = OOP_INST_DATA(cl, o);
    IPTR bus = 0, dev = 0, sub = 0;
    ULONG cap, guard;
    LONG vector;

    /* No handler on the bridge output means nobody would answer */
    if (!psd->msi_handle)
        return FALSE;

    /* Every vector lands on the same interrupt, so more than one buys a
       caller nothing; nobody has asked for more than one either. */
    if (GetTagData(tHidd_PCIVector_Min, 1, (struct TagItem *)msg->requirements) > 1)
        return FALSE;

    if (data->msiVector)
        return TRUE;    /* already ours */

    OOP_GetAttr(o, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(o, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(o, aHidd_PCIDevice_Sub, &sub);

    /* The capability list only answers once the endpoint runs its firmware */
    EnsureEndpointFirmware(psd);

    if (!((ReadConfigLong(psd, bus, dev, sub, PCI_CMD) >> 16) & PCISTF_CAPABILITIES))
        return FALSE;

    cap = ReadConfigLong(psd, bus, dev, sub, 0x34) & 0xfc;

    for (guard = 12; cap && guard; guard--)
    {
        ULONG head = ReadConfigLong(psd, bus, dev, sub, cap);

        if ((head & 0xff) == PCICAP_MSI)
        {
            UWORD ctrl = (UWORD)(head >> 16);
            ULONG dataReg = (ctrl & PCIMSIF_64BIT) ? (cap + PCIMSI_DATA64)
                                                   : (cap + PCIMSI_DATA32);

            /* The target sits above 4GB, so a function that can only put
               32 bits on the bus cannot reach it. */
            if (!(ctrl & PCIMSIF_64BIT) && ((BCM2711_MSI_TARGET >> 32) != 0))
                return FALSE;

            for (vector = 0; vector < BCM2711_MSI_VECTORS; vector++)
                if (!(psd->msi_used & (1UL << vector)))
                    break;
            if (vector >= BCM2711_MSI_VECTORS)
                return FALSE;

            WriteConfigLong(psd, bus, dev, sub, cap + PCIMSI_ADDRESSLO,
                            (uint32_t)BCM2711_MSI_TARGET);
            if (ctrl & PCIMSIF_64BIT)
                WriteConfigLong(psd, bus, dev, sub, cap + PCIMSI_ADDRESSHI,
                                (uint32_t)(BCM2711_MSI_TARGET >> 32));
            WriteConfigLong(psd, bus, dev, sub, dataReg, BCM2711_MSI_DATA(vector));

            /* One message, enabled. What it may send is ours to set in
               6:4; what it could send in 3:1 is read only. */
            ctrl = (ctrl & ~PCIMSIF_MMEN_MASK) | PCIMSIF_ENABLE;
            WriteConfigLong(psd, bus, dev, sub, cap,
                            (head & 0xffff) | ((ULONG)ctrl << 16));

            psd->msi_used |= (1UL << vector);
            data->msiVector = vector + 1;
            data->msiCap = cap;

            bug("[PCIBcm2711] %02x:%02x.%x signals by message: vector %d -> INTID %u\n",
                (unsigned)bus, (unsigned)dev, (unsigned)sub,
                (int)vector, (unsigned)psd->msi_irq);

            return TRUE;
        }

        cap = (head >> 8) & 0xfc;
    }

    return FALSE;
}

VOID PCIBcm2711Dev__Hidd_PCIDevice__ReleaseVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ReleaseVectors *msg)
{
    struct pci_staticdata *psd = PSD(cl);
    struct PCIBcm2711DevData *data = OOP_INST_DATA(cl, o);
    IPTR bus = 0, dev = 0, sub = 0;
    ULONG head;

    if (!data->msiVector)
        return;

    OOP_GetAttr(o, aHidd_PCIDevice_Bus, &bus);
    OOP_GetAttr(o, aHidd_PCIDevice_Dev, &dev);
    OOP_GetAttr(o, aHidd_PCIDevice_Sub, &sub);

    head = ReadConfigLong(psd, bus, dev, sub, data->msiCap);
    WriteConfigLong(psd, bus, dev, sub, data->msiCap,
                    head & ~((ULONG)PCIMSIF_ENABLE << 16));

    psd->msi_used &= ~(1UL << (data->msiVector - 1));
    data->msiVector = 0;
}

VOID PCIBcm2711Dev__Hidd_PCIDevice__GetVectorAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    struct pci_staticdata *psd = PSD(cl);
    struct PCIBcm2711DevData *data = OOP_INST_DATA(cl, o);
    struct TagItem *tag, *tstate = msg->attribs;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case tHidd_PCIVector_Native:
            tag->ti_Data = data->msiVector ? (data->msiVector - 1) : (IPTR)-1;
            break;

        case tHidd_PCIVector_Int:
            tag->ti_Data = data->msiVector ? psd->msi_irq : (IPTR)-1;
            break;
        }
    }
}
