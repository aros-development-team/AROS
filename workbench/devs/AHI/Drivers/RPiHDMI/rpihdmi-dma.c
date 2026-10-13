#define DEBUG 0
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>
#include <aros/macros.h>

#include "rpihdmi-dma.h"
#include "DriverData.h"

#include "rpihdmi-hwaccess.h"

extern APTR KernelBase;
extern APTR DMABase;

static inline IPTR dma_ch_base(struct RPiHDMIData *dd)
{
    return BCM2708_DMA_CH_BASE(dd->periiobase, dd->dma_channel);
}

/* DMA4 takes a 40-bit physical address, the legacy engines the VC bus alias. */
static UQUAD dma_cb_addr(struct RPiHDMIData *dd, int i)
{
    IPTR phys = (IPTR) KrnVirtualToPhysical(dd->cb[i]);

    if (dd->dma4)
        return BCM2708_DMA4_SDRAM(dd->periiobase, phys);

    return GPU_BUS_ADDR(phys);
}

static UQUAD dma_buf_addr(struct RPiHDMIData *dd, int i)
{
    IPTR phys = (IPTR) KrnVirtualToPhysical(dd->dmabuf[i]);

    if (dd->dma4)
        return BCM2708_DMA4_SDRAM(dd->periiobase, phys);

    return GPU_BUS_ADDR(phys);
}

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

    /* Not set at allocation: AHIC_Output can move us to the other HDMI port. */
    dd->dest_addr = dd->dma4 ? (UQUAD) HDMI_MAI_DATA(dd)
                             : (UQUAD) soc->mai_data_bus;

    for (i = 0; i < 2; i++) {
        if (dd->dma4) {
            struct BCM2711DMA4CB *cb = (struct BCM2711DMA4CB *) dd->cb[i];

            cb->ti = DMA4_TI_INTEN | DMA4_TI_WAIT_RESP | DMA4_TI_D_DREQ |
                     DMA4_TI_PERMAP(dd->dma_dreq);

            cb->src   = (ULONG) dma_buf_addr(dd, i);
            cb->srci  = DMA4_XI_ADDR_HI(dma_buf_addr(dd, i)) |
                        DMA4_XI_BURST_LEN(2) | DMA4_XI_INC;
            cb->dest  = (ULONG) dd->dest_addr;
            cb->desti = DMA4_XI_ADDR_HI(dd->dest_addr);
            /* Erratum: DMA4 only moves low16(len) bytes, harmless below
             * 8191 frames. */
            cb->len   = dd->dmabuf_size;
            cb->next_cb = DMA4_CB_ADDR(dma_cb_addr(dd, 1 - i));
            cb->reserved = 0;
            continue;
        }

        {
            struct BCM2708DMACB *cb = dd->cb[i];

            cb->ti = DMA_TI_INTEN | DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC | DMA_TI_BURST_LENGTH(2) |
                     DMA_TI_PERMAP(dd->dma_dreq) | DMA_TI_NO_WIDE_BURSTS;

            cb->source_ad = (ULONG) dma_buf_addr(dd, i);
            cb->dest_ad = soc->mai_data_bus;
            cb->txfr_len = dd->dmabuf_size;
            cb->stride = 0;
            cb->nextconbk = (ULONG) dma_cb_addr(dd, 1 - i);
            cb->reserved[0] = 0;
            cb->reserved[1] = 0;
        }
    }
}

/* Which control block CONBLK_AD says the engine is running. */
int dma_active_cb(struct RPiHDMIData *dd)
{
    ULONG cbaddr = rd32le(dma_ch_base(dd) + 0x04);
    ULONG want = dd->dma4 ? DMA4_CB_ADDR(dma_cb_addr(dd, 0))
                          : (ULONG) dma_cb_addr(dd, 0);

    return (cbaddr == want) ? 0 : 1;
}

void dma_set_txfr_len(struct RPiHDMIData *dd, int i, ULONG bytes)
{
    if (dd->dma4)
        ((struct BCM2711DMA4CB *) dd->cb[i])->len = bytes;
    else
        dd->cb[i]->txfr_len = bytes;
}

void dma_setup(struct RPiHDMIData *dd)
{
    struct DriverBase *AHIsubBase =
        (struct DriverBase *) dd->ahisubbase;
    IPTR peribase = dd->periiobase;
    IPTR dma_base = dma_ch_base(dd);

    /* dma.resource has already enabled and quiesced the channel. */
    if (dd->dma4) {
        (void) rd32le(dma_base + DMA4_REG_DEBUG);   /* error latches are RC */
        wr32le(dma_base + DMA4_REG_CS, DMA4_CS_INT | DMA4_CS_END);
        wr32le(dma_base + DMA4_REG_CB, DMA4_CB_ADDR(dma_cb_addr(dd, 0)));
        wr32le(dma_base + DMA4_REG_CS,
               DMA4_CS_ACTIVE | DMA4_CS_PROT | DMA4_CS_WAIT_FOR_WRITES);
        udelay(peribase, 10);

        D(bug("[RPiHDMI] DMA4 after setup: CS=%08lx CB=%08lx LEN=%08lx\n",
            rd32le(dma_base + DMA4_REG_CS),
            rd32le(dma_base + DMA4_REG_CB),
            rd32le(dma_base + DMA4_REG_LEN)));
        return;
    }

    wr32le(dma_base + 0x00, DMA_CS_RESET);
    udelay(peribase, 10);
    wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END);
    wr32le(dma_base + 0x04, (ULONG) dma_cb_addr(dd, 0));
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
    struct DriverBase *AHIsubBase =
        (struct DriverBase *) dd->ahisubbase;
    IPTR dma_base = dma_ch_base(dd);

    D(bug("[RPiHDMI] stop: irqs=%lu CS=%08lx CB=%08lx DEBUG=%08lx LEN=%08lx\n",
        dd->irq_count,
        rd32le(dma_base + 0x00),
        rd32le(dma_base + 0x04),
        rd32le(dma_base + (dd->dma4 ? DMA4_REG_DEBUG : 0x20)),
        rd32le(dma_base + (dd->dma4 ? DMA4_REG_LEN : 0x14))));

    DMAStopChannel(dd->dma_channel);
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
    IPTR dma_base = BCM2708_DMA_CH_BASE(data->periiobase, data->dma_channel);
    ULONG cs = rd32le(dma_base + 0x00);

    if (cs & DMA_CS_INT) {
        data->irq_count++;
        /* Must carry the run state, not just ACTIVE: the AXI priorities
         * share the register. See bcm2708_dma.h. */
        if (data->dma4)
            wr32le(dma_base + DMA4_REG_CS,
                   DMA4_CS_ACTIVE | DMA4_CS_PROT | DMA4_CS_WAIT_FOR_WRITES |
                   DMA4_CS_INT | DMA4_CS_END);
        else
            wr32le(dma_base + 0x00, BCM2708_DMA_CS_ACK);

        if (data->slavetask != NULL && data->slavesignal != -1) {
            Signal((struct Task *) data->slavetask, 1L << data->slavesignal);
        }
    }
}

/* Diagnostic only, and legacy engines only. */
ULONG dma_probe_dreq(struct RPiHDMIData *dd, ULONG expect)
{
    struct DriverBase *AHIsubBase =
        (struct DriverBase *) dd->ahisubbase;
    IPTR peribase = dd->periiobase;
    IPTR dma_base = dma_ch_base(dd);
    ULONG len = dd->dmabuf_size;
    ULONG best = 0;
    ULONG best_err = ~0U;
    ULONG n;

    if (dd->dma4)
        return dd->soc->dma_dreq;

    for (n = 1; n < 32; n++) {
        ULONG left, moved, err;

        wr32le(dma_base + 0x00, DMA_CS_RESET);
        udelay(peribase, 100);

        dd->cb[0]->ti = DMA_TI_WAIT_RESP | DMA_TI_DEST_DREQ | DMA_TI_SRC_INC |
                        DMA_TI_PERMAP(n) | DMA_TI_NO_WIDE_BURSTS;

        dd->cb[0]->source_ad = (ULONG) dma_buf_addr(dd, 0);
        dd->cb[0]->dest_ad = dd->soc->mai_data_bus;
        dd->cb[0]->txfr_len = len;
        dd->cb[0]->stride = 0;
        dd->cb[0]->nextconbk = 0;

        CacheClearE(dd->cb[0], sizeof(struct BCM2708DMACB), CACRF_ClearD);

        wr32le(dma_base + 0x04, (ULONG) dma_cb_addr(dd, 0));
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
