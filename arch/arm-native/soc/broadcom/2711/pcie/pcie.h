/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The Broadcom register map and the bring-up sequence below follow OpenBSD's
    sys/dev/fdt/bcm2711_pcie.c. The BCM2711 PCIe block is not covered by the
    published BCM2711 ARM Peripherals documentation, so that driver is the
    reference this one is written against:

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
*/

#ifndef PCIE_BCM2711_H
#define PCIE_BCM2711_H

#include <inttypes.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include LC_LIBDEFS_FILE

/*
 * BCM2711 PCIe host bridge (single root port, gen2 x1). The register
 * block sits outside the main peripheral window, at a fixed place in
 * the SoC address map.
 */
#define BCM2711_PCIE_REG_BASE           0xFD500000UL
#define BCM2711_PCIE_REG_SIZE           0x9310

/* CPU-side outbound window (fixed in the SoC address map) and the PCI
 * bus address programmed into it. */
#define BCM2711_PCIE_CPU_WIN            0x600000000ULL
#define BCM2711_PCIE_PCI_WIN            0xC0000000UL
#define BCM2711_PCIE_WIN_SIZE           0x04000000UL    /* 64MB */

/* The root complex configuration header is memory mapped at offset 0;
 * external configuration space is reached through an indexed window. */
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1         0x0188
/* Byte order of the inbound window. OpenBSD leaves this at its reset value
   and never defines these bits; the field is documented here as hardware. */
#define  PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_MASK            (0x3 << 2)
#define  PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_LITTLE          (0x0 << 2)
#define PCIE_RC_CFG_PRIV1_ID_VAL3                       0x043c
#define  PCIE_RC_CFG_PRIV1_ID_VAL3_CLASS_MASK           (0xffffff << 0)
#define PCIE_RC_CFG_PRIV1_LINK_CAP                      0x04dc
#define  PCIE_RC_CFG_PRIV1_LINK_CAP_ASPM_SUPPORT_MASK   (0x3 << 10)

#define PCIE_MISC_MISC_CTRL                             0x4008
#define  PCIE_MISC_MISC_CTRL_SCB_ACCESS_EN              (1 << 12)
#define  PCIE_MISC_MISC_CTRL_CFG_READ_UR_MODE           (1 << 13)
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_MASK        (0x3 << 20)
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_128         (0x0 << 20)
#define  PCIE_MISC_MISC_CTRL_SCB0_SIZE_MASK             (0x1fU << 27)
#define  PCIE_MISC_MISC_CTRL_SCB0_SIZE_SHIFT            27
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO                0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI                0x4010
#define PCIE_MISC_RC_BAR1_CONFIG_LO                     0x402c
#define  PCIE_MISC_RC_BAR1_CONFIG_SIZE_MASK             (0x1f << 0)
#define PCIE_MISC_RC_BAR1_CONFIG_HI                     0x4030
#define PCIE_MISC_RC_BAR2_CONFIG_LO                     0x4034
#define PCIE_MISC_RC_BAR2_CONFIG_HI                     0x4038
#define PCIE_MISC_RC_BAR3_CONFIG_LO                     0x403c
#define PCIE_MISC_PCIE_CTRL                             0x4064
#define PCIE_MISC_PCIE_STATUS                           0x4068
#define  PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP           (1 << 4)
#define  PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE           (1 << 5)
#define PCIE_MISC_REVISION                              0x406c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT        0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI           0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI          0x4084
#define PCIE_HARD_DEBUG                                 0x4204
#define  PCIE_HARD_DEBUG_CLKREQ_DEBUG_ENABLE            (1 << 1)
#define  PCIE_HARD_DEBUG_SERDES_IDDQ                    (1 << 27)

/*
 * The bridge's own MSI machinery. A bus master signals by writing a data
 * word to the address in MSI_BAR_CONFIG; the bridge catches the write,
 * raises the corresponding bit in INTR2_INT_STATUS and asserts its "msi"
 * interrupt output. DATA_CONFIG holds the pattern the data word is matched
 * against, with the low bits left free to select one of 32 vectors.
 */
#define PCIE_MISC_MSI_BAR_CONFIG_LO                     0x4044
#define  PCIE_MISC_MSI_BAR_CONFIG_LO_EN                 (1 << 0)
#define PCIE_MISC_MSI_BAR_CONFIG_HI                     0x4048
#define PCIE_MISC_MSI_DATA_CONFIG                       0x404c
#define  PCIE_MISC_MSI_DATA_CONFIG_32                   0xfff86540

#define PCIE_MSI_INTR2_INT_STATUS                       0x4500
#define PCIE_MSI_INTR2_INT_CLR                          0x4508
#define PCIE_MSI_INTR2_INT_MASK_SET                     0x4510
#define PCIE_MSI_INTR2_INT_MASK_CLR                     0x4514

#define BCM2711_MSI_VECTORS                             32

/* The data word carries the match pattern with the vector in the free low
   bits, so vector n signals as (pattern | n). */
#define BCM2711_MSI_DATA(vec) \
    ((PCIE_MISC_MSI_DATA_CONFIG_32 & 0xffff) | (vec))

/*
 * Where a bus master aims its MSI writes. Any address the inbound window
 * does not claim will do - the match is exact and nothing is stored there -
 * so it sits at the very top of what the matcher can hold, far above the
 * 4GB window this board uses.
 */
#define BCM2711_MSI_TARGET                              0xffffffffcULL

/* Configuration header command register */
#define PCI_CMD                         0x04

/* GIC interrupt IDs start above the 32 SGIs and PPIs; the device tree
   numbers shared peripheral interrupts from zero. */
#define GIC_SPI_BASE                    32

/*
 * Bring-up tracing. Nothing here can be exercised under emulation - QEMU's
 * raspi4b has no PCIe - so the first run on real hardware is also the first
 * test. Leave this on until a controller has been seen to enumerate.
 */
#define PCIE_BRINGUP                    1

#if PCIE_BRINGUP
#define BRINGUP(x)                      x
#else
#define BRINGUP(x)
#endif

/* The attached USB controller reports the firmware it is running here;
   zero means it has none and will execute no command. */
#define VL805_CFG_FWVERSION             0x50
#define PCI_CMD_MEMORY                  (1 << 1)
#define PCI_CMD_MASTER                  (1 << 2)

#define PCIE_EXT_CFG_DATA                               0x8000
#define PCIE_EXT_CFG_INDEX                              0x9000
#define PCIE_RGR1_SW_INIT_1                             0x9210
#define  PCIE_RGR1_SW_INIT_1_PERST                      (1 << 0)
#define  PCIE_RGR1_SW_INIT_1_INIT                       (1 << 1)

/* Inbound window and SCB size fields both hold log2 of the size, less 15. */
#define RC_BAR_SIZE(exp)                ((exp) - 15)
#define MISC_CTRL_SCB0_SIZE(exp)        (((exp) - 15) << PCIE_MISC_MISC_CTRL_SCB0_SIZE_SHIFT)

/* Fallback for a device tree that does not describe dma-ranges: the Pi 4
   lets a bus master reach the low 3GB, and the window covering it is the
   next power of two up. DMARangesFromDT() reads the real figure. */
#define BCM2711_DMA_EXP                 32

/* External config index encoding */
#define EXT_CFG_ADDR(bus, dev, func)    (((bus) << 20) | ((dev) << 15) | ((func) << 12))

/* The bridge's "msi" interrupt output as a GIC INTID (SPI 148).
   MSIIrqFromDT() reads the real figure; this covers a tree without one. */
#define BCM2711_PCIE_MSI                180

/* Uncached DMA memory allocator: page-granular bitmap over the block the
 * bootstrap reserved. xHCI controllers may use a 64KB page size and ask
 * for scratchpad areas of a megabyte or more, so the block is sized in
 * tens of megabytes and a single allocation is not artificially capped. */

struct pci_staticdata {
    OOP_AttrBase    hiddPCIDriverAB;
    OOP_AttrBase    hiddPCIDeviceAB;
    OOP_AttrBase    hiddAB;
    OOP_Class       *driverClass;
    APTR            kernelBase;         /* kernel.resource, for KrnMapGlobal() */
    OOP_Class       *deviceClass;

    volatile uint8_t *regs;
    BOOL            preinitialised;     /* firmware left the link trained */
    BOOL            fw_loaded;          /* endpoint firmware load already handled */

    /*
     * What a bus master must add to a system address to reach it. The
     * firmware picks this to suit the memory fitted and publishes it in
     * the device tree, so it is read at run time rather than assumed.
     */
    uint64_t        dma_offset;

    /*
     * log2 of the inbound window, likewise from dma-ranges: the window has
     * to be a power of two, so it is the smallest one covering what the
     * bus may reach.
     */
    uint32_t        dma_exp;

    /* The bridge's "msi" output as a GIC INTID, and the handler on it.
       A zero handle means MSI delivery is not available. */
    uint32_t        msi_irq;
    APTR            msi_handle;
    uint32_t        msi_used;           /* one bit per handed-out vector */

};

/*
 * Per-device side of the vector bookkeeping. msiVector holds the assigned
 * vector plus one, so a freshly created (zeroed) device reads as "none".
 */
struct PCIBcm2711DevData {
    ULONG           msiVector;
    ULONG           msiCap;             /* config space offset of the MSI capability */
};

struct pcibcm2711base {
    struct Library          LibNode;
    struct pci_staticdata   psd;
};

#define BASE(lib) ((struct pcibcm2711base *)(lib))
#define PSD(cl)   (&((struct pcibcm2711base *)cl->UserData)->psd)

/* One-shot endpoint firmware load, called when its driver enables it. */
void EnsureEndpointFirmware(struct pci_staticdata *psd);

#endif
