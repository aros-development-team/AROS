/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Raspberry Pi I2S Hardware Controller and DMA routines
*/

#include <config.h>
#include <exec/types.h>
#include <aros/macros.h>
#include "rpii2s-hwaccess.h"

static void udelay(IPTR pb, ULONG us) {
    volatile ULONG *clo = (volatile ULONG *)(pb + 0x003004);
    ULONG start = AROS_LE2LONG(*clo);
    while ((AROS_LE2LONG(*clo) - start) < us) ;
}

void i2s_hw_init(struct RPiI2SData *dd) {
    IPTR base = dd->i2s_base;
    /* Enable I2S */
    i2s_wr(base, I2S_CS_A, I2S_EN | I2S_STBY);
    udelay(dd->periiobase, 100);
    /* Clear FIFOs */
    i2s_wr(base, I2S_CS_A, I2S_EN | I2S_STBY | I2S_TXCLR | I2S_RXCLR);
    udelay(dd->periiobase, 100);
    /* I2S mode: 64 BCLK per frame, 32 per channel, master */
    i2s_wr(base, I2S_MODE_A, I2S_FLEN(63) | I2S_FSLEN(32) | I2S_CLKI | I2S_FSI);
    /* TX channel config: 16-bit, ch1 at pos 1, ch2 at pos 33 */
    i2s_wr(base, I2S_TXC_A,
        I2S_CH1(I2S_CHEN | I2S_CHPOS(1) | I2S_CHWID(8)) |
        I2S_CH2(I2S_CHEN | I2S_CHPOS(33) | I2S_CHWID(8)));
    /* DMA thresholds */
    i2s_wr(base, I2S_DREQ_A, (0x10 << 24) | (0x30 << 16) | (0x30 << 8) | 0x20);
    /* Enable DMA + TX + frame packed (16-bit stereo in 32-bit words) */
    i2s_wr(base, I2S_CS_A, I2S_EN | I2S_STBY | I2S_DMAEN | I2S_TXON | I2S_TXTHR(1));
    /* Set frame packed mode */
    i2s_wr(base, I2S_MODE_A, i2s_rd(base, I2S_MODE_A) | I2S_FTXP);
}

void i2s_hw_stop(struct RPiI2SData *dd) {
    i2s_wr(dd->i2s_base, I2S_CS_A, 0);
}

void dma_build_control_blocks(struct RPiI2SData *dd) {
    ULONG fifo_bus = 0x7E203004; /* I2S FIFO physical bus address */
    int i;
    for (i = 0; i < 2; i++) {
        struct DMAControlBlock *cb = dd->cb[i];
        cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ |
                 DMA_TI_SRC_INC | DMA_TI_PERMAP(DMA_DREQ_I2S_TX) | DMA_TI_NO_WIDE_BURSTS;
        cb->source_ad = gpu_bus_addr((IPTR)dd->dmabuf[i]);
        cb->dest_ad = fifo_bus;
        cb->txfr_len = dd->dmabuf_size;
        cb->stride = 0;
        cb->nextconbk = gpu_bus_addr((IPTR)dd->cb[1 - i]);
        cb->reserved[0] = cb->reserved[1] = 0;
    }
}

void dma_setup(IPTR peribase, ULONG channel, ULONG cb_bus_addr) {
    IPTR dma_base = peribase + 0x007000 + channel * 0x100;
    IPTR enable_addr = peribase + 0x007FF0;
    *(volatile ULONG *)(enable_addr) = AROS_LONG2LE(AROS_LE2LONG(*(volatile ULONG *)enable_addr) | (1 << channel));
    *(volatile ULONG *)(dma_base) = AROS_LONG2LE(DMA_CS_RESET);
    udelay(peribase, 10);
    *(volatile ULONG *)(dma_base) = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END);
    *(volatile ULONG *)(dma_base + 0x04) = AROS_LONG2LE(cb_bus_addr);
    *(volatile ULONG *)(dma_base) = AROS_LONG2LE(DMA_CS_WAIT_FOR_WRITES | DMA_CS_PANIC_PRI(15) | DMA_CS_PRI(8) | DMA_CS_ACTIVE);
}

void dma_stop(IPTR peribase, ULONG channel) {
    IPTR dma_base = peribase + 0x007000 + channel * 0x100;
    *(volatile ULONG *)(dma_base) = 0;
    udelay(peribase, 50);
    *(volatile ULONG *)(dma_base) = AROS_LONG2LE(DMA_CS_RESET);
}
