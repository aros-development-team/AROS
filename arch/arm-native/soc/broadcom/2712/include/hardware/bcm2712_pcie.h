/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: BCM2712 PCIe host bridge, MSI block and their reset blocks.

    The register map is not covered by any published Broadcom document. It
    follows OpenBSD's sys/dev/fdt/bcm2711_pcie.c and sys/arch/arm64/dev/
    bcm2712_mip.c, which drive this hardware:

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

    The inbound window size encoding and the UBUS remap registers are also
    described in worproject/edk2-platforms, branch rpi5-dev, file
    Silicon/Broadcom/Bcm27xx/Library/Bcm2712PciHostBridgeLib (BSD-2-Clause
    with patent grant).
*/

#ifndef HARDWARE_BCM2712_PCIE_H
#define HARDWARE_BCM2712_PCIE_H

/*
 * BCM2712 has three host bridges, each a separate root complex with its own
 * copy of this register block, its own reset bit and its own MSI block. They
 * share only the rescal and reset blocks below. The block is the size the
 * device tree declares; the indexed configuration window at the top of it is
 * the last thing in it, and there is no ECAM anywhere.
 */
#define BCM2712_PCIE_REG_SIZE                           0x9310

/* The root complex's own configuration header is at offset 0. */
#define PCIE_RC_CFG_VENDOR_VENDOR_SPECIFIC_REG1         0x0188
#define  PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_MASK            (0x3 << 2)
#define  PCIE_RC_CFG_VENDOR_ENDIAN_BAR2_LITTLE          (0x0 << 2)
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
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_128         (0x0 << 20)
#define  PCIE_MISC_MISC_CTRL_MAX_BURST_SIZE_512         (0x2 << 20)

#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LO                0x400c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_HI                0x4010

/*
 * Inbound windows. BAR1 through BAR3 are a contiguous group eight bytes
 * apart, so a loop over them works - but BAR4 is not part of it, which is
 * why every one is named here.
 */
#define PCIE_MISC_RC_BAR1_CONFIG_LO                     0x402c
#define PCIE_MISC_RC_BAR1_CONFIG_HI                     0x4030
#define PCIE_MISC_RC_BAR2_CONFIG_LO                     0x4034
#define PCIE_MISC_RC_BAR2_CONFIG_HI                     0x4038
#define PCIE_MISC_RC_BAR3_CONFIG_LO                     0x403c
#define PCIE_MISC_RC_BAR3_CONFIG_HI                     0x4040
#define PCIE_MISC_RC_BAR4_CONFIG_LO                     0x40d4
#define PCIE_MISC_RC_BAR4_CONFIG_HI                     0x40d8
#define  PCIE_MISC_RC_BAR_CONFIG_SIZE_MASK              (0x1f << 0)

/* The bridge's own MSI matcher, which the 2712 does not use - its messages
   go to the separate MSI block below. Kept because the register exists. */
#define PCIE_MISC_MSI_BAR_CONFIG_LO                     0x4044
#define PCIE_MISC_MSI_BAR_CONFIG_HI                     0x4048

#define PCIE_MISC_PCIE_CTRL                             0x4064
#define  PCIE_MISC_PCIE_CTRL_PCIE_PERSTB                (1 << 2)
#define PCIE_MISC_PCIE_STATUS                           0x4068
#define  PCIE_MISC_PCIE_STATUS_PCIE_PHYLINKUP           (1 << 4)
#define  PCIE_MISC_PCIE_STATUS_PCIE_DL_ACTIVE           (1 << 5)
#define PCIE_MISC_REVISION                              0x406c
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_LIMIT        0x4070
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_BASE_HI           0x4080
#define PCIE_MISC_CPU_2_PCIE_MEM_WIN0_LIMIT_HI          0x4084

/*
 * The CPU side of each inbound window, with its own enable bit. The 2711
 * has no UBUS: there an inbound window is implicitly anchored at CPU
 * address zero, so RC_BAR alone routes it. Here it is a register pair, and
 * without it the window has a bus address but no CPU side to land on.
 */
#define PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_LO             0x40ac
#define PCIE_MISC_UBUS_BAR1_CONFIG_REMAP_HI             0x40b0
#define PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_LO             0x40b4
#define PCIE_MISC_UBUS_BAR2_CONFIG_REMAP_HI             0x40b8
#define PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_LO             0x410c
#define PCIE_MISC_UBUS_BAR4_CONFIG_REMAP_HI             0x4110
#define  PCIE_MISC_UBUS_REMAP_EN                        (1 << 0)
#define  PCIE_MISC_UBUS_REMAP_LO_MASK                   0xfffff000
#define  PCIE_MISC_UBUS_REMAP_HI_MASK                   0xff

#define PCIE_MISC_AXI_READ_ERROR_DATA                   0x4170

/* Not 0x4204 as on the 2711 - the block moved on this SoC. */
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

/* External config index encoding, also the tag layout drivers use. */
#define EXT_CFG_ADDR(bus, dev, func)    (((bus) << 20) | ((dev) << 15) | ((func) << 12))

/*
 * An inbound window's size field is log2 less 15 from 64KB up, and a
 * separate encoding below that. The small branch is not a curiosity: the
 * MSI doorbell window is 4KB.
 */
#define PCIE_RC_BAR_SIZE_4K                             0x1c

/*
 * Shared by all three bridges. Neither block is described by anything but
 * the device tree; these are the addresses after /soc's ranges translation.
 */
#define BCM2712_RESCAL_BASE             0x1000119500ULL
#define BCM2712_RESCAL_SIZE             0x10
#define BCM2712_RESET_BASE              0x1001504318ULL
#define BCM2712_RESET_SIZE              0x30

/* Rescal is self-deasserting, so only the assert side is ever used. */
#define RESCAL_START                    0x00
#define  RESCAL_START_BIT               (1 << 0)
#define RESCAL_STATUS                   0x08
#define  RESCAL_STATUS_BIT              (1 << 0)

/* brcmstb reset: banks of 32 bits, set and clear registers 24 bytes apart. */
#define RESET_SW_INIT_SET(bank)         (0x00 + (bank) * 24)
#define RESET_SW_INIT_CLR(bank)         (0x04 + (bank) * 24)

/*
 * The MSI-X Interrupt Peripheral. Each host bridge has one, named by its
 * msi-parent; a device signals by writing the message number to the target
 * address, and the block raises one GIC SPI per message - so there is no
 * status register to demultiplex, unlike the 2711 bridge's own matcher.
 */
#define MIP_INT_RAISE                   0x00
#define MIP_INT_CLEAR                   0x10
#define MIP_INT_CFGL_HOST               0x20
#define MIP_INT_CFGH_HOST               0x30
#define MIP_INT_MASKL_HOST              0x40
#define MIP_INT_MASKH_HOST              0x50
#define MIP_INT_MASKL_VPU               0x60
#define MIP_INT_MASKH_VPU               0x70
#define MIP_INT_STATUSL_HOST            0x80
#define MIP_INT_STATUSH_HOST            0x90

#define MIP_REG_SIZE                    0xc0

/* GIC interrupt IDs start above the 32 SGIs and PPIs; the device tree
   numbers shared peripheral interrupts from zero. */
#define GIC_SPI_BASE                    32

#endif /* HARDWARE_BCM2712_PCIE_H */
