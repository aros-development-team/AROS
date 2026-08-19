/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 (Raspberry Pi 5) PCIe Host Bridge Header
          Supports PCIe RC0 (16-pin M.2 NVMe HAT connector) and PCIe RC1 (RP1).
*/

#ifndef PCIE_BCM2712_H
#define PCIE_BCM2712_H

#include <inttypes.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <oop/oop.h>

#include LC_LIBDEFS_FILE

/*
 * BCM2712 PCIe host bridge register bases in 40-bit ARM physical space.
 * RC0: External 16-pin FPC connector for M.2 NVMe HATs (Gen 2/3 x1).
 * RC1: Internal RP1 Southbridge connection (Gen 2 x4).
 */
#define BCM2712_PCIE0_REG_BASE          0x1000100000ULL
#define BCM2712_PCIE0_REG_SIZE          0x10000
#define BCM2712_PCIE0_ECAM_BASE         0x1000000000ULL
#define BCM2712_PCIE0_ECAM_SIZE         0x10000000ULL   /* 256MB standard ECAM space */

#define BCM2712_PCIE1_REG_BASE          0x1000110000ULL
#define BCM2712_PCIE1_REG_SIZE          0x10000
#define BCM2712_PCIE1_ECAM_BASE         0x1000020000ULL

/* Outbound 32-bit/64-bit memory window */
#define BCM2712_PCIE_CPU_WIN            0x1800000000ULL
#define BCM2712_PCIE_PCI_WIN            0xC0000000UL
#define BCM2712_PCIE_WIN_SIZE           0x40000000UL    /* 1GB window */

/* Inbound DMA mapping defaults */
#define BCM2712_DMA_EXP                 36              /* 64GB max RAM window */

/* PCIe Controller Register Offsets */
#define PCIE2712_MISC_CTRL              0x4008
#define  PCIE2712_MISC_CTRL_SCB_EN      (1 << 12)
#define  PCIE2712_MISC_CTRL_CFG_UR_MODE (1 << 13)
#define PCIE2712_STATUS                 0x4068
#define  PCIE2712_STATUS_PHYLINKUP      (1 << 4)
#define  PCIE2712_STATUS_DL_ACTIVE      (1 << 5)
#define PCIE2712_REVISION               0x406c

#define PCIE2712_EXT_CFG_INDEX          0x9000
#define PCIE2712_EXT_CFG_DATA           0x8000

/* Standard AROS OOP Driver Static Data */
struct pci_staticdata
{
    OOP_Class          *driverClass;
    OOP_Class          *deviceClass;
    OOP_Class          *busClass;

    OOP_Object         *driverObject;
    OOP_Object         *busObject;

    OOP_AttrBase        hiddPCIDriverAB;
    OOP_AttrBase        hiddPCIDeviceAB;
    OOP_AttrBase        hiddAB;

    struct Library     *kernelBase;
    struct Library     *openFirmwareBase;

    volatile uint8_t   *regs;
    volatile uint8_t   *ecam;
    uint64_t            dma_offset;
    uint32_t            dma_exp;
    uint32_t            msi_irq;
    BOOL                link_up;
};

struct pcibcm2712base
{
    struct Library        lib;
    struct pci_staticdata psd;
};

struct PCIBcm2712DevData
{
    struct pci_staticdata *psd;
    uint32_t               devfn;
};

#define PSD(cl) ((struct pci_staticdata *)(cl)->UserData)

#endif /* PCIE_BCM2712_H */
