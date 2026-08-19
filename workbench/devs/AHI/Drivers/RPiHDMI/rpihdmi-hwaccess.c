/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI MAI Audio Hardware Controller & DMA Implementation
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/kernel.h>

#include "rpihdmi-hwaccess.h"
#include "DriverData.h"

#define DMA_BASE_OFFSET         0x007000UL
#define DMA_CH_STRIDE           0x100UL

static inline void wr32(IPTR addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#endif
}

static inline uint32_t rd32(IPTR addr)
{
    uint32_t val = *(volatile uint32_t *)addr;
#if defined(__aarch64__)
    __asm__ volatile("dmb sy" ::: "memory");
#endif
    return val;
}

static inline ULONG phys_bus_addr(APTR addr)
{
    /* Physical address in low 30 bits mapped to VideoCore L2 cache coherent space */
    return ((ULONG)(uintptr_t)addr & 0x3FFFFFFFUL) | 0xC0000000UL;
}

BOOL rpihdmi_hw_init(struct RPiHDMIData *dd, ULONG rate)
{
    struct KernelBase *KernelBase = (struct KernelBase *)OpenResource("kernel.resource");
    IPTR peri_base = 0;
    IPTR dma_ch_base;
    ULONG buf_bytes;
    ULONG samples_per_buf;

    if (KernelBase) {
        peri_base = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase);
    }
    if (!peri_base) {
        peri_base = 0xFE000000UL; /* Fallback: BCM2711 low-peri */
    }

    dd->periiobase = peri_base;

    /* On Pi 4 (BCM2711), HDMI0 is at 0xFE400000; on Pi 2/3 it is at peribase + 0x200000 */
    if (peri_base == 0xFE000000UL) {
        dd->hdmi_base = 0xFE400000UL;
    } else {
        dd->hdmi_base = peri_base + 0x200000UL;
    }

    dd->samplerate = rate ? rate : 48000;
    dd->channels = 2;
    dd->dma_channel = 4; /* Standard Audio DMA Channel */

    /* Buffer size: ~20ms latency per buffer (e.g. 960 samples @ 48kHz, stereo 16-bit) */
    samples_per_buf = (dd->samplerate * 20) / 1000;
    buf_bytes = samples_per_buf * sizeof(ULONG); /* 16-bit L + 16-bit R packed into 32-bit words */

    dd->dmabuf_samples = samples_per_buf;
    dd->dmabuf_size = buf_bytes;

    /* Allocate 32-byte aligned DMA control blocks (coherent RAM) */
    dd->cb_base = AllocMem(sizeof(struct DMAControlBlock) * 2 + 32, MEMF_31BIT | MEMF_PUBLIC | MEMF_CLEAR);
    if (!dd->cb_base) {
        return FALSE;
    }

    dd->cb[0] = (struct DMAControlBlock *)(((uintptr_t)dd->cb_base + 31) & ~31UL);
    dd->cb[1] = dd->cb[0] + 1;

    /* Allocate sample buffers */
    dd->dmabuf[0] = AllocMem(buf_bytes, MEMF_31BIT | MEMF_PUBLIC | MEMF_CLEAR);
    dd->dmabuf[1] = AllocMem(buf_bytes, MEMF_31BIT | MEMF_PUBLIC | MEMF_CLEAR);
    dd->mixbuffer = AllocMem(samples_per_buf * 2 * sizeof(WORD), MEMF_PUBLIC | MEMF_CLEAR);

    if (!dd->dmabuf[0] || !dd->dmabuf[1] || !dd->mixbuffer) {
        rpihdmi_hw_cleanup(dd);
        return FALSE;
    }

    /* Initialize circular DMA Control Blocks */
    dd->cb[0]->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ |
                    DMA_TI_SRC_INC | DMA_TI_DEST_INC | DMA_TI_PERMAP(DMA_DREQ_HDMI_MAI);
    dd->cb[0]->source_ad = phys_bus_addr(dd->dmabuf[0]);
    dd->cb[0]->dest_ad = phys_bus_addr((APTR)(dd->hdmi_base + HDMI_MAI_DATA));
    dd->cb[0]->txfr_len = buf_bytes;
    dd->cb[0]->stride = 0;
    dd->cb[0]->nextconbk = phys_bus_addr(dd->cb[1]);

    dd->cb[1]->ti = dd->cb[0]->ti;
    dd->cb[1]->source_ad = phys_bus_addr(dd->dmabuf[1]);
    dd->cb[1]->dest_ad = dd->cb[0]->dest_ad;
    dd->cb[1]->txfr_len = buf_bytes;
    dd->cb[1]->stride = 0;
    dd->cb[1]->nextconbk = phys_bus_addr(dd->cb[0]);

    /* Configure HDMI MAI Hardware */
    wr32(dd->hdmi_base + HDMI_MAI_CTL, MAI_CTL_FLUSH);
    wr32(dd->hdmi_base + HDMI_MAI_CTL, MAI_CTL_ENABLE | MAI_CTL_CHANNELS_2 |
                                       MAI_CTL_FORMAT_16BIT | MAI_CTL_DREQ_EN);

    /* Reset DMA Channel */
    dma_ch_base = dd->periiobase + DMA_BASE_OFFSET + (dd->dma_channel * DMA_CH_STRIDE);
    wr32(dma_ch_base, DMA_CS_RESET);

    return TRUE;
}

void rpihdmi_hw_cleanup(struct RPiHDMIData *dd)
{
    rpihdmi_hw_stop_dma(dd);

    if (dd->dmabuf[0]) {
        FreeMem(dd->dmabuf[0], dd->dmabuf_size);
        dd->dmabuf[0] = NULL;
    }
    if (dd->dmabuf[1]) {
        FreeMem(dd->dmabuf[1], dd->dmabuf_size);
        dd->dmabuf[1] = NULL;
    }
    if (dd->mixbuffer) {
        FreeMem(dd->mixbuffer, dd->dmabuf_samples * 2 * sizeof(WORD));
        dd->mixbuffer = NULL;
    }
    if (dd->cb_base) {
        FreeMem(dd->cb_base, sizeof(struct DMAControlBlock) * 2 + 32);
        dd->cb_base = NULL;
    }
}

void rpihdmi_hw_start_dma(struct RPiHDMIData *dd)
{
    IPTR dma_ch_base = dd->periiobase + DMA_BASE_OFFSET + (dd->dma_channel * DMA_CH_STRIDE);

    wr32(dma_ch_base, DMA_CS_RESET);
    wr32(dma_ch_base + 0x04, phys_bus_addr(dd->cb[0])); /* CB address */
    wr32(dma_ch_base, DMA_CS_ACTIVE | DMA_CS_INT);
}

void rpihdmi_hw_stop_dma(struct RPiHDMIData *dd)
{
    if (dd->periiobase) {
        IPTR dma_ch_base = dd->periiobase + DMA_BASE_OFFSET + (dd->dma_channel * DMA_CH_STRIDE);
        wr32(dma_ch_base, DMA_CS_RESET);
    }
}

ULONG rpihdmi_hw_irq_handler(struct RPiHDMIData *dd)
{
    IPTR dma_ch_base = dd->periiobase + DMA_BASE_OFFSET + (dd->dma_channel * DMA_CH_STRIDE);
    uint32_t cs = rd32(dma_ch_base);

    if (cs & DMA_CS_INT) {
        /* Acknowledge interrupt */
        wr32(dma_ch_base, DMA_CS_INT | (cs & DMA_CS_ACTIVE));
        return 1;
    }
    return 0;
}
