/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: PCI driver presenting the RP1 xHCI controllers as PCI devices.
*/

/* Bring-up diagnostics, as in the rest of the rp1 modules. */
#define DEBUG 1

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <oop/oop.h>
#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>

#define __NOLIBBASE__
#include <proto/kernel.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

#define PSD(cl)         (&((struct pcirp1base *)(cl)->UserData)->psd)
#define KernelBase      (PSD(cl)->kernelBase)

#undef HiddAttrBase
#define HiddAttrBase    (PSD(cl)->hiddAB)

/*
 * The xHCI blocks are not PCI functions of their own; they sit in RP1's
 * BAR1 window, which rp1.resource has already mapped.  Each is given a
 * synthetic type 0 header at 00:0n.0, so pcixhci finds them like any
 * other controller.  The BAR holds the CPU address.
 */
#define RP1_XHCI_SIZE   0x100000

static void cfg_init(ULONG *cfg, IPTR base, ULONG irq)
{
    cfg[PCICS_VENDOR / 4]    = (RP1_PCIE_DEVICE_ID << 16) | RP1_PCIE_VENDOR_ID;
    cfg[PCICS_REVISION / 4]  = 0x0C033000;              /* USB, xHCI */
    cfg[PCICS_BAR0 / 4]      = ((ULONG)base & ~(RP1_XHCI_SIZE - 1)) | PCIBAR_MEMTYPE_64BIT;
    cfg[PCICS_BAR0 / 4 + 1]  = (ULONG)((UQUAD)base >> 32);
    cfg[PCICS_SUBVENDOR / 4] = cfg[PCICS_VENDOR / 4];
    cfg[PCICS_INT_LINE / 4]  = (1 << 8) | irq;          /* INTA, GIC INTID */
}

static ULONG *cfg_lookup(struct pcirp1_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    if (bus || sub || dev >= RP1_XHCI_COUNT || reg >= sizeof(psd->cfg[0]) || !psd->cfg[dev][0])
        return NULL;

    return &psd->cfg[dev][reg / 4];
}

static ULONG ReadConfigLong(struct pcirp1_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg)
{
    ULONG *p = cfg_lookup(psd, bus, dev, sub, reg);

    return p ? *p : 0xFFFFFFFF;
}

/* Only the enables and the BAR are writable; the BAR answers a size probe. */
static void WriteConfigLong(struct pcirp1_staticdata *psd, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val)
{
    ULONG *p = cfg_lookup(psd, bus, dev, sub, reg);

    if (!p)
        return;

    switch (reg & ~3)
    {
    case PCICS_COMMAND:
        *p = (*p & 0xFFFF0000) | (val & (PCICMF_MEMDECODE | PCICMF_BUSMASTER));
        break;
    case PCICS_BAR0:
        *p = (val & ~(RP1_XHCI_SIZE - 1)) | PCIBAR_MEMTYPE_64BIT;
        break;
    case PCICS_BAR0 + 4:
        *p = val;
        break;
    }
}

OOP_Object *PCIRP1__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct pRoot_New mymsg;

    struct TagItem mytags[] = {
        { aHidd_Name, (IPTR)"PCIRP1" },
        { aHidd_HardwareName, (IPTR)"RP1 southbridge xHCI controllers" },
        { TAG_DONE, 0 }
    };

    mymsg.mID = msg->mID;
    mymsg.attrList = (struct TagItem *)&mytags;

    if (msg->attrList)
    {
        mytags[2].ti_Tag = TAG_MORE;
        mytags[2].ti_Data = (IPTR)msg->attrList;
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
}

UBYTE PCIRP1__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);
    return (UBYTE)(val >> ((msg->reg & 3) * 8));
}

UWORD PCIRP1__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    ULONG val = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);
    return (UWORD)(val >> ((msg->reg & 2) * 8));
}

ULONG PCIRP1__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    return ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg);
}

VOID PCIRP1__Hidd_PCIDriver__WriteConfigByte(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigByte *msg)
{
    ULONG shift = (msg->reg & 3) * 8;
    ULONG val   = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & ~(0xFFUL << shift)) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

VOID PCIRP1__Hidd_PCIDriver__WriteConfigWord(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigWord *msg)
{
    ULONG shift = (msg->reg & 2) * 8;
    ULONG val   = ReadConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3);

    val = (val & ~(0xFFFFUL << shift)) | ((ULONG)msg->val << shift);
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg & ~3, val);
}

VOID PCIRP1__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    WriteConfigLong(PSD(cl), msg->bus, msg->dev, msg->sub, msg->reg, msg->val);
}

/* The BAR is a CPU address inside the window rp1.resource mapped. */
APTR PCIRP1__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_MapPCI *msg)
{
    return msg->PCIAddress;
}

APTR PCIRP1__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    return (APTR)((IPTR)msg->address + PSD(cl)->dma_offset);
}

APTR PCIRP1__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_PCItoCPU *msg)
{
    return (APTR)((IPTR)msg->address - PSD(cl)->dma_offset);
}

/*
 * RP1's DMA is not cache coherent, so memory it shares with the CPU is
 * mapped Normal Non-Cacheable for as long as we hold it.  The attribute
 * belongs to whole pages, hence whole pages are taken.  FreePCIMem() is
 * given only an address, so the raw pointer and the mapped length are
 * kept in the two words ahead of the one returned.
 */
#define PCIMEM_PAGE     4096
#define PCIMEM_ROUND(x) (((x) + PCIMEM_PAGE - 1) & ~(IPTR)(PCIMEM_PAGE - 1))

APTR PCIRP1__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    IPTR mapped = PCIMEM_ROUND(msg->Size);
    APTR raw, addr;

    if (!msg->Size)
        return NULL;

    raw = AllocMem(mapped + PCIMEM_PAGE, MEMF_PUBLIC | MEMF_CLEAR);
    if (!raw)
        return NULL;

    addr = (APTR)PCIMEM_ROUND((IPTR)raw + 2 * sizeof(APTR));
    ((APTR *)addr)[-1] = raw;
    ((APTR *)addr)[-2] = (APTR)mapped;

    /* Flush while still cacheable, or a dirty line lands on top of the
       controller's data later. */
    CacheClearE(addr, mapped, CACRF_ClearD);

    if (!KrnMapGlobal(addr, KrnVirtualToPhysical(addr), mapped,
                      MAP_Readable | MAP_Writable | MAP_WriteThrough))
    {
        bug("[PCIRP1] could not map %u bytes uncached\n", (unsigned)mapped);
        FreeMem(raw, mapped + PCIMEM_PAGE);
        return NULL;
    }

    return addr;
}

VOID PCIRP1__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o, struct pHidd_PCIDriver_FreePCIMem *msg)
{
    APTR addr = msg->Address;
    APTR raw;
    IPTR mapped;

    if (!addr)
        return;

    raw    = ((APTR *)addr)[-1];
    mapped = (IPTR)((APTR *)addr)[-2];

    /* Cacheable again before the pages get a new owner. */
    KrnMapGlobal(addr, KrnVirtualToPhysical(addr), mapped,
                 MAP_Readable | MAP_Writable);

    FreeMem(raw, mapped + PCIMEM_PAGE);
}

static int PCIRP1_Init(LIBBASETYPEPTR LIBBASE)
{
    struct pcirp1_staticdata *psd = &LIBBASE->psd;
    struct RP1Base *rp1 = OpenResource("rp1.resource");
    IPTR  base[RP1_XHCI_COUNT];
    ULONG irq[RP1_XHCI_COUNT];
    OOP_Object *pci;
    int i, found = 0;

    if (!rp1 || !rp1->rp1_Present)
        return TRUE;

    psd->kernelBase = OpenResource("kernel.resource");
    psd->hiddAB     = OOP_ObtainAttrBase(IID_Hidd);
    if (!psd->kernelBase || !psd->hiddAB)
        return FALSE;

    psd->dma_offset = rp1->rp1_DMAOffset;

    base[0] = rp1->rp1_USB0;
    base[1] = rp1->rp1_USB1;
    irq[0]  = rp1->rp1_USBIrq0;
    irq[1]  = rp1->rp1_USBIrq1;

    for (i = 0; i < RP1_XHCI_COUNT; i++)
    {
        /* A live xHCI has CAPLENGTH in 0x10..0x80.  The INTID has to fit
           the interrupt line byte; 0 means MSI never came up. */
        UBYTE caplength = *(volatile UBYTE *)base[i];

        if (caplength < 0x10 || caplength > 0x80 || !irq[i] || irq[i] > 255)
        {
            D(bug("[PCIRP1] xHCI%d at 0x%p skipped: caplength 0x%02x, INTID %u\n",
                  i, (APTR)base[i], caplength, (unsigned)irq[i]));
            continue;
        }

        cfg_init(psd->cfg[i], base[i], irq[i]);
        found++;
        D(bug("[PCIRP1] xHCI%d at 0x%p, INTID %u\n", i, (APTR)base[i], (unsigned)irq[i]));
    }

    if (!found)
        return TRUE;

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (pci)
    {
        HIDD_PCI_AddHardwareDriver(pci, psd->driverClass, NULL);
        OOP_DisposeObject(pci);
    }

    return TRUE;
}

ADD2INITLIB(PCIRP1_Init, 0)
