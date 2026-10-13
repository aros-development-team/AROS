/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 PCIe host bridge bring-up and driver registration.

    The sequence follows OpenBSD's sys/dev/fdt/bcm2711_pcie.c; see pcie2712.h
    for the licence of that work.
*/

#define __OOP_NOATTRBASES__

#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/pci.h>
#include <oop/oop.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/openfirmware.h>
#include <proto/bootloader.h>

#include <aros/bootloader.h>

#define __NOLIBBASE__
#include <proto/kernel.h>

#include <aros/macros.h>
#include <hardware/pci.h>

#include <string.h>

#include "pcie2712.h"

#include <aros/debug.h>

/* aHidd_* is written in terms of the interface's attribute base, which
   __OOP_NOATTRBASES__ leaves to us; psd is in scope where it is used. */
#undef HiddAttrBase
#define HiddAttrBase (psd->hiddAB)

void *OpenFirmwareBase;
APTR  BootLoaderBase;

/*
 * The bridges this driver takes. pcie0 is not brought out on any board we
 * know of, so it is not listed. The x4 link is adopted rather than trained -
 * resetting it takes away RP1's UART0, which is the debug console.
 * PCIE_BridgeSetup() decides that from the link, not from this table.
 */
static const struct pcie_bridge bcm2712_bridges[] =
{
    { "/axi/pcie@1000110000", 0x1000110000ULL, "BCM2712 PCIe host bridge (x1, M.2)" },
    { "/axi/pcie@1000120000", 0x1000120000ULL, "BCM2712 PCIe host bridge (x4, RP1)" },
};

#define BCM2712_NBRIDGES  (sizeof(bcm2712_bridges) / sizeof(bcm2712_bridges[0]))

static inline uint32_t rd32(volatile uint8_t *regs, uint32_t reg)
{
    return *(volatile uint32_t *)(regs + reg);
}

static inline void wr32(volatile uint8_t *regs, uint32_t reg, uint32_t val)
{
    *(volatile uint32_t *)(regs + reg) = val;
}

/* Busy wait on the generic counter; init runs before timer.device. */
static void delay_us(uint32_t us)
{
    uint64_t freq, now, end;

    asm volatile("mrs %0, CNTFRQ_EL0" : "=r"(freq));
    asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
    end = now + (freq * us) / 1000000;

    do {
        asm volatile("mrs %0, CNTPCT_EL0" : "=r"(now));
    } while (now < end);
}

/* "PCIE=disable" on the kernel command line keeps the bridges untouched. */
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
            D(bug("[PCIBcm2712] disabled on the command line\n"));
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * A bridge's registers raise an external abort while it is held disabled,
 * so the tree is consulted before the first register access. The x1 link is
 * disabled in the base device tree - "dtparam=nvme" in config.txt is what
 * turns it on.
 */
static void *PCIeNode(const struct pcie_bridge *bridge)
{
    void *key, *prop;
    const char *val;

    key = OF_OpenKey((char *)bridge->node);
    if (!key)
    {
        D(bug("[PCIBcm2712] no %s in the device tree\n", bridge->node));
        return NULL;
    }

    /* OF_OpenKey falls back to the last resolved node; make sure this
     * really is a 2712 PCIe controller. */
    prop = OF_FindProperty(key, "compatible");
    if (!prop)
        return NULL;
    val = OF_GetPropValue(prop);
    if (!val || !strstr(val, "2712-pcie"))
    {
        D(bug("[PCIBcm2712] %s is not a PCIe controller (%s)\n",
              bridge->node, val ? val : "?"));
        return NULL;
    }

    prop = OF_FindProperty(key, "status");
    if (prop)
    {
        val = OF_GetPropValue(prop);
        if (val && strcmp(val, "okay") != 0)
        {
            BRINGUP(bug("[PCIBcm2712] %s status '%s' - add \"dtparam=nvme\" to "
                        "config.txt to enable the slot\n", bridge->node, val));
            return NULL;
        }
    }

    return key;
}

static uint32_t PropCells(void *key, const char *name, uint32_t missing)
{
    void *prop = OF_FindProperty(key, (char *)name);

    if (!prop || OF_GetPropLen(prop) < 4)
        return missing;

    return AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop));
}

/*
 * "ranges" and "dma-ranges" share a cell layout: a child address of
 * child_ac cells (the first of which is the PCI flags word), a parent
 * address of parent_ac cells, and a length of child_sc cells. The counts
 * are read rather than assumed - a firmware that changes them would
 * otherwise be silently misparsed.
 */
static uint32_t ParseRanges(void *key, const char *name, struct pcie_range *out)
{
    void *parent, *prop;
    const uint32_t *r;
    uint32_t child_ac, child_sc, parent_ac, entry_cells;
    uint32_t cells, n = 0;

    prop = OF_FindProperty(key, (char *)name);
    if (!prop)
        return 0;

    child_ac = PropCells(key, "#address-cells", 3);
    child_sc = PropCells(key, "#size-cells", 2);

    parent = OF_OpenKey("/axi");
    parent_ac = parent ? PropCells(parent, "#address-cells", 2) : 2;

    entry_cells = child_ac + parent_ac + child_sc;
    if (child_ac < 1 || entry_cells == 0)
        return 0;

    r = (const uint32_t *)OF_GetPropValue(prop);
    cells = OF_GetPropLen(prop) / 4;

    while ((cells >= entry_cells) && (n < PCIE_MAX_RANGES))
    {
        uint64_t pci = 0, cpu = 0, size = 0;
        uint32_t i;

        /* The first child cell is the PCI address space descriptor. */
        out[n].flags = AROS_BE2LONG(*r++);
        for (i = 1; i < child_ac; i++)
            pci = (pci << 32) | AROS_BE2LONG(*r++);
        for (i = 0; i < parent_ac; i++)
            cpu = (cpu << 32) | AROS_BE2LONG(*r++);
        for (i = 0; i < child_sc; i++)
            size = (size << 32) | AROS_BE2LONG(*r++);

        out[n].pci_base = pci;
        out[n].cpu_base = cpu;
        out[n].size = size;

        BRINGUP(bug("[PCIBcm2712] %s[%u]: flags %08x pci %08x%08x <- cpu %08x%08x, %08x%08x bytes\n",
                    name, (unsigned)n, (unsigned)out[n].flags,
                    (unsigned)(pci >> 32), (unsigned)pci,
                    (unsigned)(cpu >> 32), (unsigned)cpu,
                    (unsigned)(size >> 32), (unsigned)size));

        cells -= entry_cells;
        n++;
    }

    return n;
}

/*
 * Which bit of the reset controller holds this bridge. The property is
 * <&rescal, &reset, cell>: rescal takes no cells, the reset controller
 * takes one, so the bridge's bit is the last word.
 */
static uint32_t ResetCellFromDT(void *key)
{
    void *prop = OF_FindProperty(key, "resets");
    uint32_t len;

    if (!prop)
        return ~0U;

    len = OF_GetPropLen(prop);
    if (len < 4)
        return ~0U;

    return AROS_BE2LONG(((const uint32_t *)OF_GetPropValue(prop))[len / 4 - 1]);
}

/*
 * Where the bridge raises the endpoint's legacy interrupt. "interrupt-map"
 * lists, per entry, a child unit address, the INTx pin, the interrupt
 * parent, and that parent's specifier - for the GIC, <type, number, flags>.
 * The link carries a single device, so its INTA is the first entry and the
 * only one that can fire.
 */
static uint32_t IntxIrqFromDT(void *key)
{
    void *prop = OF_FindProperty(key, "interrupt-map");
    uint32_t child_ac, child_ic, entry_cells;
    const uint32_t *m;

    if (!prop)
        return 0;

    child_ac = PropCells(key, "#address-cells", 3);
    child_ic = PropCells(key, "#interrupt-cells", 1);

    /* child address + pin + parent phandle + a three cell GIC specifier */
    entry_cells = child_ac + child_ic + 1 + 3;
    if (OF_GetPropLen(prop) < entry_cells * 4)
        return 0;

    m = (const uint32_t *)OF_GetPropValue(prop);

    /* The specifier's first cell is the interrupt type; only SPIs are routed
       here, and a PPI would mean we have misread the property. */
    if (AROS_BE2LONG(m[child_ac + child_ic + 1]) != 0)
        return 0;

    return GIC_SPI_BASE + AROS_BE2LONG(m[child_ac + child_ic + 2]);
}

static volatile uint8_t *MapDevice(struct pci_staticdata *psd, uint64_t base, uint32_t size)
{
    APTR KernelBase = psd->kernelBase;

    if (!KrnMapGlobal((void *)(IPTR)base, (void *)(IPTR)base, size,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
        return NULL;

    return (volatile uint8_t *)(IPTR)base;
}

/* Rescal is self-deasserting, so only the assert side is implemented. */
static void RescalReset(struct PCIBcm2712Data *data)
{
    int timo;

    if (!data->rescal)
        return;

    wr32(data->rescal, RESCAL_START, rd32(data->rescal, RESCAL_START) | RESCAL_START_BIT);

    for (timo = 10; timo > 0; timo--)
    {
        if (rd32(data->rescal, RESCAL_STATUS) & RESCAL_STATUS_BIT)
            break;
        delay_us(100);
    }

    if (timo == 0)
        bug("[PCIBcm2712] rescal timeout\n");

    wr32(data->rescal, RESCAL_START, rd32(data->rescal, RESCAL_START) & ~RESCAL_START_BIT);
}

static void BridgeReset(struct PCIBcm2712Data *data, BOOL assert)
{
    uint32_t bank, bit;

    if (!data->reset || data->reset_cell == ~0U)
        return;

    bank = data->reset_cell / 32;
    bit  = data->reset_cell % 32;

    if ((bank + 1) * 24 > BCM2712_RESET_SIZE)
        return;

    wr32(data->reset, assert ? RESET_SW_INIT_SET(bank) : RESET_SW_INIT_CLR(bank), 1U << bit);
}

static void Perst(struct PCIBcm2712Data *data, BOOL assert)
{
    uint32_t reg = rd32(data->regs, PCIE_MISC_PCIE_CTRL);

    /* Active low: the bit is the deasserted state. */
    if (assert)
        reg &= ~PCIE_MISC_PCIE_CTRL_PCIE_PERSTB;
    else
        reg |= PCIE_MISC_PCIE_CTRL_PCIE_PERSTB;

    wr32(data->regs, PCIE_MISC_PCIE_CTRL, reg);
}

static BOOL MDIOWrite(struct PCIBcm2712Data *data, uint8_t port, uint16_t addr, uint32_t val)
{
    int timo;

    wr32(data->regs, PCIE_RC_DL_MDIO_ADDR,
         PCIE_RC_DL_MDIO_CMD_WRITE | ((uint32_t)port << PCIE_RC_DL_MDIO_PORT_SHIFT) | addr);
    rd32(data->regs, PCIE_RC_DL_MDIO_ADDR);

    wr32(data->regs, PCIE_RC_DL_MDIO_WR_DATA, val | PCIE_RC_DL_MDIO_DATA_DONE);
    for (timo = 10; timo > 0; timo--)
    {
        if (!(rd32(data->regs, PCIE_RC_DL_MDIO_WR_DATA) & PCIE_RC_DL_MDIO_DATA_DONE))
            break;
        delay_us(10);
    }

    if (timo == 0)
    {
        bug("[PCIBcm2712] MDIO write timeout (reg %04x)\n", addr);
        return FALSE;
    }

    return TRUE;
}

/*
 * Pick the 54MHz reference clock and set the L1 sub-state timers to match
 * it. Without this the SerDes has no usable clock and the link never
 * trains. The register/value pairs are the ones OpenBSD programs; they are
 * not documented anywhere we can cite.
 */
static BOOL SetupRefClk(struct PCIBcm2712Data *data)
{
    static const struct { uint16_t addr; uint16_t data; } regs[] = {
        { 0x16, 0x50b9 }, { 0x17, 0xbda1 }, { 0x18, 0x0094 }, { 0x19, 0x97b4 },
        { 0x1b, 0x5030 }, { 0x1c, 0x5030 }, { 0x1e, 0x0007 },
    };
    uint32_t reg;
    unsigned int i;

    /* Make failed reads return 0xffffffff rather than 0xdeaddead, or
       probing an absent function looks like a device that answered. */
    wr32(data->regs, PCIE_MISC_AXI_READ_ERROR_DATA, 0xffffffff);

    if (!MDIOWrite(data, 0, 0x1f, 0x1600))
        return FALSE;
    for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
    {
        if (!MDIOWrite(data, 0, regs[i].addr, regs[i].data))
            return FALSE;
    }

    delay_us(100);

    reg = rd32(data->regs, PCIE_RC_PL_PHY_CTL_15);
    reg &= ~PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD_MASK;
    reg |= 18;
    wr32(data->regs, PCIE_RC_PL_PHY_CTL_15, reg);

    return TRUE;
}

/*
 * CLKREQ handling. The x1 link asks for "safe" mode, which is simply all
 * four bits clear; the other modes exist for boards that wire CLKREQ# up.
 */
static void SetupClkreq(struct PCIBcm2712Data *data, void *key)
{
    uint32_t reg = rd32(data->regs, PCIE_HARD_DEBUG) & ~PCIE_HARD_DEBUG_CLKREQ_MASK;
    void *prop = OF_FindProperty(key, "brcm,clkreq-mode");
    const char *mode = prop ? OF_GetPropValue(prop) : NULL;

    if (mode && strcmp(mode, "no-l1ss") == 0)
        reg |= PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE;
    else if (mode && strcmp(mode, "default") == 0)
        reg |= PCIE_HARD_DEBUG_L1SS_ENABLE;

    wr32(data->regs, PCIE_HARD_DEBUG, reg);
}

/* Program the MEM32 outbound window from the device tree. */
static void SetupOutbound(struct PCIBcm2712Data *data)
{
    uint64_t pci_base = 0, cpu_base = 0, size = 0, cpu_limit;
    uint32_t i;

    for (i = 0; i < data->nranges; i++)
    {
        if ((data->ranges[i].flags & 0x03000000) == 0x02000000)
        {
            pci_base = data->ranges[i].pci_base;
            cpu_base = data->ranges[i].cpu_base;
            size     = data->ranges[i].size;
            break;
        }
    }

    if (!size)
        return;

    cpu_limit = cpu_base + size - 1;

    wr32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO, (uint32_t)pci_base);
    wr32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI, (uint32_t)(pci_base >> 32));
    wr32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT,
         ((uint32_t)(cpu_base >> 20) & 0xfff) << 4 | ((uint32_t)(cpu_limit >> 20) & 0xfff) << 20);
    wr32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI, (uint32_t)(cpu_base >> 32));
    wr32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI, (uint32_t)(cpu_limit >> 32));

    /* Forward it through the bridge's own header as well */
    wr32(data->regs, PCIBR_MEMBASE,
         (((uint32_t)(cpu_limit >> 16) << 16) & 0xfff00000) |
         ((uint32_t)(pci_base >> 16) & 0xfff0));
}

/*
 * One inbound window per dma-range. Unlike 2711 - which funnels everything
 * through BAR2 plus an SCB size field - 2712 pairs each RC_BAR with a UBUS
 * remap register holding the CPU address the window lands on. The size field
 * is log2 less 15, with a separate encoding below 64KB - that branch matters,
 * the 4KB dma-range is the MSI doorbell.
 */
static void SetupInbound(struct PCIBcm2712Data *data)
{
    uint32_t i;

    for (i = 0; i < data->ndmaranges; i++)
    {
        uint64_t pci_base = data->dmaranges[i].pci_base;
        uint64_t cpu_base = data->dmaranges[i].cpu_base;
        uint32_t shift = 0, size;

        while ((1ULL << shift) < data->dmaranges[i].size)
            shift++;

        if (shift >= 12 && shift <= 15)
            size = 0x1c + (shift - 12);
        else if (shift >= 16 && shift <= 36)
            size = shift - 15;
        else
            size = 0;

        wr32(data->regs, PCIE_MISC_RC_BAR1_CONFIG_LO + i * 8,
             ((uint32_t)pci_base & ~PCIE_MISC_RC_BAR_CONFIG_SIZE_MASK) | size);
        wr32(data->regs, PCIE_MISC_RC_BAR1_CONFIG_HI + i * 8, (uint32_t)(pci_base >> 32));

        wr32(data->regs, PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_LO + i * 8,
             (uint32_t)cpu_base | PCIE_MISC_UBUS_REMAP_EN);
        wr32(data->regs, PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_HI + i * 8, (uint32_t)(cpu_base >> 32));
    }
}

/* Inverse of the size field SetupInbound() writes. 0 means the window is off. */
static uint64_t RCBarSize(uint32_t enc)
{
    if ((enc >= 1) && (enc <= 21))
        return 1ULL << (enc + 15);
    if ((enc >= 0x1c) && (enc <= 0x1f))
        return 1ULL << (enc - 0x1c + 12);

    return 0;
}

/*
 * The inbound windows as the registers hold them. Each RC_BAR carries a PCI
 * base and a size code; the CPU address it lands on is in the UBUS remap
 * register beside it, with its own enable bit. Only BAR1 through BAR3 are read
 * - the contiguous group SetupInbound() programs; a higher BAR the firmware
 * used keeps working, we just do not describe it.
 */
static void AdoptInbound(struct PCIBcm2712Data *data)
{
    uint32_t i, n = 0;

    for (i = 0; (i < 3) && (n < PCIE_MAX_RANGES); i++)
    {
        uint32_t lo   = rd32(data->regs, PCIE_MISC_RC_BAR1_CONFIG_LO + i * 8);
        uint32_t hi   = rd32(data->regs, PCIE_MISC_RC_BAR1_CONFIG_HI + i * 8);
        uint32_t rlo  = rd32(data->regs, PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_LO + i * 8);
        uint32_t rhi  = rd32(data->regs, PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_HI + i * 8);
        uint64_t size = RCBarSize(lo & PCIE_MISC_RC_BAR_CONFIG_SIZE_MASK);

        if (!size || !(rlo & PCIE_MISC_UBUS_REMAP_EN))
            continue;

        data->dmaranges[n].flags    = 0x03000000;
        data->dmaranges[n].pci_base = ((uint64_t)hi << 32) |
                                      (lo & ~(uint32_t)PCIE_MISC_RC_BAR_CONFIG_SIZE_MASK);
        data->dmaranges[n].cpu_base = ((uint64_t)rhi << 32) | (rlo & 0xfffff000);
        data->dmaranges[n].size     = size;

        BRINGUP(bug("[PCIBcm2712] %s: inbound bar%u pci %08x%08x <- cpu %08x%08x, %08x%08x bytes\n",
                    data->bridge->node, (unsigned)(i + 1),
                    (unsigned)(data->dmaranges[n].pci_base >> 32),
                    (unsigned)data->dmaranges[n].pci_base,
                    (unsigned)(data->dmaranges[n].cpu_base >> 32),
                    (unsigned)data->dmaranges[n].cpu_base,
                    (unsigned)(size >> 32), (unsigned)size));
        n++;
    }

    if (n)
        data->ndmaranges = n;
    else
        BRINGUP(bug("[PCIBcm2712] %s: no inbound window is programmed, keeping the tree's\n",
                    data->bridge->node));
}

/*
 * Depth first for the mip node whose registers are at base. msi-parent is not
 * followed - the openfirmware resource cannot resolve a phandle - but the MSI
 * block sits at the CPU address of the bridge's own 4KB dma-range, so matching
 * on the first reg entry picks the right one of the two.
 *
 * OF_FindNodeByCompatible() cannot walk this: its start argument is a subtree,
 * not a cursor, and its own root is tested first, so a match returns itself.
 */
static void *FindMIPNode(void *node, uint64_t base)
{
    void *child = NULL;
    void *prop = OF_FindProperty(node, "compatible");

    if (prop)
    {
        const char *val = OF_GetPropValue(prop);

        /* One string in this property, so the first is the whole list. */
        if (val && (strcmp(val, "brcm,bcm2712-mip") == 0))
        {
            prop = OF_FindProperty(node, "reg");
            if (prop && (OF_GetPropLen(prop) >= 4 * 4))
            {
                const uint32_t *r = (const uint32_t *)OF_GetPropValue(prop);

                /* Parent is /axi: two cells of address, two of size. */
                if ((((uint64_t)AROS_BE2LONG(r[0]) << 32) | AROS_BE2LONG(r[1])) == base)
                    return node;
            }
        }
    }

    while ((child = OF_GetChild(node, child)) != NULL)
    {
        void *found = FindMIPNode(child, base);

        if (found)
            return found;
    }

    return NULL;
}

static void AdoptMSI(struct pci_staticdata *psd, struct PCIBcm2712Data *data)
{
    uint64_t doorbell_cpu = 0, doorbell_pci = 0;
    const uint32_t *r;
    void *node, *prop;
    uint32_t i;

    for (i = 0; i < data->ndmaranges; i++)
    {
        if (data->dmaranges[i].size == 4096)
        {
            doorbell_cpu = data->dmaranges[i].cpu_base;
            doorbell_pci = data->dmaranges[i].pci_base;
            break;
        }
    }

    if (!doorbell_cpu)
    {
        BRINGUP(bug("[PCIBcm2712] %s: no doorbell window, no MSI\n", data->bridge->node));
        return;
    }

    node = FindMIPNode(OF_OpenKey("/"), doorbell_cpu);
    if (!node)
    {
        BRINGUP(bug("[PCIBcm2712] %s: no mip node at 0x%p\n", data->bridge->node,
                    (APTR)(IPTR)doorbell_cpu));
        return;
    }

    /* msi-ranges is <phandle, type, base, flags, count>; only an edge
       triggered SPI range is usable. */
    prop = OF_FindProperty(node, "msi-ranges");
    if (!prop || (OF_GetPropLen(prop) < 5 * 4))
        return;

    r = (const uint32_t *)OF_GetPropValue(prop);
    if ((AROS_BE2LONG(r[1]) != 0) || (AROS_BE2LONG(r[3]) != 1))
    {
        BRINGUP(bug("[PCIBcm2712] %s: MSI range is not an edge triggered SPI range\n",
                    data->bridge->node));
        return;
    }

    data->msi_count = AROS_BE2LONG(r[4]);
    data->msi_first_intid = AROS_BE2LONG(r[2]) + GIC_SPI_BASE;

    prop = OF_FindProperty(node, "brcm,msi-offset");
    if (prop && (OF_GetPropLen(prop) >= 4))
        data->msi_first_intid += AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop));

    if (data->msi_count > MIP_MAX_VECTORS)
        data->msi_count = MIP_MAX_VECTORS;

    data->mip = MapDevice(psd, doorbell_cpu, MIP_REG_SIZE);
    if (!data->mip)
    {
        data->msi_count = 0;
        return;
    }

    data->msi_target = doorbell_pci;

    /* Everything to the host, nothing to the VideoCore, unmasked. */
    wr32(data->mip, MIP_INT_MASKL_VPU, 0xffffffff);
    wr32(data->mip, MIP_INT_MASKH_VPU, 0xffffffff);
    wr32(data->mip, MIP_INT_CFGL_HOST, 0xffffffff);
    wr32(data->mip, MIP_INT_CFGH_HOST, 0xffffffff);
    wr32(data->mip, MIP_INT_MASKL_HOST, 0);
    wr32(data->mip, MIP_INT_MASKH_HOST, 0);

    BRINGUP(bug("[PCIBcm2712] %s: MSI at 0x%p, target %08x%08x, %u messages -> INTID %u..%u\n",
                data->bridge->node, (APTR)(IPTR)doorbell_cpu,
                (unsigned)(doorbell_pci >> 32), (unsigned)doorbell_pci,
                (unsigned)data->msi_count, (unsigned)data->msi_first_intid,
                (unsigned)(data->msi_first_intid + data->msi_count - 1)));
}

static BOOL LinkUp(struct PCIBcm2712Data *data)
{
    uint32_t reg = rd32(data->regs, PCIE_MISC_PCIE_STATUS);

    return (reg & PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP) &&
           (reg & PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE);
}

/*
 * Take a bridge from reset to a trained link with its windows programmed.
 * Only for a bridge we own outright: this resets it, and a bridge whose
 * endpoint the firmware set up - or which carries the debug console - must
 * be adopted instead.
 */
static BOOL BridgeTrain(struct PCIBcm2712Data *data, void *key)
{
    uint32_t reg;
    int timo;

    RescalReset(data);

    Perst(data, TRUE);

    BridgeReset(data, TRUE);
    delay_us(200);
    BridgeReset(data, FALSE);

    /* Power up the PHY */
    reg = rd32(data->regs, PCIE_HARD_DEBUG);
    wr32(data->regs, PCIE_HARD_DEBUG, reg & ~PCIE_HARD_DEBUG_SERDES_IDDQ);
    delay_us(200);

    BRINGUP(bug("[PCIBcm2712] %s: revision %08x\n", data->bridge->node,
                rd32(data->regs, PCIE_MISC_REVISION)));

    reg = rd32(data->regs, PCIE_MISC_MISC_CTRL);
    reg &= ~PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK;
    reg |= PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_512;
    reg |= PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN;
    reg |= PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE;
    reg |= PCIE_MISC_MISC_CTRL_PCIE_RCB_64B_MODE;
    reg |= PCIE_MISC_MISC_CTRL_PCIE_RCB_MPS_MODE;
    wr32(data->regs, PCIE_MISC_MISC_CTRL, reg);

    /* Present the root port as a PCI-PCI bridge */
    reg = rd32(data->regs, PCIE_RC_CFG_PRIV1_ID_VAL3);
    reg &= ~PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_MASK;
    wr32(data->regs, PCIE_RC_CFG_PRIV1_ID_VAL3, reg | 0x060400);

    if (!SetupRefClk(data))
        return FALSE;

    Perst(data, FALSE);

    for (timo = 100; timo > 0; timo--)
    {
        if (LinkUp(data))
            break;
        delay_us(1000);
    }

    if (timo == 0)
    {
        bug("[PCIBcm2712] %s: link did not come up (status %08x)\n", data->bridge->node,
            rd32(data->regs, PCIE_MISC_PCIE_STATUS));
        return FALSE;
    }

    BRINGUP(bug("[PCIBcm2712] %s: link up after %dms\n", data->bridge->node, 100 - timo));

    SetupClkreq(data, key);
    SetupOutbound(data);

    /* Bus numbers: primary 0, secondary 1, subordinate 1 */
    reg = rd32(data->regs, PCIBR_PRIBUS);
    wr32(data->regs, PCIBR_PRIBUS, (reg & 0xff000000) | 0x00010100);

    data->trained = TRUE;
    return TRUE;
}

/*
 * Read the bridge's translation state back out of its registers, whoever
 * programmed it, so downstream sees what the hardware does rather than what
 * a property says. On the x4 link the firmware points the outbound window at
 * a PCI address in no "ranges" entry, with the debug console behind it.
 */
static BOOL BridgeAdopt(struct PCIBcm2712Data *data)
{
    uint32_t base_limit, lo, hi;
    uint64_t cpu_base, cpu_limit, pci_base;

    if (!LinkUp(data))
        return FALSE;

    data->link_up = TRUE;
    data->secondary_bus = (UBYTE)(rd32(data->regs, PCIBR_PRIBUS) >> 8);

    lo = rd32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO);
    hi = rd32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI);
    pci_base = ((uint64_t)hi << 32) | lo;

    base_limit = rd32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT);
    cpu_base  = ((uint64_t)rd32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI) << 32) |
                ((uint64_t)(base_limit & 0x0000fff0) << 16);
    cpu_limit = ((uint64_t)rd32(data->regs, PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI) << 32) |
                ((uint64_t)(base_limit & 0xfff00000)) | 0xfffff;

    if (cpu_limit <= cpu_base)
    {
        bug("[PCIBcm2712] %s: outbound window is empty (%08x)\n",
            data->bridge->node, (unsigned)base_limit);
        return FALSE;
    }

    /*
     * The window the hardware holds replaces whatever the tree said: it is
     * the only outbound translation this driver knows how to perform, so it
     * is the only one MapPCI may offer.
     */
    data->ranges[0].flags    = 0x02000000;
    data->ranges[0].pci_base = pci_base;
    data->ranges[0].cpu_base = cpu_base;
    data->ranges[0].size     = cpu_limit - cpu_base + 1;
    data->nranges = 1;

    data->mem_pci_base = pci_base;
    data->mem_cpu_base = cpu_base;
    data->mem_size     = data->ranges[0].size;
    data->mem_next     = pci_base;

    AdoptInbound(data);

    BRINGUP(bug("[PCIBcm2712] %s: %s, secondary bus %u, window pci %08x%08x -> cpu %08x%08x, %08x%08x bytes\n",
                data->bridge->node, data->trained ? "trained here" : "adopted from firmware",
                (unsigned)data->secondary_bus,
                (unsigned)(pci_base >> 32), (unsigned)pci_base,
                (unsigned)(cpu_base >> 32), (unsigned)cpu_base,
                (unsigned)(data->mem_size >> 32), (unsigned)data->mem_size));

    return TRUE;
}

/*
 * The endpoint comes out of reset with no resources. Size BAR0 and place
 * it at the bottom of the outbound window, then let it decode: on a PC the
 * firmware does this before any driver runs, and nothing in AROS will -
 * aHidd_PCIDevice_isMaster covers only bus mastering. Without it the first
 * read of the BAR SErrors. Bus mastering is left to the endpoint's driver.
 */
static void SetupEndpoint(struct PCIBcm2712Data *data)
{
    UBYTE bus = data->secondary_bus;
    ULONG id, lo, size;
    BOOL is64;

    id = PCIE_ReadConfig(data, bus, 0, 0, 0x00);
    if (id == 0xffffffff || id == 0)
    {
        BRINGUP(bug("[PCIBcm2712] %s: no endpoint at %u:0.0\n",
                    data->bridge->node, (unsigned)bus));
        return;
    }

    lo = PCIE_ReadConfig(data, bus, 0, 0, 0x10);
    is64 = ((lo & 0x06) == 0x04);

    /* Size the BAR: all-ones comes back with the writable bits set. */
    PCIE_WriteConfig(data, bus, 0, 0, 0x10, 0xffffffff);
    size = PCIE_ReadConfig(data, bus, 0, 0, 0x10) & ~0xfUL;
    size = size ? (~size + 1) : 0;

    if (size && data->mem_size)
    {
        uint64_t addr = (data->mem_next + size - 1) & ~(uint64_t)(size - 1);

        if (addr + size <= data->mem_pci_base + data->mem_size)
        {
            PCIE_WriteConfig(data, bus, 0, 0, 0x10, (ULONG)addr);
            if (is64)
                PCIE_WriteConfig(data, bus, 0, 0, 0x14, (ULONG)(addr >> 32));
            data->mem_next = addr + size;
        }
        else
        {
            bug("[PCIBcm2712] %s: BAR0 (%u bytes) does not fit the window\n",
                data->bridge->node, (unsigned)size);
            PCIE_WriteConfig(data, bus, 0, 0, 0x10, lo);
        }
    }
    else
        PCIE_WriteConfig(data, bus, 0, 0, 0x10, lo);

    /* Cache line size, as every reference stack programs */
    PCIE_WriteConfig(data, bus, 0, 0, 0x0c,
                     (PCIE_ReadConfig(data, bus, 0, 0, 0x0c) & ~0xffUL) | 16);

    PCIE_WriteConfig(data, bus, 0, 0, PCICS_COMMAND,
                     PCIE_ReadConfig(data, bus, 0, 0, PCICS_COMMAND) | PCICMF_MEMDECODE);

    BRINGUP(bug("[PCIBcm2712] %s: endpoint %u:0.0 %04x:%04x class %06x cmd %04x BAR0 %08x%08x (%u bytes)\n",
                data->bridge->node, (unsigned)bus,
                (unsigned)(id & 0xffff), (unsigned)(id >> 16),
                (unsigned)(PCIE_ReadConfig(data, bus, 0, 0, 0x08) >> 8),
                (unsigned)(PCIE_ReadConfig(data, bus, 0, 0, PCICS_COMMAND) & 0xffff),
                (unsigned)(is64 ? PCIE_ReadConfig(data, bus, 0, 0, 0x14) : 0),
                (unsigned)PCIE_ReadConfig(data, bus, 0, 0, 0x10), (unsigned)size));
}

/*
 * Called from Root::New, once per bridge. Maps the registers, reads what the
 * tree says, trains the bridge if nothing has, and then reads the result
 * back. Returns FALSE if there is nothing usable, in which case no driver
 * object is left behind.
 */
BOOL PCIE_BridgeSetup(struct pci_staticdata *psd, struct PCIBcm2712Data *data)
{
    void *key;
    uint32_t reg;

    key = PCIeNode(data->bridge);
    if (!key)
        return FALSE;

    data->regs = MapDevice(psd, data->bridge->reg_base, BCM2712_PCIE_REG_SIZE);
    if (!data->regs)
    {
        bug("[PCIBcm2712] %s: could not map the registers\n", data->bridge->node);
        return FALSE;
    }

    data->rescal = MapDevice(psd, BCM2712_RESCAL_BASE, BCM2712_RESCAL_SIZE);
    data->reset  = MapDevice(psd, BCM2712_RESET_BASE, BCM2712_RESET_SIZE);
    data->reset_cell = ResetCellFromDT(key);
    data->intx_irq   = IntxIrqFromDT(key);

    data->nranges    = ParseRanges(key, "ranges", data->ranges);
    data->ndmaranges = ParseRanges(key, "dma-ranges", data->dmaranges);

    BRINGUP(bug("[PCIBcm2712] %s: INTA -> INTID %u\n", data->bridge->node,
                (unsigned)data->intx_irq));

    if (!data->ndmaranges)
    {
        /* Without an inbound window a bus master reaches no memory at all,
           and there is no sane default to fall back on. */
        bug("[PCIBcm2712] %s: no dma-ranges in the device tree\n", data->bridge->node);
        return FALSE;
    }

    /*
     * A link the firmware already trained is left alone. Resetting one
     * costs whatever the firmware set up behind it, which on the x4 link
     * is the console this would be reporting through.
     */
    if (!LinkUp(data))
    {
        if (!BridgeTrain(data, key))
            return FALSE;
    }
    else
        BRINGUP(bug("[PCIBcm2712] %s: link already up, adopting it\n", data->bridge->node));

    /*
     * The inbound side is ours on both paths. Nothing is transferring during
     * boot, and the console is outbound traffic, so reprogramming these
     * windows costs nothing - while the firmware's leave the x4 endpoint's
     * DMA and its MSI doorbell pointing at nothing we describe.
     */
    SetupInbound(data);

    reg = rd32(data->regs, PCICS_COMMAND);
    wr32(data->regs, PCICS_COMMAND, reg | PCICMF_MEMDECODE | PCICMF_BUSMASTER);

    if (!BridgeAdopt(data))
        return FALSE;

    /*
     * The bus numbers, on an adopted bridge too: the firmware leaves them
     * at zero on the x4 link, and config space cannot reach an endpoint on
     * a bus the bridge does not forward. This touches no window, so it
     * costs the console nothing.
     */
    if (!data->secondary_bus)
    {
        uint32_t reg = rd32(data->regs, PCIBR_PRIBUS);

        wr32(data->regs, PCIBR_PRIBUS, (reg & 0xff000000) | 0x00010100);
        data->secondary_bus = (UBYTE)(rd32(data->regs, PCIBR_PRIBUS) >> 8);

        BRINGUP(bug("[PCIBcm2712] %s: bus numbers were unset, secondary bus now %u\n",
                    data->bridge->node, (unsigned)data->secondary_bus));
    }

    AdoptMSI(psd, data);

    if (data->trained)
        SetupEndpoint(data);

    return TRUE;
}

static int PCIBcm2712_InitClass(LIBBASETYPEPTR LIBBASE)
{
    struct pci_staticdata *psd = &LIBBASE->psd;
    APTR KernelBase;
    OOP_Object *pci;
    unsigned int i;

    if (!PCIeEnabled())
        return TRUE;

    OpenFirmwareBase = OpenResource("openfirmware.resource");
    if (!OpenFirmwareBase)
        return TRUE;

    KernelBase = OpenResource("kernel.resource");
    psd->kernelBase = (struct Library *)KernelBase;
    if (!KernelBase)
        return TRUE;

    psd->hiddPCIDriverAB = OOP_ObtainAttrBase(IID_Hidd_PCIDriver);
    psd->hiddPCIDeviceAB = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    psd->hiddAB = OOP_ObtainAttrBase(IID_Hidd);
    if (psd->hiddPCIDriverAB == 0 || psd->hiddPCIDeviceAB == 0 || psd->hiddAB == 0)
    {
        D(bug("[PCIBcm2712] ObtainAttrBases failed\n"));
        return TRUE;
    }

    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci)
    {
        bug("[PCIBcm2712] could not create a pci.hidd object\n");
        return TRUE;
    }

    for (i = 0; i < BCM2712_NBRIDGES; i++)
    {
        struct TagItem instanceTags[] = {
            { aHidd_DriverData, (IPTR)&bcm2712_bridges[i] },
            { TAG_DONE,         0                         }
        };
        struct pHidd_PCI_AddHardwareDriver msg;

        BRINGUP(bug("[PCIBcm2712] starting bring-up of %s\n", bcm2712_bridges[i].node));

        msg.mID          = OOP_GetMethodID(IID_Hidd_PCI, moHidd_PCI_AddHardwareDriver);
        msg.driverClass  = psd->driverClass;
        msg.instanceTags = instanceTags;

        OOP_DoMethod(pci, (OOP_Msg)&msg);
    }

    OOP_DisposeObject(pci);

    return TRUE;
}

static int PCIBcm2712_ExpungeClass(LIBBASETYPEPTR LIBBASE)
{
    OOP_ReleaseAttrBase(IID_Hidd_PCIDriver);
    OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    OOP_ReleaseAttrBase(IID_Hidd);

    return TRUE;
}

ADD2INITLIB(PCIBcm2712_InitClass, 0)
ADD2EXPUNGELIB(PCIBcm2712_ExpungeClass, 0)
