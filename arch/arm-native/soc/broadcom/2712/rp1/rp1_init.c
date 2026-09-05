/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    RP1 Southbridge driver for Raspberry Pi 5
*/

/* Bring-up diagnostics: window and peripheral addresses. */
#define DEBUG 1

#include <exec/types.h>
#include <exec/memory.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/macros.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/openfirmware.h>

#include <string.h>

#include "rp1.h"

#include LC_LIBDEFS_FILE

/* Named for the OF_ macros, which call through a base of this name. */
static IPTR query_rp1(APTR OpenFirmwareBase, IPTR *dmaoff)
{
    void *key, *prop;
    const uint32_t *r;
    uint32_t child_ac, child_sc, parent_ac, entry_cells, cells;
    IPTR win = 0;

    *dmaoff = 0;

    key = OF_OpenKey("/axi/pcie@1000120000");
    if (!key)
    {
        D(bug("[RP1] no PCIe node for RP1 in the device tree\n"));
        return 0;
    }

    /* OF_OpenKey falls back to the last resolved node on a miss. */
    prop = OF_FindProperty(key, "compatible");
    if (!prop || !strstr(OF_GetPropValue(prop), "2712-pcie"))
    {
        D(bug("[RP1] node is not a BCM2712 PCIe bridge\n"));
        return 0;
    }

    if (!OF_GetChild(key, NULL))
    {
        D(bug("[RP1] PCIe bridge has no devices below it\n"));
        return 0;
    }

    prop = OF_FindProperty(key, "#address-cells");
    child_ac = prop ? AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)) : 3;
    prop = OF_FindProperty(key, "#size-cells");
    child_sc = prop ? AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)) : 2;

    key = OF_OpenKey("/axi");
    prop = key ? OF_FindProperty(key, "#address-cells") : NULL;
    parent_ac = prop ? AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)) : 2;

    key = OF_OpenKey("/axi/pcie@1000120000");
    prop = OF_FindProperty(key, "ranges");
    if (!prop)
        return 0;

    entry_cells = child_ac + parent_ac + child_sc;
    r = (const uint32_t *)OF_GetPropValue(prop);
    cells = OF_GetPropLen(prop) / 4;

    while (cells >= entry_cells)
    {
        /* The first child cell is the PCI space code, not an address. */
        uint32_t space = AROS_BE2LONG(r[0]);
        uint64_t cpu = 0, len = 0;
        uint32_t i;

        r += child_ac;
        for (i = 0; i < parent_ac; i++)
            cpu = (cpu << 32) | AROS_BE2LONG(*r++);
        for (i = 0; i < child_sc; i++)
            len = (len << 32) | AROS_BE2LONG(*r++);

        D(bug("[RP1] window: space %08x -> 0x%p, 0x%p bytes\n",
              space, (APTR)(IPTR)cpu, (APTR)(IPTR)len));

        /* Prefetchable memory (space code 0x03) is the one in use. */
        if (!win && ((space >> 24) & 0x03) == 0x03)
            win = (IPTR)cpu;

        cells -= entry_cells;
    }

    /* Inbound windows; the widest one covers system memory. */
    prop = OF_FindProperty(key, "dma-ranges");
    if (prop)
    {
        uint64_t best = 0;

        r = (const uint32_t *)OF_GetPropValue(prop);
        cells = OF_GetPropLen(prop) / 4;

        while (cells >= entry_cells)
        {
            uint64_t pci = 0, cpu = 0, len = 0;
            uint32_t i;

            r++;                        /* space code */
            for (i = 1; i < child_ac; i++)
                pci = (pci << 32) | AROS_BE2LONG(*r++);
            for (i = 0; i < parent_ac; i++)
                cpu = (cpu << 32) | AROS_BE2LONG(*r++);
            for (i = 0; i < child_sc; i++)
                len = (len << 32) | AROS_BE2LONG(*r++);

            D(bug("[RP1] dma window: pci 0x%p <- cpu 0x%p, 0x%p bytes\n",
                  (APTR)(IPTR)pci, (APTR)(IPTR)cpu, (APTR)(IPTR)len));

            if (len > best)
            {
                best = len;
                *dmaoff = (IPTR)(pci - cpu);
            }

            cells -= entry_cells;
        }
    }
    else
    {
        D(bug("[RP1] no dma-ranges - assuming an identity inbound window\n"));
    }

    return win;
}

/* Offsets into the BCM2712 PCIe RC, same layout as arch/../2711/pcie/pcie.h */
#define RC_VENDOR_SPECIFIC_REG1 0x0188
#define  RC_ENDIAN_BAR2_MASK    (0x3 << 2)
#define  RC_ENDIAN_BAR2_LITTLE  (0x0 << 2)
#define RC_MISC_CTRL            0x4008
#define RC_MEM_WIN0_LO          0x400c
#define RC_MEM_WIN0_HI          0x4010
#define RC_MEM_WIN0_BASE_LIMIT  0x4070
#define RC_MEM_WIN0_BASE_HI     0x4080
#define RC_MEM_WIN0_LIMIT_HI    0x4084
#define RC_BAR2_CONFIG_LO       0x4034
#define RC_BAR2_CONFIG_HI       0x4038
/* 2712 only: the CPU side of an inbound window, with its own enable. */
#define RC_UBUS_BAR2_REMAP_LO   0x40b4
#define RC_UBUS_BAR2_REMAP_HI   0x40b8
#define  RC_UBUS_REMAP_ENABLE   (1 << 0)
#define  RC_UBUS_REMAP_LO_MASK  0xfffff000
#define  RC_UBUS_REMAP_HI_MASK  0xff
#define RC_MSI_BAR_CONFIG_LO    0x4044
#define RC_MSI_BAR_CONFIG_HI    0x4048
#define RC_EXT_CFG_DATA         0x8000
#define RC_EXT_CFG_INDEX        0x9000
#define RC_REGS_SIZE            0xa000

#define EXT_CFG_ADDR(bus, dev, fn)  (((bus) << 20) | ((dev) << 15) | ((fn) << 12))

/* PCI command register bits we care about */
#define PCI_CMD_MEMORY          (1 << 1)
#define PCI_CMD_MASTER          (1 << 2)

/* RP1 interrupt-to-message block in BAR1; REG_SET is an atomic set alias.
   Datasheet 6.2, register layout from edk2-platforms Rp1BusDxe. */
#define RP1_PCIE_BASE           0x00108000
#define RP1_PCIE_REG_SET        (RP1_PCIE_BASE + 0x800)
#define RP1_PCIE_MSIX_CFG(irq)  (0x008 + ((irq) * 4))
#define  RP1_MSIX_CFG_ENABLE    (1 << 0)
#define RP1_INT_USBHOST0_0      31
#define RP1_INT_USBHOST1_0      36

/* Inbound window 1, for the MSI target: it sits outside the memory window. */
#define RC_BAR4_CONFIG_LO       0x40d4
#define RC_BAR4_CONFIG_HI       0x40d8
#define RC_UBUS_BAR4_REMAP_LO   0x410c
#define RC_UBUS_BAR4_REMAP_HI   0x4110
#define  RC_IB_SIZE_4K          0x1c        /* edk2 PcieEncodeInboundSize */

/* MSI Interrupt Peripheral, one dedicated SPI per message - nothing to
   demultiplex.  Register layout from OpenBSD bcm2712_mip.c (ISC). */
#define MIP_INT_CFGL_HOST       0x20
#define MIP_INT_CFGH_HOST       0x30
#define MIP_INT_MASKL_HOST      0x40
#define MIP_INT_MASKH_HOST      0x50
#define MIP_INT_MASKL_VPU       0x60
#define MIP_INT_MASKH_VPU       0x70

#define GIC_SPI_BASE            32          /* INTID = SPI + 32 */

/* MSI-X capability, and the table it points at */
#define PCI_CAP_ID_MSIX         0x11
#define  MSIX_CTRL_ENABLE       (1 << 31)   /* bit 15 of the 16-bit ctrl */
#define  MSIX_TABLE_BIR_MASK    0x7
#define  MSIX_TABLE_OFF_MASK    ~0x7u

#define RCRD(rc, o) (*(volatile uint32_t *)((IPTR)(rc) + (o)))

/* Root port config space is the first 4K of the register block; the
   endpoint's goes through the shared EXT_CFG window, so index and data
   have to stay together.  The bus number is the programmed secondary -
   a wrong one answers every read with all ones. */
static uint32_t rc_cfg_read(IPTR rc, uint32_t reg)
{
    return RCRD(rc, reg);
}

static uint8_t rc_secondary_bus(IPTR rc)
{
    return (uint8_t)(rc_cfg_read(rc, 0x18) >> 8);
}

static uint32_t ep_cfg_read(IPTR rc, uint32_t reg)
{
    uint32_t val;

    Disable();
    RCRD(rc, RC_EXT_CFG_INDEX) = EXT_CFG_ADDR(rc_secondary_bus(rc), 0, 0);
    val = RCRD(rc, RC_EXT_CFG_DATA + reg);
    Enable();

    return val;
}

static void ep_cfg_write(IPTR rc, uint32_t reg, uint32_t val)
{
    Disable();
    RCRD(rc, RC_EXT_CFG_INDEX) = EXT_CFG_ADDR(rc_secondary_bus(rc), 0, 0);
    RCRD(rc, RC_EXT_CFG_DATA + reg) = val;
    Enable();
}

/* Mapped writable - the inbound windows are programmed through it. */
static IPTR map_rc(APTR OpenFirmwareBase, APTR KernelBase)
{
    void *key, *prop;
    uint64_t rc = 0;

    key = OF_OpenKey("/axi/pcie@1000120000");
    prop = key ? OF_FindProperty(key, "reg") : NULL;
    if (prop && (OF_GetPropLen(prop) >= 8))
    {
        const uint32_t *r = (const uint32_t *)OF_GetPropValue(prop);

        rc = ((uint64_t)AROS_BE2LONG(r[0]) << 32) | AROS_BE2LONG(r[1]);
    }

    if (!rc)
    {
        D(bug("[RP1] no usable reg on the PCIe bridge\n"));
        return 0;
    }

    if (!KrnMapGlobal((void *)(IPTR)rc, (void *)(IPTR)rc, RC_REGS_SIZE,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
    {
        D(bug("[RP1] failed to map the bridge registers at 0x%p\n", (APTR)(IPTR)rc));
        return 0;
    }

    return (IPTR)rc;
}

/* Where the window actually is, not where dma-ranges says it should be.
   The low five bits are the size code. */
static IPTR rc_dma_offset(IPTR rc)
{
    uint32_t lo = RCRD(rc, RC_BAR2_CONFIG_LO);
    uint32_t hi = RCRD(rc, RC_BAR2_CONFIG_HI);

    return (IPTR)(((uint64_t)hi << 32) | (lo & ~0x1fU));
}

/* Nothing else resolves phandles for us. */
static void *find_by_phandle(APTR OpenFirmwareBase, void *key, uint32_t want, int depth)
{
    void *child, *prop, *hit;

    if (depth > 6)
        return NULL;

    prop = OF_FindProperty(key, "phandle");
    if (prop && (OF_GetPropLen(prop) >= 4) &&
        (AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)) == want))
        return key;

    for (child = OF_GetChild(key, NULL); child; child = OF_GetChild(key, child))
    {
        hit = find_by_phandle(OpenFirmwareBase, child, want, depth + 1);
        if (hit)
            return hit;
    }

    return NULL;
}

/* Inbound window, MIP, MSI-X table and the two USB vectors. */
static void setup_msi(APTR OpenFirmwareBase, APTR KernelBase, IPTR rc, IPTR win,
                      uint64_t msi_target, IPTR mipbase, uint32_t base_spi,
                      LIBBASETYPEPTR LIBBASE)
{
    uint32_t cap, ctrl, tbl;
    IPTR bar0, mip, table;

    /* Inbound window 1, so a message written to the target reaches the MIP. */
    RCRD(rc, RC_BAR4_CONFIG_LO) = ((uint32_t)msi_target & ~0x1fu) | RC_IB_SIZE_4K;
    RCRD(rc, RC_BAR4_CONFIG_HI) = (uint32_t)(msi_target >> 32);
    RCRD(rc, RC_UBUS_BAR4_REMAP_LO) = ((uint32_t)mipbase & RC_UBUS_REMAP_LO_MASK) |
                                      RC_UBUS_REMAP_ENABLE;
    RCRD(rc, RC_UBUS_BAR4_REMAP_HI) = (uint32_t)((UQUAD)mipbase >> 32) & RC_UBUS_REMAP_HI_MASK;

    D(bug("[RP1MSI] ib win1 %08x:%08x remap %08x:%08x\n",
          (unsigned)RCRD(rc, RC_BAR4_CONFIG_HI), (unsigned)RCRD(rc, RC_BAR4_CONFIG_LO),
          (unsigned)RCRD(rc, RC_UBUS_BAR4_REMAP_HI), (unsigned)RCRD(rc, RC_UBUS_BAR4_REMAP_LO)));

    /* The MIP itself: keep the VPU out, route everything to the host. */
    if (!KrnMapGlobal((void *)mipbase, (void *)mipbase, 0x1000,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
    {
        D(bug("[RP1MSI] failed to map the MIP at 0x%p\n", (APTR)mipbase));
        return;
    }
    mip = mipbase;

    RCRD(mip, MIP_INT_MASKL_VPU) = 0xffffffff;
    RCRD(mip, MIP_INT_MASKH_VPU) = 0xffffffff;
    RCRD(mip, MIP_INT_CFGL_HOST) = 0xffffffff;
    RCRD(mip, MIP_INT_CFGH_HOST) = 0xffffffff;
    RCRD(mip, MIP_INT_MASKL_HOST) = 0;
    RCRD(mip, MIP_INT_MASKH_HOST) = 0;

    /* The table's BAR sits next to BAR1 in the outbound window. */
    for (cap = ep_cfg_read(rc, 0x34) & 0xfc; cap && (cap != 0xfc); cap = (ep_cfg_read(rc, cap) >> 8) & 0xfc)
        if ((ep_cfg_read(rc, cap) & 0xff) == PCI_CAP_ID_MSIX)
            break;

    if (!cap || (cap == 0xfc))
    {
        D(bug("[RP1MSI] no MSI-X capability\n"));
        return;
    }

    tbl  = ep_cfg_read(rc, cap + 4);
    bar0 = ep_cfg_read(rc, 0x10) & ~0xfu;
    table = win + (bar0 - (ep_cfg_read(rc, 0x14) & ~0xfu)) + (tbl & MSIX_TABLE_OFF_MASK);

    D(bug("[RP1MSI] msix cap @%02x tbl %08x bir %u -> table at 0x%p\n",
          (unsigned)cap, (unsigned)tbl, (unsigned)(tbl & MSIX_TABLE_BIR_MASK), (APTR)table));

    if (!KrnMapGlobal((void *)(table & ~0xfffUL), (void *)(table & ~0xfffUL), 0x1000,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
    {
        D(bug("[RP1MSI] failed to map the MSI-X table\n"));
        return;
    }

    /* Every entry carries its own index, so which one an interrupt picks
       does not have to be assumed. */
    {
        volatile uint32_t *e = (volatile uint32_t *)table;
        uint32_t i, n = (ep_cfg_read(rc, cap) >> 16) & 0x7ff;

        for (i = 0; i <= n; i++)
        {
            e[i * 4 + 0] = (uint32_t)msi_target;
            e[i * 4 + 1] = (uint32_t)(msi_target >> 32);
            e[i * 4 + 2] = i;       /* message data = MSI number */
            e[i * 4 + 3] = 0;       /* unmasked */
        }

        D(bug("[RP1MSI] programmed %u msix table entries\n", (unsigned)(n + 1)));
    }

    /* Nothing has ever set MSI-X Enable, so the endpoint would stay silent. */
    ctrl = ep_cfg_read(rc, cap);
    ep_cfg_write(rc, cap, ctrl | MSIX_CTRL_ENABLE);
    D(bug("[RP1MSI] msix ctrl %08x -> %08x\n",
          (unsigned)ctrl, (unsigned)ep_cfg_read(rc, cap)));

    /* Entry i carries message i, so the MIP raises base_spi + i. */
    {
        volatile uint32_t *set0 = (volatile uint32_t *)
            (win + RP1_PCIE_REG_SET + RP1_PCIE_MSIX_CFG(RP1_INT_USBHOST0_0));
        volatile uint32_t *set1 = (volatile uint32_t *)
            (win + RP1_PCIE_REG_SET + RP1_PCIE_MSIX_CFG(RP1_INT_USBHOST1_0));

        *set0 = RP1_MSIX_CFG_ENABLE;
        *set1 = RP1_MSIX_CFG_ENABLE;

        LIBBASE->rp1_USBIrq0 = base_spi + RP1_INT_USBHOST0_0 + GIC_SPI_BASE;
        LIBBASE->rp1_USBIrq1 = base_spi + RP1_INT_USBHOST1_0 + GIC_SPI_BASE;

        D(bug("[RP1MSI] usb irqs: host0 INTID %u, host1 INTID %u\n",
              (unsigned)LIBBASE->rp1_USBIrq0, (unsigned)LIBBASE->rp1_USBIrq1));
    }

    /* MSIX_CFG.TEST is avoided: it latches through the set alias and
       leaves no rising edge for real interrupts. */
}

static int RP1_Init(LIBBASETYPEPTR LIBBASE)
{
    IPTR win, dtoff, dmaoff, rc;

    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    /* Named for the KrnMapGlobal macro below, as OpenFirmwareBase is for
     * the OF_ ones. */
    APTR KernelBase = OpenResource("kernel.resource");

    /* Without a device tree */
    if (!OpenFirmwareBase || !KernelBase)
        return TRUE;

    win = query_rp1(OpenFirmwareBase, &dtoff);
    if (!win)
        return TRUE;                    /* not an RP1 board */

    rc = map_rc(OpenFirmwareBase, KernelBase);

    /* The firmware leaves this at 0, which claims nothing RP1 emits. */
    if (rc && dtoff)
    {
        uint32_t lo = RCRD(rc, RC_BAR2_CONFIG_LO);

        RCRD(rc, RC_BAR2_CONFIG_LO) = (uint32_t)dtoff | (lo & 0x1f);
        RCRD(rc, RC_BAR2_CONFIG_HI) = (uint32_t)((UQUAD)dtoff >> 32);

        /* CPU side of the same window: system memory starts at 0. */
        RCRD(rc, RC_UBUS_BAR2_REMAP_LO) = (0 & RC_UBUS_REMAP_LO_MASK) |
                                          RC_UBUS_REMAP_ENABLE;
        RCRD(rc, RC_UBUS_BAR2_REMAP_HI) = 0 & RC_UBUS_REMAP_HI_MASK;

        D(bug("[RP1] inbound window: bar2 %08x:%08x remap %08x:%08x\n",
              (unsigned)RCRD(rc, RC_BAR2_CONFIG_HI),
              (unsigned)RCRD(rc, RC_BAR2_CONFIG_LO),
              (unsigned)RCRD(rc, RC_UBUS_BAR2_REMAP_HI),
              (unsigned)RCRD(rc, RC_UBUS_BAR2_REMAP_LO)));
    }
    else if (!rc)
    {
        D(bug("[RP1] no bridge mapping - inbound window left as the firmware set it\n"));
    }

    /* No device object to set aHidd_PCIDevice_isMaster on here. */
    if (rc)
    {
        uint32_t cs = rc_cfg_read(rc, 0x04);

        if ((cs & (PCI_CMD_MEMORY | PCI_CMD_MASTER)) != (PCI_CMD_MEMORY | PCI_CMD_MASTER))
        {
            RCRD(rc, 0x04) = cs | PCI_CMD_MEMORY | PCI_CMD_MASTER;
            D(bug("[RP1] root port cmd %04x -> %04x\n", (unsigned)(cs & 0xffff),
                  (unsigned)(rc_cfg_read(rc, 0x04) & 0xffff)));
        }
    }

    if (rc && (ep_cfg_read(rc, 0x00) != 0xffffffff))
    {
        uint32_t cs = ep_cfg_read(rc, 0x04);

        if ((cs & (PCI_CMD_MEMORY | PCI_CMD_MASTER)) != (PCI_CMD_MEMORY | PCI_CMD_MASTER))
        {
            ep_cfg_write(rc, 0x04, cs | PCI_CMD_MEMORY | PCI_CMD_MASTER);
            D(bug("[RP1] endpoint cmd %04x -> %04x\n", (unsigned)(cs & 0xffff),
                  (unsigned)(ep_cfg_read(rc, 0x04) & 0xffff)));
        }
    }

    dmaoff = rc ? rc_dma_offset(rc) : dtoff;

    if (!KrnMapGlobal((void *)win, (void *)win, RP1_BAR1_SIZE,
                      MAP_Readable | MAP_Writable | MAP_CacheInhibit | MAP_Guarded))
    {
        D(bug("[RP1] failed to map the peripheral window at 0x%p\n", (APTR)win));
        return TRUE;
    }
    
    /*
     * Does an MSI from RP1 reach the bridge at all?  MSIX_CFG.TEST fires
     * one without needing a working controller behind it, and the 2711
     * style latch is the only place we know to look.  Nothing is wired to
     * the GIC yet, so a latched bit is read and cleared here.
     */
    if (rc)
    {
        void *key = OF_OpenKey("/axi/pcie@1000120000");
        void *root = OF_OpenKey("/");
        void *prop = key ? OF_FindProperty(key, "msi-parent") : NULL;
        void *mipnode = NULL;

        if (prop && (OF_GetPropLen(prop) >= 4) && root)
            mipnode = find_by_phandle(OpenFirmwareBase, root,
                          AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)), 0);

        prop = mipnode ? OF_FindProperty(mipnode, "reg") : NULL;
        if (prop && (OF_GetPropLen(prop) >= 32))
        {
            const uint32_t *r = (const uint32_t *)OF_GetPropValue(prop);
            IPTR     mipbase = ((IPTR)AROS_BE2LONG(r[0]) << 32) | AROS_BE2LONG(r[1]);
            uint64_t target  = ((uint64_t)AROS_BE2LONG(r[4]) << 32) | AROS_BE2LONG(r[5]);
            uint32_t base_spi = 0;
            void *mr = OF_FindProperty(mipnode, "msi-ranges");

            /* <gic-phandle, type, base SPI, flags, count> */
            if (mr && (OF_GetPropLen(mr) >= 20))
                base_spi = AROS_BE2LONG(((const uint32_t *)OF_GetPropValue(mr))[2]);

            D(bug("[RP1MSI] mip regs 0x%p, msi target 0x%p, base spi %u\n",
                  (APTR)mipbase, (APTR)(IPTR)target, (unsigned)base_spi));

            if (base_spi)
                setup_msi(OpenFirmwareBase, KernelBase, rc, win, target, mipbase,
                          base_spi, LIBBASE);
        }
        else
        {
            D(bug("[RP1MSI] no usable MIP node - MSI left alone\n"));
        }
    }

    LIBBASE->rp1_BAR1 = win;
    LIBBASE->rp1_DMAOffset = dmaoff;
    LIBBASE->rp1_Present = TRUE;

    /* Export peripheral addresses */
    LIBBASE->rp1_USB0  = win + RP1_USB0_OFFSET;
    LIBBASE->rp1_USB1  = win + RP1_USB1_OFFSET;
    LIBBASE->rp1_ETH   = win + RP1_ETH_OFFSET;
    LIBBASE->rp1_GPIO  = win + RP1_GPIO_OFFSET;
    LIBBASE->rp1_I2C0  = win + RP1_I2C0_OFFSET;
    LIBBASE->rp1_I2C1  = win + RP1_I2C1_OFFSET;
    LIBBASE->rp1_UART0 = win + RP1_UART0_OFFSET;

    D(bug("[RP1] USB0=0x%p USB1=0x%p ETH=0x%p GPIO=0x%p I2C0=0x%p I2C1=0x%p UART0=0x%p\n",
          LIBBASE->rp1_USB0, LIBBASE->rp1_USB1, LIBBASE->rp1_ETH,
          LIBBASE->rp1_GPIO, LIBBASE->rp1_I2C0, LIBBASE->rp1_I2C1,
          LIBBASE->rp1_UART0));

    return TRUE;
}

ADD2INITLIB(RP1_Init, 0)
