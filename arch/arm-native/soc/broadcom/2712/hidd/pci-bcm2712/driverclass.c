/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 PCIe host bridge OOP driver class. One object per bridge.
*/

#define __OOP_NOATTRBASES__

#include <exec/types.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#define __NOLIBBASE__
#include <proto/kernel.h>
#define KernelBase (PSD(cl)->kernelBase)

#include <aros/symbolsets.h>
#include <string.h>
#include <hardware/pci.h>

#include "pcie2712.h"

#include <aros/debug.h>

#undef HiddPCIDriverAttrBase
#undef HiddPCIDeviceAttrBase
#undef HiddAttrBase

#define HiddPCIDriverAttrBase (PSD(cl)->hiddPCIDriverAB)
#define HiddPCIDeviceAttrBase (PSD(cl)->hiddPCIDeviceAB)
#define HiddAttrBase          (PSD(cl)->hiddAB)

static inline uint32_t rd32(volatile uint8_t *regs, uint32_t reg)
{
    return *(volatile uint32_t *)(regs + reg);
}

static inline void wr32(volatile uint8_t *regs, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(regs + reg) = val;
}

/*
 * The bridge comes in as aHidd_DriverData; PCIE_BridgeSetup() either trains it
 * or adopts what the firmware left. One that will not come up leaves no object.
 */
OOP_Object *PCIBcm2712__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    const struct pcie_bridge *bridge =
        (const struct pcie_bridge *)GetTagData(aHidd_DriverData, 0, msg->attrList);
    struct pRoot_New mymsg;
    struct TagItem mytags[] = {
        { aHidd_Name,         (IPTR)"PCINative" },
        { aHidd_HardwareName, (IPTR)(bridge ? bridge->name : "BCM2712 PCIe host bridge") },
        { TAG_DONE, 0 }
    };

    if (!bridge)
        return NULL;

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (o)
    {
        struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);

        data->bridge = bridge;

        if (!PCIE_BridgeSetup(PSD(cl), data))
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);

            OOP_DoSuperMethod(cl, o, &dispose_mid);
            return NULL;
        }
    }

    return o;
}

VOID PCIBcm2712__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
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
 * The root complex's own configuration header is memory mapped at offset 0
 * of the register block; everything on the external bus goes through the
 * indexed window. There is no ECAM on this bridge - the register block is
 * only 0x9310 bytes, ending just past the index register. The link is a
 * single point to point lane, so only device 0 exists on the external bus.
 */
ULONG PCIE_ReadConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG val = 0xffffffff;

    if (bus == 0)
    {
        if (dev == 0 && sub == 0)
            val = rd32(data->regs, reg & 0xffc);
    }
    else if (dev == 0 && data->link_up)
    {
        Disable();
        wr32(data->regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(bus, dev, sub));
        val = rd32(data->regs, PCIE_EXT_CFG_DATA + (reg & 0xffc));
        Enable();

        /*
         * Nothing ever wrote the endpoint's interrupt line field, so report
         * where the bridge actually raises INTx instead.
         */
        if ((reg & 0xffc) == PCI_INTLINE && data->intx_irq && val != 0xffffffff)
            val = (val & ~0xffUL) | data->intx_irq;
    }

    return val;
}

/*
 * An adopted bridge's endpoint is placed by the firmware and not ours to
 * move: on the x4 link RP1's peripheral window holds the debug console.
 * BAR sizing writes all ones and reads back, and for that long the window
 * stops decoding - a bug() in between hits nothing and SErrors. So base
 * registers and the expansion ROM base are read only here. Addresses still
 * read correctly; aHidd_PCIDevice_SizeN does not, and nothing uses it.
 */
static BOOL ConfigWriteAllowed(struct PCIBcm2712Data *data, UWORD reg)
{
    if (data->trained)
        return TRUE;

    reg &= 0xffc;

    if ((reg >= PCI_BAR0) && (reg < PCI_BAR0 + 6 * 4))
        return FALSE;
    if (reg == PCI_ROMBASE)
        return FALSE;

    return TRUE;
}

void PCIE_WriteConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    if (bus == 0)
    {
        if (dev == 0 && sub == 0)
            wr32(data->regs, reg & 0xffc, val);
    }
    else if (dev == 0 && data->link_up)
    {
        if (!ConfigWriteAllowed(data, reg))
        {
            D(bug("[PCIBcm2712] %s: refusing a write to %02x on an adopted bridge\n",
                  data->bridge->node, reg & 0xffc));
            return;
        }

        Disable();
        wr32(data->regs, PCIE_EXT_CFG_INDEX, EXT_CFG_ADDR(bus, dev, sub));
        wr32(data->regs, PCIE_EXT_CFG_DATA + (reg & 0xffc), val);
        Enable();
    }
}

ULONG PCIBcm2712__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return PCIE_ReadConfig(OOP_INST_DATA(cl, o), msg->bus, msg->dev, msg->sub, msg->reg);
}

UWORD PCIBcm2712__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    ULONG val = PCIE_ReadConfig(OOP_INST_DATA(cl, o), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    return (UWORD)(val >> ((msg->reg & 2) * 8));
}

UBYTE PCIBcm2712__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    ULONG val = PCIE_ReadConfig(OOP_INST_DATA(cl, o), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    return (UBYTE)(val >> ((msg->reg & 3) * 8));
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    PCIE_WriteConfig(OOP_INST_DATA(cl, o), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);
    ULONG shift = (msg->reg & 2) * 8;
    ULONG mask  = ~(0xFFFFUL << shift);
    ULONG val   = PCIE_ReadConfig(data, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & mask) | ((ULONG)msg->val << shift);
    PCIE_WriteConfig(data, msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

VOID PCIBcm2712__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);
    ULONG shift = (msg->reg & 3) * 8;
    ULONG mask  = ~(0xFFUL << shift);
    ULONG val   = PCIE_ReadConfig(data, msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & mask) | ((ULONG)msg->val << shift);
    PCIE_WriteConfig(data, msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

/*
 * A BAR holds a PCI bus address; the CPU reaches it through whichever
 * outbound window covers it. Nothing maps these windows for us, so the
 * mapping is made here rather than assumed.
 */
APTR PCIBcm2712__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_MapPCI *msg)
{
    struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);
    uint64_t addr = (uint64_t)(IPTR)msg->PCIAddress;
    uint32_t i;

    for (i = 0; i < data->nranges; i++)
    {
        uint64_t pci_base = data->ranges[i].pci_base;

        if (addr >= pci_base && (addr - pci_base) < data->ranges[i].size)
        {
            IPTR cpu = (IPTR)(addr - pci_base + data->ranges[i].cpu_base);

            if (!KrnMapGlobal((void *)cpu, (void *)cpu, msg->Length,
                              MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
                return NULL;

            return (APTR)cpu;
        }
    }

    D(bug("[PCIBcm2712] MapPCI: %p is outside every outbound window\n", msg->PCIAddress));
    return NULL;
}

/*
 * What a bus master must be given to reach a system address, and back.
 * Each dma-range carries its own offset, so this is a lookup rather than
 * one global displacement: on this bridge the memory window and the MSI
 * doorbell sit at unrelated bus addresses.
 */
IPTR PCIBcm2712__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);
    uint64_t addr = (uint64_t)(IPTR)msg->address;
    uint32_t i;

    for (i = 0; i < data->ndmaranges; i++)
    {
        uint64_t cpu_base = data->dmaranges[i].cpu_base;

        if (addr >= cpu_base && (addr - cpu_base) < data->dmaranges[i].size)
            return (IPTR)(addr - cpu_base + data->dmaranges[i].pci_base);
    }

    D(bug("[PCIBcm2712] CPUtoPCI: %p is outside every inbound window\n", msg->address));
    return (IPTR)msg->address;
}

APTR PCIBcm2712__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDriver_PCItoCPU *msg)
{
    struct PCIBcm2712Data *data = OOP_INST_DATA(cl, o);
    uint64_t addr = (uint64_t)(IPTR)msg->address;
    uint32_t i;

    for (i = 0; i < data->ndmaranges; i++)
    {
        uint64_t pci_base = data->dmaranges[i].pci_base;

        if (addr >= pci_base && (addr - pci_base) < data->dmaranges[i].size)
            return (APTR)(IPTR)(addr - pci_base + data->dmaranges[i].cpu_base);
    }

    return msg->address;
}

/*
 * Descriptor memory a bus master shares with us. The PCIe masters on this
 * SoC are not cache coherent, and a ring the controller writes while the
 * CPU reads it cannot be made to work with cache maintenance alone, so the
 * memory is mapped Normal Non-Cacheable for as long as we hold it. The
 * attribute belongs to whole pages, so whole pages are allocated. FreePCIMem()
 * gets only an address, so the raw pointer and length sit in the two words
 * ahead of the one returned - hence the page of slack.
 */
#define PCIMEM_PAGE     4096
#define PCIMEM_ROUND(x) (((x) + PCIMEM_PAGE - 1) & ~(uintptr_t)(PCIMEM_PAGE - 1))

APTR PCIBcm2712__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
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
        bug("[PCIBcm2712] could not map %u bytes uncached\n", (unsigned)mapped);
        FreeMem(raw, mapped + PCIMEM_PAGE);
        return NULL;
    }

    D(bug("[PCIBcm2712] AllocPCIMem(%u) = %p (%u bytes uncached)\n",
          (unsigned)msg->Size, addr, (unsigned)mapped));

    return addr;
}

VOID PCIBcm2712__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
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
 * Message signalled interrupts are delivered by a separate MIP block on
 * this SoC, which is not driven yet. Until it is, a device asking for
 * vectors is told there are none and falls back to its INTx path.
 */
ULONG PCIBcm2712Dev__Hidd_PCIDevice__ObtainVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ObtainVectors *msg)
{
    return 0;
}

VOID PCIBcm2712Dev__Hidd_PCIDevice__ReleaseVectors(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_ReleaseVectors *msg)
{
}

ULONG PCIBcm2712Dev__Hidd_PCIDevice__GetVectorAttribs(OOP_Class *cl, OOP_Object *o,
    struct pHidd_PCIDevice_GetVectorAttribs *msg)
{
    return 0;
}
