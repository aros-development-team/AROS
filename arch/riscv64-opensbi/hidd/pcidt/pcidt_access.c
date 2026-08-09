/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Configuration space access for device-tree PCI host bridges.
*/

#include <aros/debug.h>

#include "pcidt.h"

#define DREG(x)

#define PCI_CAP_ID_MSI          0x05
#define PCI_CAP_ID_MSIX         0x11
#define PCI_MSI_ENABLE          0x0001
#define PCI_MSI_MULTI_ENABLE    0x000e
#define PCI_MSIX_ENABLE         0x8000
#define PCI_STATUS_CAPLIST      0x0010
#define PCI_CAP_ID_EXP          0x10
#define PCI_STATUS_IRQ          0x0008
#define PCI_COMMAND_INTX_DISABLE 0x0400

static inline ULONG mmio_read32(IPTR addr)
{
    return *(volatile ULONG *)addr;
}

static inline void mmio_write32(IPTR addr, ULONG val)
{
    *(volatile ULONG *)addr = val;
}

/*
 * DesignWare address translation unit.
 *
 * Config space is not directly addressable on these controllers: a
 * single viewport window is pointed at one device at a time by an
 * outbound iATU region, then read through. Everything below is the
 * "unroll" register layout used by DesignWare 4.80 and later, where
 * each region has its own block of registers at a fixed offset into
 * the DBI space.
 */
#define DWC_ATU_OFFSET          0x300000
/* Direction 0 is outbound (CPU to bus), 1 inbound (bus to memory) */
#define DWC_ATU_REGION(i)       (DWC_ATU_OFFSET + ((i) << 9))
#define DWC_ATU_INREGION(i)     (DWC_ATU_OFFSET + ((i) << 9) + (1 << 8))

#define DWC_ATU_CTRL1           0x00
#define DWC_ATU_CTRL2           0x04
#define DWC_ATU_LOWER_BASE      0x08
#define DWC_ATU_UPPER_BASE      0x0c
#define DWC_ATU_LIMIT           0x10
#define DWC_ATU_LOWER_TARGET    0x14
#define DWC_ATU_UPPER_TARGET    0x18

/* Transaction types the controller can generate */
#define DWC_ATU_TYPE_CFG0       0x4     /* the bus directly below us */
#define DWC_ATU_TYPE_CFG1       0x5     /* anything further down     */

#define DWC_ATU_ENABLE          0x80000000

/* The region we borrow for config accesses */
#define DWC_ATU_CFG_REGION      0

/* And the inbound region used to let devices reach memory */
#define DWC_ATU_DMA_REGION      0
#define DWC_ATU_TYPE_MEM        0x0

/*
 * The older layout. Before the registers were "unrolled" into a block
 * per region, there was one set of them behind a viewport register
 * that selects which region is being programmed. Which layout a
 * controller has cannot be asked directly, so it is worked out by
 * writing a region and reading it back (see dwc_probe_atu).
 */
#define DWC_ATU_VIEWPORT        0x900
#define DWC_ATU_VIEWPORT_INBOUND (1UL << 31)
#define DWC_ATU_VP_CTRL1        0x904
#define DWC_ATU_VP_CTRL2        0x908
#define DWC_ATU_VP_LOWER_BASE   0x90c
#define DWC_ATU_VP_UPPER_BASE   0x910
#define DWC_ATU_VP_LIMIT        0x914
#define DWC_ATU_VP_LOWER_TARGET 0x918
#define DWC_ATU_VP_UPPER_TARGET 0x91c

/*
 * Point the viewport at a region, then use the single register set.
 * Every access has to re-select, so this is not reentrant - all of it
 * happens with the caller holding whatever serialises config access.
 */
static void dwc_vp_select(struct pcidt_bridge *b, unsigned int region,
                          int inbound)
{
    mmio_write32(b->dbiBase + DWC_ATU_VIEWPORT,
                 (inbound ? DWC_ATU_VIEWPORT_INBOUND : 0) | region);
    (void)mmio_read32(b->dbiBase + DWC_ATU_VIEWPORT);
}

/*
 * Which layout this controller has. Programming a target address into
 * the unrolled block and reading it back says whether those registers
 * exist at all: on a viewport controller the space is unimplemented
 * and reads back as zero or all ones.
 */
static void dwc_probe_atu(struct pcidt_bridge *b)
{
    IPTR atu = b->dbiBase + DWC_ATU_REGION(DWC_ATU_CFG_REGION);
    ULONG pattern = 0x12345000, readback;

    mmio_write32(atu + DWC_ATU_LOWER_TARGET, pattern);
    readback = mmio_read32(atu + DWC_ATU_LOWER_TARGET);

    b->atuUnroll = (readback == pattern) ? 1 : 0;

    D(bug("[PCIDT] address translation unit is %s (wrote %08x, read %08x)\n",
        b->atuUnroll ? "unrolled" : "behind a viewport", pattern, readback);)
}

/*
 * Let bus masters reach system memory.
 *
 * A device fetches its command queues and writes completions back by
 * itself, so an inbound translation has to exist before any of that
 * works - without one the transfers are simply dropped and the driver
 * waits for a completion that never comes. The device tree carries no
 * "dma-ranges", which means addresses on the bus are memory addresses,
 * so this is a straight one to one window over RAM.
 */
static void dwc_setup_dma(struct pcidt_bridge *b, IPTR base, IPTR size)
{
    IPTR atu;
    IPTR limit = base + size - 1;

    if (b->atuUnroll)
        atu = b->dbiBase + DWC_ATU_INREGION(DWC_ATU_DMA_REGION);
    else
    {
        dwc_vp_select(b, DWC_ATU_DMA_REGION, 1);
        atu = b->dbiBase + DWC_ATU_VP_CTRL1 - DWC_ATU_CTRL1;
    }

    mmio_write32(atu + DWC_ATU_LOWER_BASE, (ULONG)(base & 0xffffffff));
    mmio_write32(atu + DWC_ATU_UPPER_BASE, (ULONG)(base >> 32));
    mmio_write32(atu + DWC_ATU_LIMIT, (ULONG)(limit & 0xffffffff));
    mmio_write32(atu + DWC_ATU_LOWER_TARGET, (ULONG)(base & 0xffffffff));
    mmio_write32(atu + DWC_ATU_UPPER_TARGET, (ULONG)(base >> 32));
    mmio_write32(atu + DWC_ATU_CTRL1, DWC_ATU_TYPE_MEM);
    mmio_write32(atu + DWC_ATU_CTRL2, DWC_ATU_ENABLE);

    /* Make sure it has landed before anything is told to use it */
    (void)mmio_read32(atu + DWC_ATU_CTRL2);
}

/*
 * Drop the boot firmware's outbound config windows that shadow the
 * memory and I/O windows the device tree describes.
 *
 * Some firmwares hand over with ECAM-style config windows programmed
 * into the outbound regions, parked inside those windows - here they
 * overlay the start of the 64-bit prefetch window, where a
 * framebuffer BAR lives. A memory access that lands in one becomes a
 * config transaction: a wide store is malformed there and the
 * completion never comes, which stalls the hart with no trap.
 *
 * The device tree is the authority on the address map on this path,
 * and config access goes through our own viewport (see dwc_setup), so
 * a CFG-type region contradicting the tree is broken handoff state:
 * disable it. Regions elsewhere, and every memory and I/O region, are
 * left exactly as the firmware set them - on a clean handoff this
 * routine changes nothing. An address matching no region passes
 * through untranslated, which is what the identity ranges describe.
 */
#define DWC_ATU_BOOT_REGIONS    8

static int dwc_overlaps(IPTR rbase, IPTR rend, IPTR wbase, IPTR wsize)
{
    if (!wsize)
        return 0;
    return (rbase <= wbase + wsize - 1) && (rend >= wbase);
}

void PCIDT_DropBootCfgWindows(struct pcidt_bridge *b)
{
    unsigned int i;

    if (b->access != PCIDT_ACCESS_DWC || !b->dbiBase)
        return;

    /* Also settles atuUnroll before the first config access */
    dwc_probe_atu(b);

    for (i = 0; i < DWC_ATU_BOOT_REGIONS; i++)
    {
        IPTR atu, base, end;
        ULONG type, ctrl2;

        if (b->atuUnroll)
            atu = b->dbiBase + DWC_ATU_REGION(i);
        else
        {
            dwc_vp_select(b, i, 0);
            atu = b->dbiBase + DWC_ATU_VP_CTRL1 - DWC_ATU_CTRL1;
        }

        ctrl2 = mmio_read32(atu + DWC_ATU_CTRL2);
        type  = mmio_read32(atu + DWC_ATU_CTRL1) & 0x1f;

        if (!(ctrl2 & DWC_ATU_ENABLE) ||
            (type != DWC_ATU_TYPE_CFG0 && type != DWC_ATU_TYPE_CFG1))
            continue;

        /* The limit register carries the low 32 bits; a config window
           never crosses a 4GiB boundary */
        base = ((IPTR)mmio_read32(atu + DWC_ATU_UPPER_BASE) << 32) |
               mmio_read32(atu + DWC_ATU_LOWER_BASE);
        end  = (base & ~(IPTR)0xffffffff) |
               mmio_read32(atu + DWC_ATU_LIMIT);

        if (!dwc_overlaps(base, end, b->mem64CpuBase, b->mem64Size) &&
            !dwc_overlaps(base, end, b->mem32CpuBase, b->mem32Size) &&
            !dwc_overlaps(base, end, b->ioCpuBase, b->ioSize))
            continue;

        mmio_write32(atu + DWC_ATU_CTRL2, 0);
        (void)mmio_read32(atu + DWC_ATU_CTRL2);
        D(bug("[PCIDT] dbi %p: dropped boot config window %u (%p-%p)\n",
              b->dbiBase, i, base, end);)
    }
}

void PCIDT_EnableDMA(struct pcidt_bridge *b, IPTR base, IPTR size)
{
    IPTR atu;

    if (b->access != PCIDT_ACCESS_DWC || !b->dbiBase)
        return;

    dwc_probe_atu(b);
    dwc_setup_dma(b, base, size);

    /*
     * Read the region back. The address translation unit is only at
     * this offset on controllers using the "unroll" layout; an older
     * one puts a single window behind a viewport register instead, and
     * writes here would go nowhere at all. Config access cannot tell
     * us which it is, because the firmware has already mapped the
     * windows it needs and ours may simply be redundant.
     */
    if (b->atuUnroll)
        atu = b->dbiBase + DWC_ATU_INREGION(DWC_ATU_DMA_REGION);
    else
    {
        dwc_vp_select(b, DWC_ATU_DMA_REGION, 1);
        atu = b->dbiBase + DWC_ATU_VP_CTRL1 - DWC_ATU_CTRL1;
    }
    D(bug("[PCIDT] dbi %p inbound window %p+%p: ctrl2 %08x target %08x%08x\n",
        b->dbiBase, base, size,
        mmio_read32(atu + DWC_ATU_CTRL2),
        mmio_read32(atu + DWC_ATU_UPPER_TARGET),
        mmio_read32(atu + DWC_ATU_LOWER_TARGET));)
}

/*
 * The controller's message interrupt block.
 *
 * A message interrupt is a memory write to an address the controller
 * watches for; it raises one SoC interrupt for all of them and records
 * which vector it was in a status register. That register is written
 * back to acknowledge - until it reads zero the interrupt stays
 * asserted, so anything that leaves a bit set stops the line for every
 * device sharing it.
 */
#define DWC_MSI_ADDR_LO         0x820
#define DWC_MSI_ADDR_HI         0x824
#define DWC_MSI_INTR0_ENABLE    0x828
#define DWC_MSI_INTR0_MASK      0x82c
#define DWC_MSI_INTR0_STATUS    0x830

/*
 * Point the controller at an address and let every vector through.
 * The address is never actually read or written as memory - it only
 * has to be one the controller recognises and nothing else claims.
 */
BOOL PCIDT_MSIInit(struct pcidt_bridge *b, IPTR target)
{
    if (b->access != PCIDT_ACCESS_DWC || !b->dbiBase || !b->msiIrq)
        return FALSE;

    b->msiTarget = target;

    mmio_write32(b->dbiBase + DWC_MSI_ADDR_LO, (ULONG)(target & 0xffffffff));
    mmio_write32(b->dbiBase + DWC_MSI_ADDR_HI, (ULONG)(target >> 32));
    mmio_write32(b->dbiBase + DWC_MSI_INTR0_MASK, 0);
    mmio_write32(b->dbiBase + DWC_MSI_INTR0_ENABLE, ~0);
    mmio_write32(b->dbiBase + DWC_MSI_INTR0_STATUS, ~0);
    (void)mmio_read32(b->dbiBase + DWC_MSI_INTR0_ENABLE);

    b->msiReady = 1;
    b->msiUsed = 0;

    D(bug("[PCIDT:msi] dbi %p target %p irq %u: enable %08x mask %08x\n",
          b->dbiBase, target, b->msiIrq,
          mmio_read32(b->dbiBase + DWC_MSI_INTR0_ENABLE),
          mmio_read32(b->dbiBase + DWC_MSI_INTR0_MASK));)

    return TRUE;
}

/* Acknowledge whatever arrived, and say whether anything had */
ULONG PCIDT_MSIAck(struct pcidt_bridge *b)
{
    ULONG status;

    if (!b->msiReady)
        return 0;

    status = mmio_read32(b->dbiBase + DWC_MSI_INTR0_STATUS);
    if (status)
    {
        mmio_write32(b->dbiBase + DWC_MSI_INTR0_STATUS, status);
        (void)mmio_read32(b->dbiBase + DWC_MSI_INTR0_STATUS);
    }
    return status;
}

/*
 * Give a device a vector, and tell it where to write.
 *
 * The capability holds the address and the value to write; the low
 * bits of that value are the vector, which is the bit the controller
 * will set in its status register.
 */
LONG PCIDT_MSIEnable(struct pcidt_bridge *b, UBYTE bus, UBYTE dev, UBYTE sub)
{
    ULONG cs, cap, head;
    unsigned int guard = 0;
    LONG vector;

    if (!b->msiReady)
        return -1;

    for (vector = 0; vector < 32; vector++)
    {
        if (!(b->msiUsed & (1UL << vector)))
            break;
    }
    if (vector >= 32)
        return -1;

    cs = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);
    if (!((cs >> 16) & PCI_STATUS_CAPLIST))
        return -1;

    cap = PCIDT_ReadConfig(b, bus, dev, sub, 0x34) & 0xfc;

    while (cap >= 0x40 && guard++ < 48)
    {
        head = PCIDT_ReadConfig(b, bus, dev, sub, cap);

        if ((head & 0xff) == PCI_CAP_ID_MSI)
        {
            UWORD ctrl = (UWORD)(head >> 16);
            /* Bit 7 says the address is 64 bits wide, which moves the
               data register along by one long */
            UWORD dataReg = (ctrl & 0x0080) ? (cap + 0x0c) : (cap + 0x08);

            PCIDT_WriteConfig(b, bus, dev, sub, cap + 0x04,
                              (ULONG)(b->msiTarget & 0xffffffff));
            if (ctrl & 0x0080)
                PCIDT_WriteConfig(b, bus, dev, sub, cap + 0x08,
                                  (ULONG)(b->msiTarget >> 32));

            PCIDT_WriteConfig(b, bus, dev, sub, dataReg, (ULONG)vector);

            /* One vector, enabled. The count the device is asking for
               sits in 3:1 and is ours to set; 6:4 is what it can do and
               is read only, so asking for more than that is undefined */
            ctrl &= ~PCI_MSI_MULTI_ENABLE;
            ctrl |= PCI_MSI_ENABLE;
            PCIDT_WriteConfig(b, bus, dev, sub, cap,
                              (head & 0xffff) | ((ULONG)ctrl << 16));

            b->msiUsed |= (1UL << vector);

            D(bug("[PCIDT:msi] %02x:%02x.%x vector %d, ctrl now %04x\n",
                  bus, dev, sub, vector,
                  (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap) >> 16));)

            return vector;
        }

        cap = (head >> 8) & 0xfc;
    }

    return -1;
}

/*
 * Link state, reported in the controller's debug register.
 * Downstream configuration reads only complete once the link is up;
 * issuing them before that wedges the controller.
 */
#define DWC_PORT_DEBUG1         0x72c
#define DWC_PORT_DEBUG1_UP      (1 << 4)
#define DWC_PORT_DEBUG1_TRAINING (1 << 17)

static int dwc_link_up(struct pcidt_bridge *b)
{
    ULONG val = mmio_read32(b->dbiBase + DWC_PORT_DEBUG1);

    return (val & DWC_PORT_DEBUG1_UP) &&
           !(val & DWC_PORT_DEBUG1_TRAINING);
}

/*
 * A configuration address as it appears on the wire: the target bus,
 * device and function, with the register offset supplied separately.
 */
static inline ULONG dwc_target(UBYTE bus, UBYTE dev, UBYTE sub)
{
    return ((ULONG)bus << 24) | ((ULONG)dev << 19) | ((ULONG)sub << 16);
}

/*
 * Point the viewport at one device. Returns the address to access, or
 * 0 if this bridge cannot reach that device.
 */
static IPTR dwc_setup(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                      UBYTE sub, UWORD reg)
{
    IPTR atu;
    ULONG type;

    /*
     * The root complex itself is not behind the viewport - its own
     * configuration registers are the DBI block, read directly.
     */
    if (bus == b->busStart)
    {
        if (dev != 0 || sub != 0)
            return 0;           /* the root port is the only device */
        return b->dbiBase + (reg & 0xffc);
    }

    /*
     * The link from the root port is point to point, so the bus
     * directly below it can only ever hold device 0. Probing the other
     * 31 slots produces unsupported requests, and the controller stops
     * answering rather than returning all-ones for them.
     */
    if (bus == b->busStart + 1 && dev != 0)
        return 0;

    /* Nothing downstream can answer until the link is up */
    if (!dwc_link_up(b))
        return 0;

    /* CFG0 reaches the bus immediately below the root port; anything
       deeper has to be routed by a bridge, which is CFG1 */
    type = (bus == b->busStart + 1) ? DWC_ATU_TYPE_CFG0 : DWC_ATU_TYPE_CFG1;

    if (b->atuUnroll)
        atu = b->dbiBase + DWC_ATU_REGION(DWC_ATU_CFG_REGION);
    else
    {
        dwc_vp_select(b, DWC_ATU_CFG_REGION, 0);
        atu = b->dbiBase + DWC_ATU_VP_CTRL1 - DWC_ATU_CTRL1;
    }

    mmio_write32(atu + DWC_ATU_LOWER_BASE, (ULONG)(b->cfgBase & 0xffffffff));
    mmio_write32(atu + DWC_ATU_UPPER_BASE, (ULONG)(b->cfgBase >> 32));
    mmio_write32(atu + DWC_ATU_LIMIT,
                 (ULONG)((b->cfgBase + b->cfgSize - 1) & 0xffffffff));
    mmio_write32(atu + DWC_ATU_LOWER_TARGET, dwc_target(bus, dev, sub));
    mmio_write32(atu + DWC_ATU_UPPER_TARGET, 0);
    mmio_write32(atu + DWC_ATU_CTRL1, type);
    mmio_write32(atu + DWC_ATU_CTRL2, DWC_ATU_ENABLE);

    /*
     * The write has to have landed before the config access is issued.
     * Reading a register back is the documented way to be sure.
     */
    (void)mmio_read32(atu + DWC_ATU_CTRL2);

    return b->cfgBase + (reg & 0xffc);
}

/*
 * Flat ECAM: one megabyte of config space per bus, so the address is
 * just the bus, device, function and register concatenated.
 */
static IPTR ecam_address(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                         UBYTE sub, UWORD reg)
{
    IPTR off = ((IPTR)(bus - b->busStart) << 20) | ((IPTR)(dev & 31) << 15) |
               ((IPTR)(sub & 7) << 12) | (reg & 0xffc);

    if (off >= b->cfgSize)
        return 0;

    return b->cfgBase + off;
}

static IPTR pcidt_address(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                          UBYTE sub, UWORD reg)
{
    if (bus < b->busStart || bus > b->busEnd)
        return 0;

    if (b->access == PCIDT_ACCESS_DWC)
        return dwc_setup(b, bus, dev, sub, reg);

    return ecam_address(b, bus, dev, sub, reg);
}

ULONG PCIDT_ReadConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                       UBYTE sub, UWORD reg)
{
    IPTR addr = pcidt_address(b, bus, dev, sub, reg);

    /*
     * All ones is what a PCI read returns when nobody answers, so an
     * unreachable address reports "no device" rather than faulting.
     */
    if (!addr)
        return 0xffffffff;

    DREG(bug("[PCIDT] read %02x:%02x.%x reg %03x @ %p\n",
             bus, dev, sub, reg, addr);)

    return mmio_read32(addr);
}

void PCIDT_WriteConfig(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                       UBYTE sub, UWORD reg, ULONG val)
{
    IPTR addr = pcidt_address(b, bus, dev, sub, reg);

    if (!addr)
        return;

    DREG(bug("[PCIDT] write %02x:%02x.%x reg %03x @ %p = %08x\n",
             bus, dev, sub, reg, addr, val);)

    mmio_write32(addr, val);
}

/*
 * Make a device use its legacy interrupt pin.
 *
 * A device with MSI enabled must not assert INTx - the two are
 * mutually exclusive - so whatever firmware left enabled has to go
 * before the pin will fire. A driver that wants messages instead asks
 * for them through ObtainVectors, which turns the pin back off.
 *
 * Walks the capability list looking for MSI (id 5) and MSI-X (id 17);
 * both keep their enable bit in the message control word two bytes
 * into the capability.
 */

void PCIDT_DisableMSI(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                      UBYTE sub)
{
    ULONG val = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);
    UBYTE cap;
    unsigned int guard = 0;

    /* Status is the top half of the command/status long */
    if (!((val >> 16) & PCI_STATUS_CAPLIST))
        return;

    cap = (UBYTE)(PCIDT_ReadConfig(b, bus, dev, sub, 0x34) & 0xfc);

    while (cap >= 0x40 && guard++ < 48)
    {
        ULONG head = PCIDT_ReadConfig(b, bus, dev, sub, cap);
        UBYTE id = (UBYTE)(head & 0xff);
        UBYTE next = (UBYTE)((head >> 8) & 0xfc);
        UWORD ctrl = (UWORD)(head >> 16);

        if (id == PCI_CAP_ID_MSI && (ctrl & PCI_MSI_ENABLE))
        {
            bug("[PCIDT] %02x:%02x.%x had MSI enabled - disabling\n",
                bus, dev, sub);
            PCIDT_WriteConfig(b, bus, dev, sub, cap,
                              (head & 0xffff) |
                              ((ULONG)(ctrl & ~PCI_MSI_ENABLE) << 16));
        }
        else if (id == PCI_CAP_ID_MSIX && (ctrl & PCI_MSIX_ENABLE))
        {
            bug("[PCIDT] %02x:%02x.%x had MSI-X enabled - disabling\n",
                bus, dev, sub);
            PCIDT_WriteConfig(b, bus, dev, sub, cap,
                              (head & 0xffff) |
                              ((ULONG)(ctrl & ~PCI_MSIX_ENABLE) << 16));
        }

        cap = next;
    }
}

/*
 * Let a function drive its interrupt pin, or stop it.
 *
 * A pin is shared by everything below the bridge, so one function with
 * nobody to service it holds the wire down for all of them - and a
 * level triggered source that never goes quiet takes the machine with
 * it. Devices are therefore left masked until a driver claims one.
 */
void PCIDT_SetINTx(struct pcidt_bridge *b, UBYTE bus, UBYTE dev, UBYTE sub,
                   BOOL enable)
{
    ULONG cs = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);
    ULONG cmd = cs & 0xffff;

    if (cs == 0xffffffff)
        return;

    if (enable)
        cmd &= ~PCI_COMMAND_INTX_DISABLE;
    else
        cmd |= PCI_COMMAND_INTX_DISABLE;

    if (cmd != (cs & 0xffff))
        PCIDT_WriteConfig(b, bus, dev, sub, 0x04,
                          (cs & 0xffff0000) | cmd);
}

/*
 * What the inbound translation looks like right now.
 *
 * Read back at the point of use rather than at setup: a window that was
 * programmed correctly and has since been overwritten looks identical
 * from the setup path, and the device just sees its transfers refused.
 */
void PCIDT_DumpInbound(struct pcidt_bridge *b)
{
    IPTR atu;
    unsigned int r;

    if (b->access != PCIDT_ACCESS_DWC || !b->dbiBase)
        return;

    for (r = 0; r < 8; r++)
    {
        if (b->atuUnroll)
            atu = b->dbiBase + DWC_ATU_INREGION(r);
        else
        {
            dwc_vp_select(b, r, 1);
            atu = b->dbiBase + DWC_ATU_VP_CTRL1 - DWC_ATU_CTRL1;
        }

        bug("[PCIDT:atu] dbi %p in[%u] ctrl1 %08x ctrl2 %08x base %08x%08x"
            " limit %08x target %08x%08x\n",
            b->dbiBase, r,
            mmio_read32(atu + DWC_ATU_CTRL1),
            mmio_read32(atu + DWC_ATU_CTRL2),
            mmio_read32(atu + DWC_ATU_UPPER_BASE),
            mmio_read32(atu + DWC_ATU_LOWER_BASE),
            mmio_read32(atu + DWC_ATU_LIMIT),
            mmio_read32(atu + DWC_ATU_UPPER_TARGET),
            mmio_read32(atu + DWC_ATU_LOWER_TARGET));
    }
}

/*
 * Report who on this bridge is currently pulling its interrupt wire.
 *
 * A pin is shared by everything below the bridge, so a source that will
 * not go quiet says nothing about which function is holding it. Status
 * bit 3 does: it is set for exactly as long as that function asserts.
 * The express status registers come along too, since a root port has
 * several latched conditions - error, hot-plug, power management - that
 * assert the wire with nothing on the device side to answer them.
 */
void PCIDT_DumpIntStatus(struct pcidt_bridge *b, UBYTE bus, UBYTE dev,
                         UBYTE sub)
{
    ULONG id = PCIDT_ReadConfig(b, bus, dev, sub, 0x00);
    ULONG cs, cap;
    unsigned int guard = 0;

    if (id == 0xffffffff || id == 0)
        return;

    cs = PCIDT_ReadConfig(b, bus, dev, sub, 0x04);

    bug("[PCIDT:int] %02x:%02x.%x %04x:%04x command %04x status %04x -"
        " asserting %s, INTx %s\n",
        bus, dev, sub, (UWORD)(id & 0xffff), (UWORD)(id >> 16),
        (UWORD)(cs & 0xffff), (UWORD)(cs >> 16),
        ((cs >> 16) & PCI_STATUS_IRQ) ? "YES" : "no",
        (cs & PCI_COMMAND_INTX_DISABLE) ? "disabled" : "enabled");

    /*
     * A bridge records upstream trouble in its secondary status, not in
     * the status above - that one only covers what it sourced itself.
     */
    if (((PCIDT_ReadConfig(b, bus, dev, sub, 0x0c) >> 16) & 0x7f) == 1)
    {
        ULONG sec = PCIDT_ReadConfig(b, bus, dev, sub, 0x1c);
        ULONG ctl = PCIDT_ReadConfig(b, bus, dev, sub, 0x3c);

        bug("[PCIDT:int] %02x:%02x.%x   bridge secstatus %04x brctl %04x -%s%s%s%s\n",
            bus, dev, sub, (UWORD)(sec >> 16), (UWORD)(ctl >> 16),
            ((sec >> 16) & (1 << 13)) ? " master-abort" : "",
            ((sec >> 16) & (1 << 12)) ? " target-abort-rcvd" : "",
            ((sec >> 16) & (1 << 11)) ? " target-abort-sig" : "",
            ((sec >> 16) & (1 << 14)) ? " system-error" : "");
    }

    if (!((cs >> 16) & PCI_STATUS_CAPLIST))
        return;

    cap = PCIDT_ReadConfig(b, bus, dev, sub, 0x34) & 0xfc;

    while (cap >= 0x40 && guard++ < 48)
    {
        ULONG head = PCIDT_ReadConfig(b, bus, dev, sub, cap);

        if ((head & 0xff) == PCI_CAP_ID_EXP)
        {
            bug("[PCIDT:int] %02x:%02x.%x   express devctl %04x devsts %04x,"
                " slotctl %04x slotsts %04x, rootctl %04x rootsts %08x\n",
                bus, dev, sub,
                (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x08) & 0xffff),
                (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x08) >> 16),
                (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x18) & 0xffff),
                (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x18) >> 16),
                (UWORD)(PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x1c) & 0xffff),
                PCIDT_ReadConfig(b, bus, dev, sub, cap + 0x20));
            break;
        }

        cap = (head >> 8) & 0xfc;
    }
}
