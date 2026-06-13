/*-
 * Register definitions for the BCM2835 SDHOST controller.
 * Derived from NetBSD's sys/arch/arm/broadcom/bcm2835_sdhost.c.
 *
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * AROS port additions:
 *     Copyright (C) 2026, The AROS Development Team.
 */

#ifndef SDHOST_H
#define SDHOST_H

/* Controller base address (caller supplies ARM_PERIIOBASE). */
#define SDHOST_BASE                     (ARM_PERIIOBASE + 0x202000)

/* Register offsets from SDHOST_BASE. */
#define SDCMD                           0x00
#define  SDCMD_NEW                      (1U << 15)
#define  SDCMD_FAIL                     (1U << 14)
#define  SDCMD_BUSY                     (1U << 11)
#define  SDCMD_NORESP                   (1U << 10)
#define  SDCMD_LONGRESP                 (1U <<  9)
#define  SDCMD_WRITE                    (1U <<  7)
#define  SDCMD_READ                     (1U <<  6)
#define SDARG                           0x04
#define SDTOUT                          0x08
#define  SDTOUT_DEFAULT                 0xf00000
#define SDCDIV                          0x0c
#define  SDCDIV_MASK                    0x7ff
#define SDRSP0                          0x10
#define SDRSP1                          0x14
#define SDRSP2                          0x18
#define SDRSP3                          0x1c
#define SDHSTS                          0x20
#define  SDHSTS_BUSY                    (1U << 10)
#define  SDHSTS_BLOCK                   (1U <<  9)
#define  SDHSTS_SDIO                    (1U <<  8)
#define  SDHSTS_REW_TO                  (1U <<  7)
#define  SDHSTS_CMD_TO                  (1U <<  6)
#define  SDHSTS_CRC16_E                 (1U <<  5)
#define  SDHSTS_CRC7_E                  (1U <<  4)
#define  SDHSTS_FIFO_E                  (1U <<  3)
#define  SDHSTS_DATA                    (1U <<  0)
#define SDVDD                           0x30
#define  SDVDD_POWER                    (1U <<  0)
#define SDEDM                           0x34
#define  SDEDM_RD_FIFO_SHIFT            14
#define  SDEDM_RD_FIFO_MASK             (0x1fU << SDEDM_RD_FIFO_SHIFT)
#define  SDEDM_WR_FIFO_SHIFT             9
#define  SDEDM_WR_FIFO_MASK             (0x1fU << SDEDM_WR_FIFO_SHIFT)
#define  SDEDM_FIFO_LEVEL_SHIFT          4    /* Current FIFO occupancy, 0..16 */
#define  SDEDM_FIFO_LEVEL_MASK          (0x1fU << SDEDM_FIFO_LEVEL_SHIFT)
#define SDHCFG                          0x38
#define  SDHCFG_BUSY_EN                 (1U << 10)
#define  SDHCFG_BLOCK_EN                (1U <<  8)
#define  SDHCFG_SDIO_EN                 (1U <<  5)
#define  SDHCFG_DATA_EN                 (1U <<  4)
#define  SDHCFG_SLOW                    (1U <<  3)
#define  SDHCFG_WIDE_EXT                (1U <<  2)
#define  SDHCFG_WIDE_INT                (1U <<  1)
#define  SDHCFG_REL_CMD                 (1U <<  0)
#define SDHBCT                          0x3c
#define SDDATA                          0x40
#define SDHBLC                          0x50

/* Convenience masks composed from the bit definitions above. */
#define SDHSTS_ERROR_MASK               (SDHSTS_CMD_TO  | \
                                         SDHSTS_REW_TO  | \
                                         SDHSTS_CRC16_E | \
                                         SDHSTS_CRC7_E  | \
                                         SDHSTS_FIFO_E)
#define SDHSTS_W1C_MASK                 (SDHSTS_BUSY    | \
                                         SDHSTS_BLOCK   | \
                                         SDHSTS_SDIO    | \
                                         SDHSTS_ERROR_MASK)

/*
 * AROS-side additions below — not part of the NetBSD reference.
 */

/* VideoCore interrupt line for the SDHOST controller. */
#define IRQ_VC_SDHOST                   IRQ_VC_SDIO  /* GPUIRQ1 bit 24 */

/* SDHOST timing derives from the VideoCore core clock. */
#define VCCLOCK_CORE                    4

/* DMA parameters chosen by the AROS driver. The channel itself is
 * allocated at runtime from dma.resource. */
#define SDHOST_DMA_DREQ                 13      /* BCM2835 DREQ map ID for SDHOST */

/* DMA sees peripherals through the VideoCore bus aperture. */
#define SDHOST_SDDATA_DMA_ADDR          (BCM2835_PERIBUSBASE + 0x202000 + SDDATA)

#endif /* SDHOST_H */
