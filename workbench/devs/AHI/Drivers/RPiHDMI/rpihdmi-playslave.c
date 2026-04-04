/*
 *  Slave process for RPiHDMI AHI driver.
 *
 *  This process runs in a loop:
 *  1. Call the AHI PlayerFunc (timing)
 *  2. Call the AHI MixerFunc to fill the mix buffer with signed 16-bit samples
 *  3. Encode the mixed samples as IEC958/SPDIF subframes for DMA buffer
 *  4. Wait for the DMA interrupt signaling buffer consumption
 *
 *  The DMA runs autonomously via two chained control blocks (ping-pong).
 *  When a CB completes, the DMA IRQ fires and we get signaled to refill
 *  the consumed buffer while the other buffer plays.
 */

#include <config.h>

#include <devices/ahi.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>
#include <aros/macros.h>
#include <aros/debug.h>

#include "DriverData.h"
#include "library.h"
#include "rpihdmi-hwaccess.h"

#define dd ((struct RPiHDMIData *) AudioCtrl->ahiac_DriverData)


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
        wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END | DMA_CS_ACTIVE);

        if (data->slavetask != NULL && data->slavesignal != -1) {
            Signal((struct Task *) data->slavetask, 1L << data->slavesignal);
        }
    }
}


/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

void Slave(struct ExecBase *SysBase);

#if defined(__AROS__)

#include <aros/asmcall.h>

AROS_UFH3(void,
          SlaveEntry,
          AROS_UFHA(STRPTR, argPtr, A0),
          AROS_UFHA(ULONG, argSize, D0),
          AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT
    Slave(SysBase);
    AROS_USERFUNC_EXIT
}

#else

void SlaveEntry(void)
{
    struct ExecBase *SysBase = *((struct ExecBase **) 4);
    Slave(SysBase);
}

#endif

void Slave(struct ExecBase *SysBase)
{
    struct AHIAudioCtrlDrv *AudioCtrl;
    struct DriverBase *AHIsubBase;
    BOOL running;
    ULONG signals;

    /*
     * On SMP the master may not have set tc_UserData yet when we start
     * running on another CPU.  Spin until it's visible.
     */
    while ((AudioCtrl = (struct AHIAudioCtrlDrv *) FindTask(NULL)->tc_UserData) == NULL)
        __sync_synchronize();

    AHIsubBase = (struct DriverBase *) dd->ahisubbase;

    dd->slavesignal = AllocSignal(-1);

    D(bug("[HDMI] Slave: slavesignal=%ld\n", dd->slavesignal));

    if (dd->slavesignal != -1) {
        /*
         * Pre-fill both DMA buffers with silent IEC958 subframes.
         * Silence still needs valid Z/M/W preambles and channel status bits.
         */
        ULONG i;

        for (i = 0; i < AudioCtrl->ahiac_BuffSamples * 2; i++)
            ((WORD *) dd->mixbuffer)[i] = 0;

        convert_mix_to_iec958((WORD *) dd->mixbuffer,
                              dd->dmabuf[0],
                              AudioCtrl->ahiac_BuffSamples,
                              dd->channel_status_l,
                              dd->channel_status_r,
                              &dd->frame_counter);
        convert_mix_to_iec958((WORD *) dd->mixbuffer,
                              dd->dmabuf[1],
                              AudioCtrl->ahiac_BuffSamples,
                              dd->channel_status_l,
                              dd->channel_status_r,
                              &dd->frame_counter);

        /* Flush silence data from cache to physical RAM for DMA */
        CacheClearE(dd->dmabuf[0], dd->dmabuf_size, CACRF_ClearD);
        CacheClearE(dd->dmabuf[1], dd->dmabuf_size, CACRF_ClearD);

        /* Tell the master we're alive */
        Signal((struct Task *) dd->mastertask, 1L << dd->mastersignal);

        running = TRUE;

        while (running) {
            signals = Wait(SIGBREAKF_CTRL_C | (1L << dd->slavesignal));

            if (signals & (SIGBREAKF_CTRL_C | (1L << dd->slavesignal))) {
                if (signals & SIGBREAKF_CTRL_C) {
                    running = FALSE;
                    continue;
                }

                /* Call the player hook (timing) */
                CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);

                /* Call the mixer to fill our mix buffer */
                CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer);

                /*
                 * Convert mixed audio to SPDIF subframes and write
                 * to the DMA buffer that is NOT currently being read.
                 *
                 * Read the DMA CBADDR register to find which control block
                 * is active.  The active CB's source buffer is busy — fill
                 * the OTHER one.  This avoids the blind curbuf toggle which
                 * has a 50% chance of starting out of phase with the DMA
                 * (and staying out of phase permanently).
                 */
                {
                    ULONG dma_base = dd->periiobase + 0x007000 + dd->dma_channel * 0x100;
                    ULONG cbaddr = rd32le(dma_base + 0x04);
                    ULONG fillbuf;

                    if (cbaddr == GPU_BUS_ADDR(dd->cb[0]))
                        fillbuf = 1; /* DMA on CB[0] → fill dmabuf[1] */
                    else
                        fillbuf = 0; /* DMA on CB[1] → fill dmabuf[0] */

                    /*
                     * Encode fully-formed IEC958 subframes in software.
                     * The MAI consumes L/R subframe pairs from the DMA buffer.
                     */
                    convert_mix_to_iec958((WORD *) dd->mixbuffer,
                                          dd->dmabuf[fillbuf],
                                          AudioCtrl->ahiac_BuffSamples,
                                          dd->channel_status_l,
                                          dd->channel_status_r,
                                          &dd->frame_counter);

                    /* Flush converted data to physical RAM for DMA */
                    CacheClearE(dd->dmabuf[fillbuf], dd->dmabuf_size, CACRF_ClearD);
                }
            }
        }
    }

    Forbid();

    /*
     * Clear slavetask with a barrier so the IRQ handler on other CPUs
     * stops signaling us before we die.  Don't FreeSignal — the signal
     * belongs to this dying task; the master clears slavesignal after
     * removing the IRQ handler.
     */
    dd->slavetask = NULL;
    __sync_synchronize();

    /* Tell the master we're dying */
    Signal((struct Task *) dd->mastertask, 1L << dd->mastersignal);
}
