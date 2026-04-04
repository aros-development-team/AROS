/*
 *  Slave process for RPiPWM AHI driver.
 *
 *  This process runs in a loop:
 *  1. Call the AHI PlayerFunc (timing)
 *  2. Call the AHI MixerFunc to fill the mix buffer with signed 16-bit samples
 *  3. Convert the mixed samples to unsigned PWM values and write to DMA buffer
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

#include "DriverData.h"
#include "library.h"
#include "rpipwm-hwaccess.h"

#define dd ((struct RPiPWMData *) AudioCtrl->ahiac_DriverData)

/******************************************************************************
** Sample conversion **********************************************************
******************************************************************************/

/*
 * Convert AHI mix buffer (signed 16-bit stereo, native endian) to
 * unsigned 32-bit PWM values for the DMA buffer.
 *
 * AHI's MixerFunc produces ahiac_BuffType data:
 *   AHIST_S16S = signed 16-bit stereo (interleaved L, R)
 *
 * PWM expects unsigned values in range [0, pwm_range-1].
 * We convert: pwm_val = ((sample + 32768) * pwm_range) >> 16
 *
 * The DMA buffer has interleaved 32-bit L/R words that feed the PWM FIFO.
 * Channel 1 (left) reads the first word, Channel 2 (right) reads the second.
 */
static void convert_mix_to_pwm(WORD *src, ULONG *dst, ULONG frames, ULONG range)
{
    ULONG i;

    for (i = 0; i < frames; i++) {
        LONG left = (LONG) src[i * 2]; /* -32768 .. 32767 */
        LONG right = (LONG) src[i * 2 + 1];

        /* Convert to unsigned and scale to PWM range */
        dst[i * 2] = AROS_LONG2LE(((left + 32768) * range) >> 16);
        dst[i * 2 + 1] = AROS_LONG2LE(((right + 32768) * range) >> 16);
    }
}


/******************************************************************************
** DMA interrupt handler ******************************************************
******************************************************************************/

/*
 * This is called from the DMA IRQ context via KrnAddIRQHandler.
 * We acknowledge the DMA interrupt and signal the slave task.
 *
 * Signature matches irqhandler_t: void handler(void *data, void *data2)
 */
#undef SysBase

void dma_irq_handler(struct RPiPWMData *data, void *data2)
{
    struct ExecBase *SysBase = (struct ExecBase *) data2;
    ULONG dma_base = data->periiobase + 0x007000 + data->dma_channel * 0x100;
    ULONG cs = rd32le(dma_base + 0x00);

    if (cs & DMA_CS_INT) {
        /*
         * Acknowledge the interrupt by writing only the W1C bits
         * plus ACTIVE to keep the DMA running. Don't write back the
         * read value — read-only bits like PAUSED can confuse the
         * DMA state machine.
         */
        wr32le(dma_base + 0x00, DMA_CS_INT | DMA_CS_END | DMA_CS_ACTIVE);

        /* Signal the slave task to refill a buffer */
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

    if (dd->slavesignal != -1) {
        /*
         * Pre-fill both DMA buffers with silence (mid-range PWM value)
         * so there's no click/pop on startup.
         */
        ULONG mid = dd->pwm_range / 2;
        ULONG i;

        for (i = 0; i < dd->dmabuf_samples * 2; i++) {
            dd->dmabuf[0][i] = AROS_LONG2LE(mid);
            dd->dmabuf[1][i] = AROS_LONG2LE(mid);
        }

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

                /*
                 * A DMA buffer has been consumed.
                 * Figure out which buffer just finished playing
                 * and refill it while the other one plays.
                 */

                /* Call the player hook (timing) */
                CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);

                /* Call the mixer to fill our mix buffer */
                CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer);

                /*
                 * Read the DMA CBADDR register to find which control
                 * block is active.  Fill the OTHER buffer so we never
                 * write to the buffer the DMA is currently reading.
                 */
                {
                    ULONG dma_base = dd->periiobase + 0x007000 + dd->dma_channel * 0x100;
                    ULONG cbaddr = rd32le(dma_base + 0x04);
                    ULONG fillbuf;

                    if (cbaddr == GPU_BUS_ADDR(dd->cb[0]))
                        fillbuf = 1; /* DMA on CB[0] → fill dmabuf[1] */
                    else
                        fillbuf = 0; /* DMA on CB[1] → fill dmabuf[0] */

                    convert_mix_to_pwm(
                        (WORD *) dd->mixbuffer, dd->dmabuf[fillbuf], AudioCtrl->ahiac_BuffSamples, dd->pwm_range);

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
