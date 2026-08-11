/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI host bridge driver class, discovered from the device tree.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include <aros/symbolsets.h>
#include <aros/kernel.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/macros.h>
#include <hidd/pci.h>
#include <hardware/pci.h>
#include <utility/tagitem.h>

#include <fdt.h>

#include LC_LIBDEFS_FILE
#include "pcidt.h"

static CONST_STRPTR pcidtHWName = "PCI Express (device tree)";

/*
 * Bridges the device tree may describe. "pci-host-ecam-generic" is the
 * flat window every virtual machine uses; the DesignWare entries cover
 * the real hardware this port targets.
 */
static const struct
{
    CONST_STRPTR      compatible;
    enum pcidt_access access;
} pcidt_known[] =
{
    { "pci-host-ecam-generic",  PCIDT_ACCESS_ECAM },
    { "pci-host-cam-generic",   PCIDT_ACCESS_ECAM },
    { "snps,dw-pcie",           PCIDT_ACCESS_DWC  },
    { "ultrarisc,dw-pcie",      PCIDT_ACCESS_DWC  },
};

/*
 * Map a device's registers so they can be reached. The boot-time page
 * tables cover RAM only, so without this every access faults.
 */
BOOL PCIDT_Map(struct pcidt_staticdata *psd, IPTR base, IPTR size)
{
    return PCIDT_MapAt(psd, base, base, size);
}

BOOL PCIDT_MapAt(struct pcidt_staticdata *psd, IPTR va, IPTR pa, IPTR size)
{
    APTR KernelBase = psd->kernelBase;

    if (va + size > PCIDT_SV39_IDENTITY_LIMIT)
    {
        bug("[PCIDT:Driver] cannot map %p+%p at a non-canonical address\n",
            (APTR)va, (APTR)size);
        return FALSE;
    }

    if (!KrnMapGlobal((APTR)va, (APTR)pa, size,
                      MAP_Readable | MAP_Writable))
    {
        D(bug("[PCIDT:Driver] could not map %p+%p\n", pa, size);)
        return FALSE;
    }
    return TRUE;
}

/*
 * Where the CPU actually reaches a window's contents: identity for
 * canonical windows, the remap window otherwise.
 */
static inline IPTR pcidt_mem64_va(struct pcidt_bridge *b, IPTR cpu)
{
    return b->mem64Va + (cpu - b->mem64CpuBase);
}

/*
 * Map the address windows the bridge forwards onto the bus.
 *
 * A device's BARs are assigned out of these, and drivers use the
 * addresses they read back directly - on a machine where the firmware
 * identity maps everything that just works, but here nothing is
 * reachable until it has been mapped.
 *
 * Each "ranges" entry is a PCI address (3 cells), the CPU address it
 * appears at (2 cells) and a size (2 cells). The top byte of the PCI
 * address says which space it is: 1 is I/O, 2 is 32-bit memory and 3
 * is 64-bit memory.
 */
#define PCI_RANGE_SPACE(hi)     (((hi) >> 24) & 0x03)
#define PCI_RANGE_IO            1
#define PCI_RANGE_MEM32         2
#define PCI_RANGE_MEM64         3

static void pcidt_mapranges(struct pcidt_staticdata *psd, fdt_node_t node,
                            struct pcidt_bridge *b)
{
    ULONG len = 0;
    const ULONG *r = FDT_GetProp(node, "ranges", &len);
    ULONG entries, i;

    if (!r)
        return;

    /* 3 + 2 + 2 cells per entry */
    entries = (len / 4) / 7;

    for (i = 0; i < entries; i++)
    {
        const ULONG *e = &r[i * 7];
        ULONG space = PCI_RANGE_SPACE(FDT_ReadCells(&e[0], 1));
        UQUAD cpu = FDT_ReadCells(&e[3], 2);
        UQUAD size = FDT_ReadCells(&e[5], 2);

        if (!cpu || !size)
            continue;

        /*
         * The 64-bit prefetchable window is tens of gigabytes wide.
         * Mapping it up front would cost more page tables than the
         * pool holds, so remember where it is and map only the
         * regions firmware actually assigned out of it, afterwards.
         */
        if (space == PCI_RANGE_MEM64)
        {
            b->mem64PciBase = (IPTR)FDT_ReadCells(&e[1], 2);
            b->mem64CpuBase = (IPTR)cpu;
            b->mem64Size    = (IPTR)size;
            b->mem64Va      = (IPTR)cpu;
            if ((IPTR)cpu >= PCIDT_SV39_IDENTITY_LIMIT)
                b->mem64Va = PCIDT_HIGH_WINDOW;
            D(bug("[PCIDT:Driver] leaving the 64-bit window at %p (%p) "
                  "unmapped, reachable at %p\n", (IPTR)cpu, (IPTR)size,
                  (APTR)b->mem64Va);)
            continue;
        }

        if (space != PCI_RANGE_IO && space != PCI_RANGE_MEM32)
            continue;

        if (space == PCI_RANGE_IO)
        {
            b->ioPciBase = (IPTR)FDT_ReadCells(&e[1], 2);
            b->ioCpuBase = (IPTR)cpu;
            b->ioSize    = (IPTR)size;
        }
        else
        {
            b->mem32PciBase = (IPTR)FDT_ReadCells(&e[1], 2);
            b->mem32CpuBase = (IPTR)cpu;
            b->mem32Size    = (IPTR)size;
        }

        D(bug("[PCIDT:Driver] mapping %s window %p+%p\n",
              space == PCI_RANGE_IO ? "I/O" : "memory",
              (IPTR)cpu, (IPTR)size);)

        PCIDT_Map(psd, (IPTR)cpu, (IPTR)size);
    }
}

/*
 * Size one base register (or pair) the usual way. Returns the size and
 * leaves the register as it was found.
 */
static UQUAD pcidt_sizebar(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                           UBYTE sub, ULONG reg, BOOL is64)
{
    ULONG lo, hi = 0, szlo, szhi = 0xffffffff;
    UQUAD size;

    lo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
    PCIDT_WriteConfig(b, bus, dev, sub, reg, 0xffffffff);
    szlo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
    PCIDT_WriteConfig(b, bus, dev, sub, reg, lo);

    if (is64)
    {
        hi = PCIDT_ReadConfig(b, bus, dev, sub, reg + 4);
        PCIDT_WriteConfig(b, bus, dev, sub, reg + 4, 0xffffffff);
        szhi = PCIDT_ReadConfig(b, bus, dev, sub, reg + 4);
        PCIDT_WriteConfig(b, bus, dev, sub, reg + 4, hi);
    }

    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
        size = ~(((UQUAD)szhi << 32) | (szlo & ~(UQUAD)0x3)) + 1;
    else
        size = ~(((UQUAD)szhi << 32) | (szlo & ~(UQUAD)0xf)) + 1;

    if (!szlo || szlo == 0xffffffff)
        size = 0;

    return size;
}

/*
 * Give out address space firmware did not. A firmware that only sets
 * up its boot path leaves everything else with empty base registers
 * and decode turned off; nothing behind the bridge is reachable until
 * someone finishes the job.
 */
static void pcidt_assignbars(struct pcidt_staticdata *psd,
                             struct pcidt_bridge *b)
{
    UQUAD memNext, memLimit, ioNext, ioLimit;
    UBYTE bus;

    if (!b->mem32Size)
        return;

    memNext = b->mem32PciBase;
    memLimit = b->mem32PciBase + b->mem32Size;
    /* Keep clear of legacy ports at the bottom of the window */
    ioNext = b->ioPciBase + 0x1000;
    ioLimit = b->ioPciBase + b->ioSize;

    for (bus = b->busStart; ; bus++)
    {
        UBYTE dev;

        for (dev = 0; dev < 32; dev++)
        {
            UBYTE sub, nfunc = 1;

            for (sub = 0; sub < nfunc; sub++)
            {
                ULONG hdr, reg, lastreg;

                if (PCIDT_ReadConfig(b, bus, dev, sub, 0x00) == 0xffffffff)
                    continue;

                hdr = (PCIDT_ReadConfig(b, bus, dev, sub, 0x0c) >> 16) & 0xff;
                if (sub == 0 && (hdr & 0x80))
                    nfunc = 8;
                if ((hdr & 0x7f) != 0)
                    continue;
                lastreg = 0x24;

                /* First move past whatever is already assigned */
                for (reg = 0x10; reg <= lastreg; reg += 4)
                {
                    ULONG lo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
                    BOOL is64 = ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_MMAP) &&
                                ((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT);
                    UQUAD addr, size;

                    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
                    {
                        addr = lo & ~(UQUAD)0x3;
                        if (addr)
                        {
                            size = pcidt_sizebar(b, bus, dev, sub, reg, FALSE);
                            if (addr + size > ioNext &&
                                addr < ioLimit)
                                ioNext = addr + size;
                        }
                        continue;
                    }

                    addr = lo & ~(UQUAD)0xf;
                    if (is64)
                    {
                        addr |= (UQUAD)PCIDT_ReadConfig(b, bus, dev, sub,
                                                        reg + 4) << 32;
                    }
                    if (addr >= b->mem32PciBase && addr < memLimit)
                    {
                        size = pcidt_sizebar(b, bus, dev, sub, reg, is64);
                        if (addr + size > memNext)
                            memNext = addr + size;
                    }
                    if (is64)
                        reg += 4;
                }

                /* Then hand space to whatever has none */
                for (reg = 0x10; reg <= lastreg; reg += 4)
                {
                    ULONG lo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
                    BOOL is64 = ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_MMAP) &&
                                ((lo & PCIBAR_MEMTYPE_MASK) == PCIBAR_MEMTYPE_64BIT);
                    BOOL isio = ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO);
                    UQUAD addr, size;

                    addr = isio ? (lo & ~(UQUAD)0x3) : (lo & ~(UQUAD)0xf);
                    if (is64)
                        addr |= (UQUAD)PCIDT_ReadConfig(b, bus, dev, sub,
                                                        reg + 4) << 32;

                    if (!addr)
                    {
                        size = pcidt_sizebar(b, bus, dev, sub, reg, is64);
                        if (size)
                        {
                            if (isio)
                            {
                                UQUAD a = (ioNext + size - 1) & ~(size - 1);
                                if (a + size <= ioLimit)
                                {
                                    PCIDT_WriteConfig(b, bus, dev, sub, reg,
                                        (ULONG)a | (lo & 0x3));
                                    ioNext = a + size;
                                    D(bug("[PCIDT:Driver] %02x:%02x.%x "
                                          "io bar %02x -> %p (%p)\n", bus,
                                          dev, sub, reg, (IPTR)a, (IPTR)size);)
                                }
                            }
                            else
                            {
                                UQUAD a = (memNext + size - 1) & ~(size - 1);
                                if (a + size <= memLimit)
                                {
                                    PCIDT_WriteConfig(b, bus, dev, sub, reg,
                                        (ULONG)a | (lo & 0xf));
                                    if (is64)
                                        PCIDT_WriteConfig(b, bus, dev, sub,
                                                          reg + 4, 0);
                                    memNext = a + size;
                                    D(bug("[PCIDT:Driver] %02x:%02x.%x "
                                          "mem bar %02x -> %p (%p)\n", bus,
                                          dev, sub, reg, (IPTR)a, (IPTR)size);)
                                }
                            }
                        }
                    }
                    if (is64)
                        reg += 4;
                }

                /* Let the device answer in the space it was given */
                {
                    ULONG cmd = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);
                    PCIDT_WriteConfig(b, bus, dev, sub, 0x04,
                                      (cmd & 0xffff0000) | ((cmd & 0xffff) | 0x0003));
                }
            }
        }

        if (bus == b->busEnd)
            break;
    }
}

/*
 * Map whatever firmware assigned out of the 64-bit window. Walk the
 * devices and map just the regions their base registers point into it,
 * sized the way anything sizes a base register - write ones, read the
 * mask back, restore.
 */
static void pcidt_map64bars(struct pcidt_staticdata *psd,
                            struct pcidt_bridge *b)
{
    UBYTE bus;

    if (!b->mem64Size)
        return;

    for (bus = b->busStart; ; bus++)
    {
        UBYTE dev;

        for (dev = 0; dev < 32; dev++)
        {
            UBYTE sub, nfunc = 1;

            for (sub = 0; sub < nfunc; sub++)
            {
                ULONG hdr, reg, lastreg;

                if (PCIDT_ReadConfig(b, bus, dev, sub, 0x00) == 0xffffffff)
                    continue;

                hdr = (PCIDT_ReadConfig(b, bus, dev, sub, 0x0c) >> 16) & 0xff;
                if (sub == 0 && (hdr & 0x80))
                    nfunc = 8;

                /* Type 0 headers have six base registers, bridges two */
                lastreg = ((hdr & 0x7f) == 0) ? 0x24 : 0x14;

                for (reg = 0x10; reg <= lastreg; reg += 4)
                {
                    ULONG lo, hi, szlo, szhi;
                    UQUAD addr, size;

                    lo = PCIDT_ReadConfig(b, bus, dev, sub, reg);
                    if ((lo & PCIBAR_MASK_TYPE) == PCIBAR_TYPE_IO)
                        continue;
                    if ((lo & PCIBAR_MEMTYPE_MASK) != PCIBAR_MEMTYPE_64BIT)
                        continue;
                    if (reg == lastreg)
                        break;

                    hi = PCIDT_ReadConfig(b, bus, dev, sub, reg + 4);
                    addr = ((UQUAD)hi << 32) | (lo & 0xfffffff0);
                    reg += 4;

                    if (!hi)
                        continue;
                    if (addr <  b->mem64PciBase ||
                        addr >= b->mem64PciBase + b->mem64Size)
                        continue;

                    PCIDT_WriteConfig(b, bus, dev, sub, reg - 4, 0xffffffff);
                    PCIDT_WriteConfig(b, bus, dev, sub, reg,     0xffffffff);
                    szlo = PCIDT_ReadConfig(b, bus, dev, sub, reg - 4);
                    szhi = PCIDT_ReadConfig(b, bus, dev, sub, reg);
                    PCIDT_WriteConfig(b, bus, dev, sub, reg - 4, lo);
                    PCIDT_WriteConfig(b, bus, dev, sub, reg,     hi);

                    size = ~(((UQUAD)szhi << 32) | (szlo & 0xfffffff0)) + 1;
                    if (!size)
                        continue;

                    D(bug("[PCIDT:Driver] %02x:%02x.%x 64-bit region "
                          "%p (%p)\n", bus, dev, sub,
                          (APTR)(IPTR)addr, (APTR)(IPTR)size);)

                    {
                        IPTR cpu = (IPTR)(addr - b->mem64PciBase) +
                                   b->mem64CpuBase;

                        PCIDT_MapAt(psd, pcidt_mem64_va(b, cpu), cpu,
                                    (IPTR)size);
                    }
                }
            }
        }

        if (bus == b->busEnd)
            break;
    }
}

/*
 * Read the interrupt routing out of the tree.
 *
 * Each map entry is a child address (3 cells, as PCI nodes use), the
 * child interrupt (1 cell), the controller's phandle, and the source
 * on that controller (1 cell for the platform controller). Only the
 * parts the mask selects take part in the comparison - on these
 * bridges that is the pin alone, so all four INTx pins of every device
 * land on four fixed sources.
 */
#define PCIDT_INTMAP_CELLS  6

static void pcidt_intmap(fdt_node_t node, struct pcidt_bridge *b)
{
    ULONG len = 0, i, n;
    const ULONG *map = FDT_GetProp(node, "interrupt-map", &len);
    const ULONG *mask = FDT_GetProp(node, "interrupt-map-mask", NULL);

    b->intMapCount = 0;

    if (!map || !mask || (len / 4) % PCIDT_INTMAP_CELLS)
        return;

    for (i = 0; i < 4; i++)
        b->intMask[i] = (ULONG)FDT_ReadCells(&mask[i], 1);

    n = (len / 4) / PCIDT_INTMAP_CELLS;
    if (n > PCIDT_MAX_INTMAP)
        n = PCIDT_MAX_INTMAP;

    for (i = 0; i < n; i++)
    {
        const ULONG *e = &map[i * PCIDT_INTMAP_CELLS];
        ULONG c;

        for (c = 0; c < 4; c++)
            b->intMap[i].key[c] = (ULONG)FDT_ReadCells(&e[c], 1) &
                                  b->intMask[c];
        b->intMap[i].irq = (ULONG)FDT_ReadCells(&e[5], 1);
    }

    b->intMapCount = n;
}

/*
 * Which source a device's interrupt pin comes out on, or 0 if the tree
 * does not say.
 */
ULONG PCIDT_MapInterrupt(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                         UBYTE sub, UBYTE pin)
{
    /* The child address as a PCI node writes it */
    ULONG key[4];
    ULONG i, c;

    if (!b->intMapCount || !pin)
        return 0;

    key[0] = ((ULONG)bus << 16) | ((ULONG)(dev & 31) << 11) |
             ((ULONG)(sub & 7) << 8);
    key[1] = 0;
    key[2] = 0;
    key[3] = pin;

    for (c = 0; c < 4; c++)
        key[c] &= b->intMask[c];

    for (i = 0; i < b->intMapCount; i++)
    {
        for (c = 0; c < 4; c++)
        {
            if (b->intMap[i].key[c] != key[c])
                break;
        }
        if (c == 4)
            return b->intMap[i].irq;
    }

    return 0;
}

/*
 * Acknowledge the controller's message interrupts.
 *
 * Runs ahead of the drivers on the same source, at a higher priority,
 * because the status register has to be cleared for the line to drop -
 * a driver that does not know about the controller cannot do it, and
 * the source would never go quiet.
 */
static AROS_INTH1(pcidt_msiack, struct pcidt_bridge *, b)
{
    AROS_INTFUNC_INIT

    PCIDT_MSIAck(b);

    /* Let the drivers sharing this source look at their hardware */
    return FALSE;

    AROS_INTFUNC_EXIT
}

/*
 * Which source the controller raises for message interrupts.
 *
 * The names run alongside the interrupts, so the position of "msi" in
 * "interrupt-names" is the entry to read. Nothing else identifies it -
 * the numbers themselves are just SoC sources.
 */
static void pcidt_msiirq(fdt_node_t node, struct pcidt_bridge *b)
{
    ULONG len = 0, off = 0, idx = 0;
    CONST_STRPTR names = FDT_GetProp(node, "interrupt-names", &len);
    const ULONG *irqs;
    ULONG irqlen = 0;

    b->msiIrq = 0;

    if (!names || !len)
        return;

    /* The names are one after another, each terminated */
    while (off < len)
    {
        CONST_STRPTR n = &names[off];

        if (n[0] == 'm' && n[1] == 's' && n[2] == 'i' && n[3] == '\0')
            break;

        while (off < len && names[off])
            off++;
        off++;
        idx++;
    }

    if (off >= len)
        return;

    irqs = FDT_GetProp(node, "interrupts", &irqlen);
    if (!irqs || ((idx + 1) * 4) > irqlen)
        return;

    b->msiIrq = (ULONG)FDT_ReadCells(&irqs[idx], 1);

    D(bug("[PCIDT:Driver] message interrupts arrive on source %u\n",
          b->msiIrq);)

}

/* Pick the bus numbers this bridge is responsible for */
static void pcidt_busrange(fdt_node_t node, struct pcidt_bridge *b)
{
    ULONG len = 0;
    const ULONG *br = FDT_GetProp(node, "bus-range", &len);

    b->busStart = 0;
    b->busEnd = 255;

    if (br && len >= 8)
    {
        b->busStart = (UBYTE)FDT_ReadCells(&br[0], 1);
        b->busEnd   = (UBYTE)FDT_ReadCells(&br[1], 1);
    }
}

/*
 * Find every host bridge the firmware described, and record how to
 * reach each one's configuration space.
 */
static ULONG pcidt_discover(struct pcidt_staticdata *psd)
{
    ULONG i;

    for (i = 0; i < sizeof(pcidt_known) / sizeof(pcidt_known[0]); i++)
    {
        fdt_node_t node = FDT_NONE;

        while ((node = FDT_FindCompatible(node, pcidt_known[i].compatible))
               != FDT_NONE)
        {
            struct pcidt_bridge *b;
            UQUAD addr = 0, size = 0;

            if (psd->bridgeCount >= PCIDT_MAX_BRIDGES)
            {
                D(bug("[PCIDT:Driver] more bridges than we can hold\n");)
                return psd->bridgeCount;
            }

            b = &psd->bridges[psd->bridgeCount];
            b->access = pcidt_known[i].access;
            b->index = psd->bridgeCount;

            /*
             * A DesignWare node names its windows: "dbi" is the
             * controller's own registers, "config" the viewport. The
             * generic bindings have a single unnamed window, which is
             * the whole ECAM area.
             */
            if (FDT_GetRegNamed(node, "config", &addr, &size))
            {
                b->cfgBase = (IPTR)addr;
                b->cfgSize = (IPTR)size;
            }
            else if (FDT_GetReg(node, 0, &addr, &size))
            {
                b->cfgBase = (IPTR)addr;
                b->cfgSize = (IPTR)size;
            }
            else
                continue;

            if (FDT_GetRegNamed(node, "dbi", &addr, &size))
            {
                b->dbiBase = (IPTR)addr;
                b->dbiSize = (IPTR)size;
            }
            else
            {
                b->dbiBase = 0;
                b->dbiSize = 0;
            }

            /*
             * A DesignWare controller is useless without its register
             * block - the viewport cannot be aimed without it.
             */
            if (b->access == PCIDT_ACCESS_DWC && !b->dbiBase)
            {
                D(bug("[PCIDT:Driver] %s has no dbi window, skipping\n",
                      FDT_NodeName(node));)
                continue;
            }

            pcidt_busrange(node, b);

            D(bug("[PCIDT:Driver] %s: cfg %p+%p dbi %p buses %d-%d (%s)\n",
                  FDT_NodeName(node), b->cfgBase, b->cfgSize, b->dbiBase,
                  b->busStart, b->busEnd,
                  b->access == PCIDT_ACCESS_DWC ? "DesignWare" : "ECAM");)

            if (!PCIDT_Map(psd, b->cfgBase, b->cfgSize))
                continue;
            if (b->dbiBase && !PCIDT_Map(psd, b->dbiBase, b->dbiSize))
                continue;

            /* The windows devices behind this bridge will answer in */
            pcidt_mapranges(psd, node, b);
            PCIDT_DropBootCfgWindows(b);
            pcidt_assignbars(psd, b);
            pcidt_map64bars(psd, b);

            /* And how their interrupt pins are wired */
            pcidt_intmap(node, b);
            pcidt_msiirq(node, b);

            /*
             * Let devices behind it reach memory. The window covers
             * the low 2GiB of RAM, which is where anything a driver
             * allocates for a device ends up.
             */
            PCIDT_EnableDMA(b, 0x80000000, 0x80000000);

            /*
             * Message interrupts. The address is never used as memory -
             * the controller claims writes to it before they get there -
             * but it has to be one nothing else owns, so a page is
             * allocated and kept for the purpose.
             */
            if (b->msiIrq)
            {
                APTR page = AllocMem(4096, MEMF_PUBLIC | MEMF_CLEAR);

                if (page && PCIDT_MSIInit(b, (IPTR)page))
                {
                    b->msiAck.is_Node.ln_Name = (STRPTR)"pcidt message interrupts";
                    b->msiAck.is_Node.ln_Pri = 100;
                    b->msiAck.is_Node.ln_Type = NT_INTERRUPT;
                    b->msiAck.is_Code = (VOID_FUNC)pcidt_msiack;
                    b->msiAck.is_Data = b;
                    AddIntServer(INTB_KERNEL + b->msiIrq, &b->msiAck);
                }
                else if (page)
                    FreeMem(page, 4096);
            }

            psd->bridgeCount++;
        }
    }

    return psd->bridgeCount;
}

OOP_Object *PCIDT__Root__New(OOP_Class *cl, OOP_Object *o,
                             struct pRoot_New *msg)
{
    struct pcidt_staticdata *psd = PSD(cl);
    OOP_Object *busObj;

    D(bug("[PCIDT:Driver] %s()\n", __func__);)

    busObj = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (busObj)
    {
        struct PCIDTBusData *data = OOP_INST_DATA(cl, busObj);

        /*
         * Take the next bridge in the list discovered at init. See the
         * note in pcidt.h - the driver attributes are all read-only,
         * so there is no way to be handed the bridge directly.
         */
        if (psd->bridgeNext < psd->bridgeCount)
            data->bridge = psd->bridges[psd->bridgeNext++];

        D(bug("[PCIDT:Driver] %s: bus object @ 0x%p, cfg @ %p\n",
              __func__, busObj, data->bridge.cfgBase);)
    }

    return busObj;
}

void PCIDT__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct pcidt_staticdata *psd = PSD(cl);
    ULONG idx;
    BOOL handled = FALSE;

    if (IS_HIDD_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_HardwareName:
            handled = TRUE;
            *msg->storage = (IPTR)pcidtHWName;
            break;
        }
    }
    else if (IS_PCIDRV_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_PCIDriver_DeviceClass:
            handled = TRUE;
            *msg->storage = (IPTR)psd->pcidtDeviceClass;
            break;

        case aoHidd_PCIDriver_DirectBus:
            /* Addresses on the bus are the addresses the CPU uses */
            handled = TRUE;
            *msg->storage = (IPTR)TRUE;
            break;
        }
    }

    if (!handled)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/*
 * Translation between the addresses BARs hold and addresses the CPU
 * can dereference. Identity everywhere except a 64-bit window above
 * the Sv39 identity limit, whose regions are reached through the remap
 * window instead. Already-translated addresses fall outside the
 * window's PCI range and pass through unchanged.
 */
APTR PCIDT__Hidd_PCIDriver__PCItoCPU(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_PCItoCPU *msg)
{
    struct pcidt_bridge *b = &((struct PCIDTBusData *)OOP_INST_DATA(cl, o))->bridge;
    IPTR addr = (IPTR)msg->address;

    if (b->mem64Size && addr >= b->mem64PciBase &&
        addr < b->mem64PciBase + b->mem64Size)
        return (APTR)pcidt_mem64_va(b, addr - b->mem64PciBase + b->mem64CpuBase);

    return (APTR)addr;
}

APTR PCIDT__Hidd_PCIDriver__CPUtoPCI(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_CPUtoPCI *msg)
{
    struct pcidt_bridge *b = &((struct PCIDTBusData *)OOP_INST_DATA(cl, o))->bridge;
    IPTR addr = (IPTR)msg->address;

    if (b->mem64Size && addr >= b->mem64Va &&
        addr < b->mem64Va + b->mem64Size)
        return (APTR)(addr - b->mem64Va + b->mem64PciBase);

    return (APTR)addr;
}

APTR PCIDT__Hidd_PCIDriver__MapPCI(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_MapPCI *msg)
{
    struct pcidt_bridge *b = &((struct PCIDTBusData *)OOP_INST_DATA(cl, o))->bridge;
    IPTR addr = (IPTR)msg->PCIAddress;

    /* The regions were mapped when the bridge came up; hand out
       the address they are reachable at */
    if (b->mem64Size && addr >= b->mem64PciBase &&
        addr < b->mem64PciBase + b->mem64Size)
        return (APTR)pcidt_mem64_va(b, addr - b->mem64PciBase + b->mem64CpuBase);

    return (APTR)addr;
}

ULONG PCIDT__Hidd_PCIDriver__ReadConfigLong(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_ReadConfigLong *msg)
{
    struct PCIDTBusData *data = OOP_INST_DATA(cl, o);

    return PCIDT_ReadConfig(&data->bridge, msg->bus, msg->dev, msg->sub,
                            msg->reg);
}

UWORD PCIDT__Hidd_PCIDriver__ReadConfigWord(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_ReadConfigWord *msg)
{
    struct PCIDTBusData *data = OOP_INST_DATA(cl, o);
    ULONG val = PCIDT_ReadConfig(&data->bridge, msg->bus, msg->dev,
                                 msg->sub, msg->reg & ~3);

    return (UWORD)(val >> ((msg->reg & 2) * 8));
}

UBYTE PCIDT__Hidd_PCIDriver__ReadConfigByte(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_ReadConfigByte *msg)
{
    struct PCIDTBusData *data = OOP_INST_DATA(cl, o);
    ULONG val = PCIDT_ReadConfig(&data->bridge, msg->bus, msg->dev,
                                 msg->sub, msg->reg & ~3);
    UBYTE b = (UBYTE)(val >> ((msg->reg & 3) * 8));

    /*
     * Firmware leaves the interrupt line register alone here - it has
     * no number to put in it, because which source a pin reaches is
     * described by the tree rather than by convention. Answer from the
     * routing map instead, so a driver asking for the line gets
     * something it can hand to KrnAddIRQHandler().
     */
    if (msg->reg == PCICS_INT_LINE && (b == 0 || b == 0xff))
    {
        ULONG pinval = PCIDT_ReadConfig(&data->bridge, msg->bus, msg->dev,
                                        msg->sub, PCICS_INT_PIN & ~3);
        UBYTE pin = (UBYTE)(pinval >> ((PCICS_INT_PIN & 3) * 8));
        ULONG irq = PCIDT_MapInterrupt(&data->bridge, msg->bus, msg->dev,
                                       msg->sub, pin);

        if (irq)
        {
            D(
                bug("[PCIDT:Driver] %02x:%02x.%x pin %d -> source %u\n",
                    msg->bus, msg->dev, msg->sub, pin, irq);
                /*
                 * Everything below the bridge shares this wire, so show
                 * the state of the whole path - the driver about to hook
                 * the source is not necessarily what holds it down.
                 */
                PCIDT_DumpIntStatus(&data->bridge, 0, 0, 0);
                if (msg->bus != 0)
                    PCIDT_DumpIntStatus(&data->bridge, msg->bus, msg->dev,
                                        msg->sub);
                PCIDT_DumpInbound(&data->bridge);
            )

            return (UBYTE)irq;
        }
    }

    return b;
}

/*
 * A driver is taking this device, so it is now safe to let it drive
 * its pin - devices are left masked at enumeration precisely because
 * nothing was listening yet. Unmask before the handler goes on, or an
 * already asserted device would have no way to be noticed.
 */
BOOL PCIDT__Hidd_PCIDriver__AddInterrupt(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_AddInterrupt *msg)
{
    struct PCIDTBusData *data = OOP_INST_DATA(cl, o);
    struct PCIDTDeviceData *ddata =
        OOP_INST_DATA(PSD(cl)->pcidtDeviceClass, msg->device);

    PCIDT_SetINTx(&data->bridge, ddata->bus, ddata->dev, ddata->sub, TRUE);

    return (BOOL)OOP_DoSuperMethod(cl, o, &msg->mID);
}

void PCIDT__Hidd_PCIDriver__WriteConfigLong(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_WriteConfigLong *msg)
{
    struct PCIDTBusData *data = OOP_INST_DATA(cl, o);

    PCIDT_WriteConfig(&data->bridge, msg->bus, msg->dev, msg->sub,
                      msg->reg, msg->val);
}

/* Past this point there is no `cl` to resolve the library bases from */
#undef OOPBase
#undef UtilityBase

/*
 * Memory a bus master can reach.
 *
 * The base class asks for MEMF_31BIT. RAM here starts at 2GB, so only
 * the banks below 4GB that kernel_startup declares with that flag could
 * serve it - a pool better left to the consumers that genuinely need
 * 32-bit addresses (hunk relocation, 32-bit DMA bounce buffers). These
 * controllers are cache coherent and their windows reach well above
 * 4GB, so ordinary memory is what a device should be given.
 *
 * The layout matches the base class: the allocation is padded so the
 * address handed out is page aligned, with the real pointer stored in
 * the word just below it for FreePCIMem() to recover.
 */
APTR PCIDT__Hidd_PCIDriver__AllocPCIMem(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_AllocPCIMem *msg)
{
    APTR memory = AllocVec(msg->Size + 4096 + AROS_ALIGN(sizeof(APTR)),
                           MEMF_CLEAR);
    IPTR diff;

    if (!memory)
        return NULL;

    diff = (IPTR)memory - (AROS_ROUNDUP2((IPTR)memory + sizeof(APTR), 4096));
    *((APTR *)((IPTR)memory - diff - sizeof(APTR))) = memory;

    return (APTR)((IPTR)memory - diff);
}

VOID PCIDT__Hidd_PCIDriver__FreePCIMem(OOP_Class *cl, OOP_Object *o,
        struct pHidd_PCIDriver_FreePCIMem *msg)
{
    if (msg->Address)
        FreeVec(*(APTR *)((IPTR)msg->Address - sizeof(APTR)));
}

static int PCIDT_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct pcidt_staticdata *psd = &LIBBASE->psd;
    struct Library *OOPBase = psd->OOPBase;
    struct Library *UtilityBase = psd->utilityBase;
    APTR KernelBase = psd->kernelBase;
    struct pHidd_PCI_AddHardwareDriver msg, *pmsg = &msg;
    OOP_Object *pci;
    APTR bootinfo;
    APTR dtb;
    ULONG i;

    D(bug("[PCIDT:Driver] %s()\n", __func__);)

    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hidd_PCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);

    if (!psd->hiddAB || !psd->hiddPCIDriverAB || !psd->hidd_PCIDeviceAB)
    {
        bug("[PCIDT:Driver] %s: ObtainAttrBases failed\n", __func__);
        return FALSE;
    }

    /* The firmware's device tree, as kernel.resource recorded it */
    bootinfo = KrnGetBootInfo();
    dtb = (APTR)GetTagData(KRN_FlattenedDeviceTree, 0,
                           (struct TagItem *)bootinfo);
    if (dtb && FDT_Open(dtb))
        pcidt_discover(psd);
    else
        D(bug("[PCIDT:Driver] %s: no device tree\n", __func__);)

    /*
     * A UEFI machine describes its bridges with ACPI instead. The tree
     * comes first and stays authoritative - this only runs when it
     * named nothing, so a board that has one is unaffected.
     */
    if (!psd->bridgeCount)
        PCIDT_DiscoverACPI(psd);

    if (!psd->bridgeCount)
    {
        D(bug("[PCIDT:Driver] %s: no host bridges described\n", __func__);)
        return FALSE;
    }

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci)
        return FALSE;

    msg.mID = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
    msg.driverClass = psd->pcidtDriverClass;
    msg.instanceTags = NULL;

    /* One driver instance per bridge, in the order discovered */
    for (i = 0; i < psd->bridgeCount; i++)
    {
        D(bug("[PCIDT:Driver] %s: registering bridge %u\n", __func__, i);)
        OOP_DoMethod(pci, (OOP_Msg)pmsg);
    }

    OOP_DisposeObject(pci);

    D(bug("[PCIDT:Driver] %s: %u bridge(s) registered\n", __func__,
          psd->bridgeCount);)

    return TRUE;
}

ADD2INITLIB(PCIDT_InitClass, 10)
