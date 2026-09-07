/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The Broadcom register map and the bring-up sequence below follow OpenBSD's
    sys/dev/fdt/bcm2711_pcie.c, which drives both brcm,bcm2711-pcie and
    brcm,bcm2712-pcie. The BCM2712 PCIe block is not covered by any published
    Broadcom documentation, so that driver is the reference this one is
    written against:

    Copyright (c) 2020, 2025 Mark Kettenis <kettenis@openbsd.org>

    Permission to use, copy, modify, and distribute this software for any
    purpose with or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

    Desc: BCM2712 (Raspberry Pi 5 / 500 / 500+) PCIe host bridge.
*/

#ifndef PCIE_BCM2712_H
#define PCIE_BCM2712_H

#include <inttypes.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include LC_LIBDEFS_FILE

/*
 * BCM2712 has three host bridges, each a separate root complex with its own
 * register block, reset bit, windows and MSI block. pcie0 is not brought out
 * on any board we know of. pcie1 is the x1 link - Pi 5 FPC connector, Pi 500+
 * M.2 slot - and needs "dtparam=nvme" or its node stays disabled. pcie2 is the
 * x4 link, hardwired to the RP1 southbridge. One driver object per bridge.
 */
#define BCM2712_PCIE_REG_SIZE           0x9310

/*
 * The rescal block and the reset controller are shared by all three
 * bridges and live in /soc. Neither is described by anything but the
 * device tree; these are the addresses after /soc's ranges translation.
 */
#define BCM2712_RESCAL_BASE             0x1000119500ULL
#define BCM2712_RESCAL_SIZE             0x10
#define BCM2712_RESET_BASE              0x1001504318ULL
#define BCM2712_RESET_SIZE              0x30

/* Rescal: self-deasserting, so only the assert side exists. */
#define RESCAL_START                    0x00
#define  RESCAL_START_BIT               (1 << 0)
#define RESCAL_STATUS                   0x08
#define  RESCAL_STATUS_BIT              (1 << 0)

/* brcmstb reset: banks of 32 bits, set/clear registers 24 bytes apart. */
#define RESET_SW_INIT_SET(bank)         (0x00 + (bank) * 24)
#define RESET_SW_INIT_CLR(bank)         (0x04 + (bank) * 24)

/*
 * The root complex configuration header is memory mapped at offset 0 of
 * the register block; external configuration space is reached through the
 * indexed window at the top of it. There is no ECAM.
 */
#define PCIE_RC_CFG_PRIV1_ID_VAL3                       0x043c
#define  PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_MASK           (0xffffff << 0)

/* MDIO to the SerDes, used to pick the reference clock. */
#define PCIE_RC_DL_MDIO_ADDR                            0x1100
#define  PCIE_RC_DL_MDIO_PORT_SHIFT                     16
#define  PCIE_RC_DL_MDIO_CMD_READ                       (1 << 20)
#define  PCIE_RC_DL_MDIO_CMD_WRITE                      (0 << 20)
#define PCIE_RC_DL_MDIO_WR_DATA                         0x1104
#define PCIE_RC_DL_MDIO_RD_DATA                         0x1108
#define  PCIE_RC_DL_MDIO_DATA_DONE                      (1U << 31)

#define PCIE_RC_PL_PHY_CTL_15                           0x184c
#define  PCIE_RC_PL_PHY_CTL_15_PM_CLK_PERIOD_MASK       (0xff << 0)

#define PCIE_MISC_MISC_CTRL                             0x4008
#define  PCIE_MISC_MISC_CTRL_PCIE_RCB_64B_MODE          (1 << 7)
#define  PCIE_MISC_MISC_CTRL_PCIE_RCB_MPS_MODE          (1 << 10)
#define  PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN              (1 << 12)
#define  PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE           (1 << 13)
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK        (0x3 << 20)
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_512         (0x2 << 20)

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO                0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI                0x4010

/* Inbound windows: one per dma-range, 8 bytes apart from BAR1. */
#define PCIE_MISC_RC_BAR1_CONFIG_LO                     0x402c
#define  PCIE_MISC_RC_BAR_CONFIG_SIZE_MASK              (0x1f << 0)
#define PCIE_MISC_RC_BAR1_CONFIG_HI                     0x4030

#define PCIE_MISC_PCIE_CTRL                             0x4064
#define  PCIE_MISC_PCIE_CTRL_PCIE_PERSTB                (1 << 2)
#define PCIE_MISC_PCIE_STATUS                           0x4068
#define  PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP           (1 << 4)
#define  PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE           (1 << 5)
#define PCIE_MISC_REVISION                              0x406c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT        0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI           0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI          0x4084

/* UBUS remap: the CPU-side half of each inbound window. 2711 has no UBUS. */
#define PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_LO             0x40ac
#define  PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_EN            (1 << 0)
#define PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_HI             0x40b0

#define PCIE_MISC_AXI_READ_ERROR_DATA                   0x4170

/* Not 0x4204 as on 2711 - the block moved on this SoC. */
#define PCIE_HARD_DEBUG                                 0x4304
#define  PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE            (1 << 1)
#define  PCIE_HARD_DEBUG_REFCLK_OVRD_ENABLE             (1 << 16)
#define  PCIE_HARD_DEBUG_REFCLK_OVRD_OUT                (1 << 20)
#define  PCIE_HARD_DEBUG_L1SS_ENABLE                    (1 << 21)
#define  PCIE_HARD_DEBUG_SERDES_IDDQ                    (1 << 27)
#define  PCIE_HARD_DEBUG_CLKREQ_MASK \
    (PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE | PCIE_HARD_DEBUG_REFCLK_OVRD_ENABLE | \
     PCIE_HARD_DEBUG_REFCLK_OVRD_OUT | PCIE_HARD_DEBUG_L1SS_ENABLE)

#define PCIE_EXT_CFG_DATA                               0x8000
#define PCIE_EXT_CFG_INDEX                              0x9000

/* External config index encoding, also the tag layout used internally. */
#define EXT_CFG_ADDR(bus, dev, func)    (((bus) << 20) | ((dev) << 15) | ((func) << 12))

/* Configuration header offsets used on the root port itself. */
#define PCI_CMD                         0x04
#define  PCI_CMD_MEMORY                 (1 << 1)
#define  PCI_CMD_MASTER                 (1 << 2)
#define PCI_BAR0                        0x10
#define PCI_BUSNUM                      0x18
#define PCI_MEMBASE                     0x20
#define PCI_ROMBASE                     0x30
#define PCI_INTLINE                     0x3c

/* GIC interrupt IDs start above the 32 SGIs and PPIs. */
#define GIC_SPI_BASE                    32

/*
 * The MSI-X Interrupt Peripheral. Each host bridge has one, named by its
 * msi-parent; a device signals by writing the message number to the target
 * address, and the block raises one GIC SPI per message - so there is no
 * status register to demultiplex, unlike the 2711 bridge's own MSI matcher.
 *
 * Register layout from OpenBSD's bcm2712_mip.c (ISC, same author as the
 * bridge driver this one follows).
 */
#define MIP_INT_CFGL_HOST               0x20
#define MIP_INT_CFGH_HOST               0x30
#define MIP_INT_MASKL_HOST              0x40
#define MIP_INT_MASKH_HOST              0x50
#define MIP_INT_MASKL_VPU               0x60
#define MIP_INT_MASKH_VPU               0x70

#define MIP_REG_SIZE                    0xc0
#define MIP_MAX_VECTORS                 32      /* what one bitmap word holds */

/* Capabilities we hand out vectors through. */
#define PCI_CAP_PTR                     0x34
#define PCI_CAP_ID_MSI                  0x05
#define PCI_CAP_ID_MSIX                 0x11

#define PCI_MSI_CTRL_ENABLE             (1 << 16)   /* bit 0 of the 16 bit control */
#define PCI_MSI_CTRL_64BIT              (1 << 23)   /* bit 7 */
#define PCI_MSI_CTRL_MME_MASK           (7 << 20)   /* bits 6:4 */
#define PCI_MSI_ADDRESSLO               0x04
#define PCI_MSI_ADDRESSHI               0x08
#define PCI_MSI_DATA32                  0x08
#define PCI_MSI_DATA64                  0x0c

#define PCI_MSIX_CTRL_ENABLE            (1U << 31)  /* bit 15 */
#define PCI_MSIX_CTRL_SIZE_MASK         0x07ff0000  /* table size less one */
#define PCI_MSIX_TABLE                  0x04
#define  PCI_MSIX_TABLE_BIR_MASK        0x7
#define  PCI_MSIX_TABLE_OFF_MASK        (~0x7U)
#define PCI_MSIX_ENTRY_SIZE             16

/*
 * Bring-up tracing. None of this can be exercised under emulation - QEMU's
 * raspi models have no PCIe - so the first run on real hardware is also the
 * first test. Leave this on until a controller has been seen to enumerate.
 */
#define PCIE_BRINGUP                    1

#if PCIE_BRINGUP
#define BRINGUP(x)                      x
#else
#define BRINGUP(x)
#endif

/*
 * A translation window, from either "ranges" (outbound) or "dma-ranges"
 * (inbound). Both properties use the same cell layout on this bridge.
 */
struct pcie_range
{
    uint32_t        flags;
    uint64_t        pci_base;
    uint64_t        cpu_base;
    uint64_t        size;
};

/* Each bridge publishes two of each; leave room for a tree that grows one. */
#define PCIE_MAX_RANGES                 4

/* Which bridge a driver object drives, handed to it via aHidd_DriverData. */
struct pcie_bridge
{
    const char         *node;           /* device tree path */
    uint64_t            reg_base;
    const char         *name;
};

/*
 * Per bridge state, the instance data of one driver object.
 *
 * Everything here is read back out of the registers once the bridge is
 * running, whether we programmed it or the firmware did - see BridgeAdopt().
 * Nothing downstream needs to know which of the two happened.
 */
struct PCIBcm2712Data
{
    const struct pcie_bridge *bridge;

    volatile uint8_t   *regs;
    volatile uint8_t   *rescal;
    volatile uint8_t   *reset;

    /* Which bit of the reset controller holds this bridge, from "resets". */
    uint32_t            reset_cell;

    struct pcie_range   ranges[PCIE_MAX_RANGES];
    uint32_t            nranges;
    struct pcie_range   dmaranges[PCIE_MAX_RANGES];
    uint32_t            ndmaranges;

    /* The MEM32 outbound window, which is where endpoint BARs are placed. */
    uint64_t            mem_pci_base;
    uint64_t            mem_cpu_base;
    uint64_t            mem_size;
    uint64_t            mem_next;       /* bump allocator over the window */

    /* The bridge's secondary bus, read from its own config header rather
       than assumed: on a firmware initialised bridge it is already set. */
    UBYTE               secondary_bus;

    /*
     * The GIC INTID the bridge raises for the endpoint's INTA, from the
     * node's interrupt-map. Config space carries no usable interrupt line
     * on this bridge, so this is substituted when one is read.
     */
    uint32_t            intx_irq;

    /*
     * This bridge's MSI block. msi_first_intid already carries the message
     * offset, so message n of ours raises msi_first_intid + n; msi_used is
     * one bit per message handed out.
     */
    volatile uint8_t   *mip;
    uint64_t            msi_target;     /* the PCI address a device writes */
    uint32_t            msi_first_intid;
    uint32_t            msi_count;
    uint32_t            msi_used;

    BOOL                link_up;
    BOOL                trained;        /* we brought it up, firmware had not */
};

/*
 * Per device vector bookkeeping. firstVector holds the first message plus
 * one, so a freshly created device reads as holding none.
 */
struct PCIBcm2712DevData
{
    ULONG               firstVector;
    ULONG               nvectors;
    ULONG               cap;            /* config offset of the capability */
    BOOL                msix;
};

/* Per module state. Only what genuinely has one instance lives here. */
struct pci_staticdata
{
    OOP_Class          *driverClass;
    OOP_Class          *deviceClass;

    OOP_AttrBase        hiddPCIDriverAB;
    OOP_AttrBase        hiddPCIDeviceAB;
    OOP_AttrBase        hiddAB;

    struct Library     *kernelBase;
};

struct pcibcm2712base
{
    struct Library        lib;
    struct pci_staticdata psd;
};

#define PSD(cl) (&((struct pcibcm2712base *)(cl)->UserData)->psd)

/* Shared between the two source files. */
ULONG PCIE_ReadConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg);
void  PCIE_WriteConfig(struct PCIBcm2712Data *data, UBYTE bus, UBYTE dev, UBYTE sub, UWORD reg, ULONG val);
BOOL  PCIE_BridgeSetup(struct pci_staticdata *psd, struct PCIBcm2712Data *data);

#endif /* PCIE_BCM2712_H */
