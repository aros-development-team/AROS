#include <proto/exec.h>
#include <aros/macros.h>

#include "rpihdmi-dma.h"
#include "DriverData.h"

#include "rpihdmi-hwaccess.h"

/*
 * Microsecond delay using a busy loop on the system timer.
 */
static void udelay(ULONG peribase, ULONG us)
{
    volatile ULONG *clo = (volatile ULONG *) (ULONG) (peribase + 0x003004);
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
                 DMA_TI_PERMAP(soc->dma_dreq) | DMA_TI_NO_WIDE_BURSTS;

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
    ULONG peribase = dd->periiobase;
    ULONG channel = dd->dma_channel;
    ULONG cb_bus_addr = GPU_BUS_ADDR(dd->cb[0]);
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;

    /* The channel is already enabled by dma.resource at allocation. */
    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 10);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
    wr32le(dma_base + 0x04, cb_bus_addr);
    /* The handler re-writes this with the W1C flags added, so the priorities
     * survive the session; see bcm2708_dma.h. */
    wr32le(dma_base + 0x00, BCM2708_DMA_CS_RUN);
    udelay(peribase, 10);
}

void dma_stop(struct RPiHDMIData *dd)
{
    ULONG peribase = dd->periiobase;
    ULONG channel = dd->dma_channel;
    ULONG dma_base = peribase + 0x007000 + channel * 0x100;

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
    ULONG dma_base = data->periiobase + 0x007000 + data->dma_channel * 0x100;
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
