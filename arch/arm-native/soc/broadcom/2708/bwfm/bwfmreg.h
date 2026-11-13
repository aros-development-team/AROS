/*
 * Copyright (c) 2010-2016 Broadcom Corporation
 * Copyright (c) 2016,2017 Patrick Wildt <patrick@blueri.se>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Subset of OpenBSD sys/dev/ic/bwfmreg.h ported for AROS.
 */

#ifndef BWFMREG_H
#define BWFMREG_H

/* ChipCommon core registers (backplane base) */
#define BWFM_CHIP_BASE                          0x18000000
#define BWFM_CHIP_REG_CHIPID                    0x00000000
#define  BWFM_CHIP_CHIPID_ID(x)                     (((x) >> 0) & 0xffff)
#define  BWFM_CHIP_CHIPID_REV(x)                    (((x) >> 16) & 0xf)
#define  BWFM_CHIP_CHIPID_PKG(x)                    (((x) >> 20) & 0xf)
#define  BWFM_CHIP_CHIPID_CC(x)                     (((x) >> 24) & 0xf)
#define  BWFM_CHIP_CHIPID_TYPE(x)                   (((x) >> 28) & 0xf)
#define  BWFM_CHIP_CHIPID_TYPE_SOCI_SB              0
#define  BWFM_CHIP_CHIPID_TYPE_SOCI_AI              1
#define BWFM_CHIP_REG_CAPABILITIES              0x00000004
#define  BWFM_CHIP_REG_CAPABILITIES_PMU             0x10000000
#define BWFM_CHIP_REG_CAPABILITIES_EXT          0x000000AC
#define  BWFM_CHIP_REG_CAPABILITIES_EXT_AOB_PRESENT 0x00000040
#define BWFM_CHIP_REG_EROMPTR                   0x000000FC
#define BWFM_CHIP_REG_PMUCONTROL                0x00000600
#define  BWFM_CHIP_REG_PMUCONTROL_RES_MASK          0x00006000
#define  BWFM_CHIP_REG_PMUCONTROL_RES_SHIFT         13
#define  BWFM_CHIP_REG_PMUCONTROL_RES_RELOAD        0x2
#define BWFM_CHIP_REG_PMUCAPABILITIES           0x00000604
#define  BWFM_CHIP_REG_PMUCAPABILITIES_REV_MASK     0x000000ff

/* AI (silicon-backplane agent) wrapper registers */
#define BWFM_AGENT_IOCTL                        0x0408
#define  BWFM_AGENT_IOCTL_CLK                       0x0001
#define  BWFM_AGENT_IOCTL_FGC                       0x0002
#define  BWFM_AGENT_IOCTL_CORE_BITS                 0x3FFC
#define  BWFM_AGENT_IOCTL_ARMCR4_CPUHALT            0x0020
#define BWFM_AGENT_RESET_CTL                    0x0800
#define  BWFM_AGENT_RESET_CTL_RESET                 0x0001

/* CPU-core IOCTL bits */
#define BWFM_AGENT_IOCTL_ARMCR4_CPUHALT         0x0020
#define BWFM_AGENT_D11_IOCTL_PHYCLOCKEN         0x0004
#define BWFM_AGENT_D11_IOCTL_PHYRESET           0x0008

/* ARM CR4 core (TCM RAM) registers */
#define BWFM_ARMCR4_CAP                         0x0004
#define  BWFM_ARMCR4_CAP_TCBANB_MASK                0xf
#define  BWFM_ARMCR4_CAP_TCBANB_SHIFT               0
#define  BWFM_ARMCR4_CAP_TCBBNB_MASK                0xf0
#define  BWFM_ARMCR4_CAP_TCBBNB_SHIFT               4
#define BWFM_ARMCR4_BANKIDX                     0x0040
#define BWFM_ARMCR4_BANKINFO                    0x0044
#define  BWFM_ARMCR4_BANKINFO_BSZ_MASK              0x7f
#define  BWFM_ARMCR4_BANKINFO_BLK_1K_MASK           0x200

/* SOCRAM / SYSMEM core registers */
#define BWFM_SOCRAM_COREINFO                    0x0000
#define  BWFM_SOCRAM_COREINFO_SRBSZ_BASE            14
#define  BWFM_SOCRAM_COREINFO_SRBSZ_MASK            0xf
#define  BWFM_SOCRAM_COREINFO_SRNB_MASK             0xf0
#define  BWFM_SOCRAM_COREINFO_SRNB_SHIFT            4
#define  BWFM_SOCRAM_COREINFO_LSS_MASK              0xf00000
#define  BWFM_SOCRAM_COREINFO_LSS_SHIFT             20
#define BWFM_SOCRAM_BANKIDX                     0x0010
#define  BWFM_SOCRAM_BANKIDX_MEMTYPE_RAM            0
#define  BWFM_SOCRAM_BANKIDX_MEMTYPE_SHIFT          8
#define BWFM_SOCRAM_BANKINFO                    0x0040
#define  BWFM_SOCRAM_BANKINFO_SZBASE                8192
#define  BWFM_SOCRAM_BANKINFO_SZMASK                0x7f
#define  BWFM_SOCRAM_BANKINFO_RETNTRAM_MASK         0x10000
#define BWFM_SOCRAM_BANKPDA                     0x0044

/* DMP (EROM) descriptor format */
#define BWFM_DMP_DESC_MASK                      0x0000000F
#define BWFM_DMP_DESC_COMPONENT                 0x00000001
#define BWFM_DMP_DESC_MASTER_PORT               0x00000003
#define BWFM_DMP_DESC_ADDRESS                   0x00000005
#define BWFM_DMP_DESC_ADDRSIZE_GT32             0x00000008
#define BWFM_DMP_DESC_EOT                       0x0000000F
#define BWFM_DMP_COMP_PARTNUM                   0x000FFF00
#define BWFM_DMP_COMP_PARTNUM_S                 8
#define BWFM_DMP_COMP_REVISION                  0xFF000000
#define BWFM_DMP_COMP_REVISION_S                24
#define BWFM_DMP_COMP_NUM_SWRAP                 0x00F80000
#define BWFM_DMP_COMP_NUM_SWRAP_S               19
#define BWFM_DMP_COMP_NUM_MWRAP                 0x0007C000
#define BWFM_DMP_COMP_NUM_MWRAP_S               14
#define BWFM_DMP_MASTER_PORT_NUM                0x000000F0
#define BWFM_DMP_MASTER_PORT_NUM_S              4
#define BWFM_DMP_SLAVE_ADDR_BASE                0xFFFFF000
#define BWFM_DMP_SLAVE_TYPE                     0x000000C0
#define BWFM_DMP_SLAVE_TYPE_S                   6
#define  BWFM_DMP_SLAVE_TYPE_SLAVE                  0
#define  BWFM_DMP_SLAVE_TYPE_SWRAP                  2
#define  BWFM_DMP_SLAVE_TYPE_MWRAP                  3
#define BWFM_DMP_SLAVE_SIZE_TYPE                0x00000030
#define BWFM_DMP_SLAVE_SIZE_TYPE_S              4
#define  BWFM_DMP_SLAVE_SIZE_4K                     0
#define  BWFM_DMP_SLAVE_SIZE_8K                     1
#define  BWFM_DMP_SLAVE_SIZE_DESC                   3

/* Core IDs (DMP/EROM) */
#define BWFM_AGENT_CORE_CHIPCOMMON              0x800
#define BWFM_AGENT_INTERNAL_MEM                 0x80E
#define BWFM_AGENT_CORE_80211                   0x812
#define BWFM_AGENT_CORE_PMU                     0x827
#define BWFM_AGENT_CORE_SDIO_DEV                0x829
#define BWFM_AGENT_CORE_ARM_CM3                 0x82A
#define BWFM_AGENT_CORE_ARM_CR4                 0x83E
#define BWFM_AGENT_CORE_GCI                     0x840
#define BWFM_AGENT_CORE_ARM_CA7                 0x847
#define BWFM_AGENT_SYS_MEM                      0x849

/* Chipcommon chip IDs (selected; Pi 3 = 43430, Pi 3B+/4 = 0x4345) */
#define BRCM_CC_43430_CHIP_ID                   43430
#define BRCM_CC_4345_CHIP_ID                    0x4345
#define BRCM_CC_4339_CHIP_ID                    0x4339
#define BRCM_CC_43340_CHIP_ID                   43340
#define BRCM_CC_43362_CHIP_ID                   43362

#endif /* BWFMREG_H */
