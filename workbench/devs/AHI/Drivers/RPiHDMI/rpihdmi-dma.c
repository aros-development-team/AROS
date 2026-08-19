#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <aros/macros.h>

#include "rpihdmi-dma.h"
#include "DriverData.h"

#include "rpihdmi-hwaccess.h"

/*
 * Microsecond delay using a busy loop on the system timer.
 */
static void udelay(IPTR peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (peribase + 0x003004);
    ULONG start = AROS_LE2LONG(*clo);

    while ((AROS_LE2LONG(*clo) - start) < us)
        ;
}

/******************************************************************************
** DMA setup ******************************************************************
******************************************************************************/

void dma_build_control_blocks(struct RPiHDMIData *dd)
{
    const struct RPiHDMISoc *soc = dd->soc;

    int i;

    for (i = 0; i < 2; i++) {
        struct BCM2708DMACB *cb = dd->cb[i];

        cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC | DMA_TI_BURST_LENGTH(2) |
                 DMA_TI_PERMAP(dd->dma_dreq) | DMA_TI_NO_WIDE_BURSTS;

        cb->source_ad = GPU_BUS_ADDR(dd->dmabuf[i]);
        cb->dest_ad = soc->mai_data_bus;
        cb->txfr_len = dd->dmabuf_size;
        cb->stride = 0;
        cb->nextconbk = GPU_BUS_ADDR(dd->cb[1 - i]);
        cb->reserved[0] = 0;
        cb->reserved[1] = 0;
    }
}

void dma_setup(struct RPiHDMIData *dd)
{
    struct DriverBase *AHIsubBase =
        (struct DriverBase *) dd->ahisubbase;
    IPTR peribase = dd->periiobase;
    ULONG channel = dd->dma_channel;
    ULONG cb_bus_addr = GPU_BUS_ADDR(dd->cb[0]);
    IPTR dma_base = peribase + 0x007000 + channel * 0x100;
    /* The channel is already enabled by dma.resource at allocation. */
    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 10);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
    wr32le(dma_base + 0x04, cb_bus_addr);
    /* The handler re-writes this with the W1C flags added, so the priorities
     * survive the session; see bcm2708_dma.h. */
    wr32le(dma_base + 0x00, BCM2708_DMA_CS_RUN);
    udelay(peribase, 10);

    D(bug("[RPiHDMI] DMA after setup: CS=%08lx CB=%08lx TXFR=%08lx\n",
        rd32le(dma_base + 0x00),
        rd32le(dma_base + 0x04),
        rd32le(dma_base + 0x14)));
}

void dma_stop(struct RPiHDMIData *dd)
{
    IPTR peribase = dd->periiobase;
    ULONG channel = dd->dma_channel;
    IPTR dma_base = peribase + 0x007000 + channel * 0x100;

    wr32le(dma_base + 0x00, 0);
    udelay(peribase, 50);
    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 100);
    wr32le(dma_base + 0x04, 0);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
}

/******************************************************************************
** DMA interrupt handler ******************************************************
******************************************************************************/

/*
 * This is called from the DMA IRQ context via KrnAddIRQHandler.
 * We acknowledge the DMA interrupt and signal the slave task.
 */
#undef SysBase

void dma_irq_handler(struct RPiHDMIData *data, void *data2)
{
    struct ExecBase *SysBase = (struct ExecBase *) data2;
    IPTR dma_base = data->periiobase + 0x007000 + data->dma_channel * 0x100;
    ULONG cs = rd32le(dma_base + 0x00);

    if (cs & DMA_CS_INT) {
        /* Must carry the run state, not just ACTIVE: the AXI priorities
         * share the register. See bcm2708_dma.h. */
        wr32le(dma_base + 0x00, BCM2708_DMA_CS_ACK);

        if (data->slavetask != NULL && data->slavesignal != -1) {
            Signal((struct Task *) data->slavetask, 1L << data->slavesignal);
        }
    }
}

ULONG dma_probe_dreq(struct RPiHDMIData *dd, ULONG expect)
{
    IPTR peribase = dd->periiobase;
    IPTR dma_base = peribase + 0x007000 + dd->dma_channel * 0x100;
    ULONG len = dd->dmabuf_size;
    ULONG best = 0;
    ULONG best_err = ~0U;
    ULONG n;

    for (n = 1; n < 32; n++) {
        ULONG left, moved, err;

        wr32le(dma_base + 0x00, DMA_CS_RESET);
        udelay(peribase, 100);

        dd->cb[0]->ti = DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC |
                        DMA_TI_PERMAP(n) | DMA_TI_NO_WIDE_BURSTS;

        dd->cb[0]->source_ad = GPU_BUS_ADDR(dd->dmabuf[0]);
        dd->cb[0]->dest_ad = dd->soc->mai_data_bus;
        dd->cb[0]->txfr_len = len;
        dd->cb[0]->stride = 0;
        dd->cb[0]->nextconbk = 0;

        CacheClearE(dd->cb[0], sizeof(struct BCM2708DMACB), CACRF_ClearD);

        wr32le(dma_base + 0x04, GPU_BUS_ADDR(dd->cb[0]));
        wr32le(dma_base + 0x00, DMA_CS_ACTIVE);

        udelay(peribase, 10000);

        left = rd32le(dma_base + 0x14);          /* TXFR_LEN, counts down */
        wr32le(dma_base + 0x00, DMA_CS_RESET);

        moved = (left < len) ? (len - left) / sizeof(ULONG) : 0;
        if (moved == 0)
            continue;

        err = (moved > expect) ? (moved - expect) : (expect - moved);
        D(bug("[RPiHDMI] dreq probe: permap %u moved %u words in 10 ms (want ~%u)\n",
            n, moved, expect));

        if (err < best_err) {
            best_err = err;
            best = n;
        }
    }

    /* Within a quarter of the expected rate is the paced one; anything else is
     * some other peripheral's request line and no use to us. */
    if (best_err > expect / 4) {
        D(bug("[RPiHDMI] dreq probe: nothing paced at ~%u words/10 ms, keeping %u\n",
            expect, dd->soc->dma_dreq));
        best = dd->soc->dma_dreq;
    } else
        D(bug("[RPiHDMI] dreq probe: using permap %u\n", best));

    return best;
}
