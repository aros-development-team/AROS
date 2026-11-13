/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Raspberry Pi I2S Hardware Register and DMA Definitions
*/

#ifndef RPII2S_HWACCESS_H
#define RPII2S_HWACCESS_H

#include <exec/types.h>
#include <aros/macros.h>
#include "DriverData.h"

/* BCM2835 I2S register offsets */
#define I2S_CS_A    0x00
#define I2S_FIFO_A  0x04
#define I2S_MODE_A  0x08
#define I2S_RXC_A   0x0C
#define I2S_TXC_A   0x10
#define I2S_DREQ_A  0x14
#define I2S_INTEN_A 0x18
#define I2S_INTSTC_A 0x1C

/* CS_A bits */
#define I2S_EN      (1 << 0)
#define I2S_RXON    (1 << 1)
#define I2S_TXON    (1 << 2)
#define I2S_TXCLR   (1 << 3)
#define I2S_RXCLR   (1 << 4)
#define I2S_TXTHR(v) ((v) << 5)
#define I2S_RXTHR(v) ((v) << 7)
#define I2S_DMAEN   (1 << 9)
#define I2S_STBY    (1 << 25)

/* MODE_A bits */
#define I2S_CLKM    (1 << 23)  /* Clock slave */
#define I2S_FSM     (1 << 21)  /* FS slave */
#define I2S_FSI     (1 << 20)  /* FS invert */
#define I2S_CLKI    (1 << 22)  /* Clock invert */
#define I2S_FLEN(v) ((v) << 10)
#define I2S_FSLEN(v) (v)
#define I2S_FTXP    (1 << 24)  /* Frame packed TX */
#define I2S_FRXP    (1 << 25)  /* Frame packed RX */

/* TXC_A/RXC_A bits */
#define I2S_CHEN    (1 << 14)
#define I2S_CHPOS(v) ((v) << 4)
#define I2S_CHWID(v) (v)
#define I2S_CH1(v)  ((v) << 16)
#define I2S_CH2(v)  (v)

/* DMA */
#define I2S_DMA_CHANNEL 2
#define DMA_DREQ_I2S_TX 2
#define BCM_IRQ_DMA0    16

/* I2S FIFO bus address for DMA */
#define I2S_FIFO_BUS(peribase) (0x7E000000 + ((peribase & 0xFFFFFF) + I2S_FIFO_A))

/* BCM2835 I2S base offset from peribase */
#define BCM_I2S_OFFSET  0x203000

/* RP1 I2S offset (for RPi5) */
#define RP1_I2S_OFFSET  0x0A0000

/* DMA bits (same as HDMI driver) */
#define DMA_TI_INTEN          (1 << 0)
#define DMA_TI_WAIT_RESP      (1 << 3)
#define DMA_TI_DEST_DREQ      (1 << 6)
#define DMA_TI_SRC_INC        (1 << 8)
#define DMA_TI_PERMAP(x)      (((x) & 0x1F) << 16)
#define DMA_TI_NO_WIDE_BURSTS (1 << 26)
#define DMA_CS_ACTIVE          (1 << 0)
#define DMA_CS_END             (1 << 1)
#define DMA_CS_INT             (1 << 2)
#define DMA_CS_RESET           (1U << 31)
#define DMA_CS_WAIT_FOR_WRITES (1 << 28)
#define DMA_CS_PANIC_PRI(x)    (((x) & 0xF) << 20)
#define DMA_CS_PRI(x)          (((x) & 0xF) << 16)

static inline ULONG gpu_bus_addr(IPTR arm_phys) { return 0xC0000000 | (ULONG)(arm_phys & 0x3FFFFFFF); }
static inline ULONG i2s_rd(IPTR base, ULONG off) { return AROS_LE2LONG(*(volatile ULONG *)(base + off)); }
static inline void i2s_wr(IPTR base, ULONG off, ULONG val) { *(volatile ULONG *)(base + off) = AROS_LONG2LE(val); }

void i2s_hw_init(struct RPiI2SData *dd);
void i2s_hw_stop(struct RPiI2SData *dd);
void dma_setup(IPTR peribase, ULONG channel, ULONG cb_bus_addr);
void dma_stop(IPTR peribase, ULONG channel);
void dma_build_control_blocks(struct RPiI2SData *dd);
void dma_irq_handler(struct RPiI2SData *data, void *data2);
#endif
