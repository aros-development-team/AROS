/*
 *  AHI driver for Raspberry Pi PWM audio output.
 *
 *  Uses the BCM2835 PWM peripheral with DMA-based double buffering
 *  to play audio through the 3.5mm headphone jack (GPIO 40/45).
 */

#include <config.h>

#include <devices/ahi.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/kernel.h>

#include <stddef.h>
#include <string.h>

#include "library.h"
#include "DriverData.h"
#include "rpipwm-hwaccess.h"

#define dd ((struct RPiPWMData *) AudioCtrl->ahiac_DriverData)

void SlaveEntry(void);

#ifdef PROCGW
PROCGW(static, void, slaveentry, SlaveEntry);
#else
#define slaveentry SlaveEntry
#endif

extern APTR KernelBase;

/*
 * Supported sample rates.
 * We support a small set of common rates that work well with
 * the PWM clock divisor from PLLD (500 MHz).
 */
static const LONG frequencies[] = {
    8000,
    11025,
    22050,
    44100,
    48000,
};

#define FREQUENCIES (sizeof frequencies / sizeof frequencies[0])


/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG
_AHIsub_AllocAudio(struct TagItem *taglist, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    struct RPiPWMBase *RPiPWMBase = (struct RPiPWMBase *) AHIsubBase;

    AudioCtrl->ahiac_DriverData = AllocVec(sizeof(struct RPiPWMData), MEMF_CLEAR | MEMF_PUBLIC);

    if (dd != NULL) {
        dd->slavesignal = -1;
        dd->mastersignal = AllocSignal(-1);
        dd->mastertask = (struct Process *) FindTask(NULL);
        dd->ahisubbase = RPiPWMBase;
        dd->periiobase = RPiPWMBase->periiobase;
        dd->dma_channel = PWM_DMA_CHANNEL;
        dd->pwm_range = PWM_AUDIO_RANGE;
    } else {
        return AHISF_ERROR;
    }

    if (dd->mastersignal == -1) {
        return AHISF_ERROR;
    }

    return (AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING);
}


/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void _AHIsub_FreeAudio(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    if (AudioCtrl->ahiac_DriverData != NULL) {
        FreeSignal(dd->mastersignal);
        FreeVec(AudioCtrl->ahiac_DriverData);
        AudioCtrl->ahiac_DriverData = NULL;
    }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void _AHIsub_Disable(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void _AHIsub_Enable(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG
_AHIsub_Start(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    struct RPiPWMBase *RPiPWMBase = (struct RPiPWMBase *) AHIsubBase;

    AHIsub_Stop(flags, AudioCtrl);

    if (flags & AHISF_PLAY) {
        struct TagItem proctags[] = {
            {NP_Entry, (IPTR) &slaveentry}, {NP_Name, (IPTR) LibName}, {NP_Priority, 50}, {TAG_DONE, 0}};

        ULONG buf_frames;
        ULONG buf_bytes;
        ULONG cb_alloc_size;
        UBYTE *cb_raw;
        int i;

        dd->samplerate = AudioCtrl->ahiac_MixFreq;

        /*
         * Calculate DMA buffer size.
         * Use the AHI-requested buffer size in sample frames.
         * Each frame = 2 channels * 4 bytes (32-bit PWM words) = 8 bytes.
         */
        buf_frames = AudioCtrl->ahiac_MaxBuffSamples;
        if (buf_frames < 256)
            buf_frames = 256;
        buf_bytes = buf_frames * 2 * sizeof(ULONG); /* stereo, 32-bit per sample */

        dd->dmabuf_samples = buf_frames;
        dd->dmabuf_size = buf_bytes;

        /* Allocate AHI mix buffer */
        dd->mixbuffer = AllocVec(AudioCtrl->ahiac_BuffSize, MEMF_ANY | MEMF_PUBLIC);
        if (dd->mixbuffer == NULL)
            return AHIE_NOMEM;

        /* Allocate DMA buffers (need physical contiguous memory) */
        for (i = 0; i < 2; i++) {
            dd->dmabuf[i] = AllocVec(buf_bytes, MEMF_CLEAR | MEMF_PUBLIC);
            if (dd->dmabuf[i] == NULL)
                return AHIE_NOMEM;
        }

        /*
         * Allocate DMA control blocks.
         * Each CB is 32 bytes and must be 32-byte aligned.
         * Allocate enough for 2 CBs + alignment padding.
         */
        cb_alloc_size = sizeof(struct DMAControlBlock) * 2 + 32;
        cb_raw = AllocVec(cb_alloc_size, MEMF_CLEAR | MEMF_PUBLIC);
        if (cb_raw == NULL)
            return AHIE_NOMEM;

        dd->cb_base = (struct DMAControlBlock *) cb_raw;

        /* Align to 32 bytes */
        cb_raw = (UBYTE *) (((ULONG) cb_raw + 31) & ~31);
        dd->cb[0] = (struct DMAControlBlock *) cb_raw;
        dd->cb[1] = (struct DMAControlBlock *) (cb_raw + sizeof(struct DMAControlBlock));

        /* Build the DMA control block chain */
        dma_build_control_blocks(dd, dd->periiobase);

        /*
         * Flush DMA control blocks and buffers from ARM data cache
         * to physical RAM. The DMA engine reads via the GPU bus
         * (uncached alias 0xC0000000) and won't see cached data.
         */
        CacheClearE(dd->cb[0], sizeof(struct DMAControlBlock) * 2, CACRF_ClearD);
        CacheClearE(dd->dmabuf[0], buf_bytes, CACRF_ClearD);
        CacheClearE(dd->dmabuf[1], buf_bytes, CACRF_ClearD);

        /* Configure hardware (but don't start DMA yet) */
        pwm_gpio_setup(dd->periiobase);
        pwm_clock_setup(dd->periiobase, dd->samplerate, dd->pwm_range);
        pwm_init(dd->periiobase, dd->pwm_range);

        /*
         * Start slave task.
         * On SMP, Forbid() only prevents task switching on the local CPU,
         * so the new process can start on another CPU immediately.
         * We set tc_UserData with a memory barrier so the slave can
         * busy-wait on it safely.
         */
        dd->slavetask = CreateNewProc(proctags);

        if (dd->slavetask != NULL) {
            dd->slavetask->pr_Task.tc_UserData = AudioCtrl;
            __sync_synchronize();
        }

        if (dd->slavetask != NULL) {
            /* Wait for slave to allocate its signal and pre-fill buffers */
            Wait(1L << dd->mastersignal);

            if (dd->slavetask == NULL) {
                return AHIE_UNKNOWN;
            }
        } else {
            return AHIE_NOMEM;
        }

        /*
         * Register IRQ and start DMA AFTER the slave is ready.
         * The slave has allocated slavesignal and pre-filled both
         * DMA buffers, so IRQ signals won't be lost.
         */
        dd->irq_handle = KrnAddIRQHandler(BCM_IRQ_DMA0 + dd->dma_channel, dma_irq_handler, dd, SysBase);

        dma_setup(dd->periiobase, dd->dma_channel, GPU_BUS_ADDR(dd->cb[0]));
    }

    if (flags & AHISF_RECORD) {
        return AHIE_UNKNOWN; /* Recording not supported */
    }

    return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void _AHIsub_Update(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    /* Nothing to do — buffer parameters don't change dynamically */
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void _AHIsub_Stop(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase)
{
    if (flags & AHISF_PLAY) {
        int i;

        /* Signal slave task to exit */
        if (dd->slavetask != NULL) {
            Signal((struct Task *) dd->slavetask, SIGBREAKF_CTRL_C);
            Wait(1L << dd->mastersignal);
        }

        /* Remove IRQ handler first so no callbacks fire during teardown */
        if (dd->irq_handle != NULL) {
            KrnRemIRQHandler(dd->irq_handle);
            dd->irq_handle = NULL;
        }

        /* Stop hardware */
        dma_stop(dd->periiobase, dd->dma_channel);
        pwm_stop(dd->periiobase);
        pwm_clock_stop(dd->periiobase);
        pwm_gpio_restore(dd->periiobase);

        dd->slavesignal = -1;

        /* Free buffers */
        for (i = 0; i < 2; i++) {
            FreeVec(dd->dmabuf[i]);
            dd->dmabuf[i] = NULL;
        }

        FreeVec(dd->cb_base);
        dd->cb_base = NULL;
        dd->cb[0] = NULL;
        dd->cb[1] = NULL;

        FreeVec(dd->mixbuffer);
        dd->mixbuffer = NULL;
    }

    if (flags & AHISF_RECORD) {
        /* Nothing */
    }
}


/******************************************************************************
** AHIsub_GetAttr *************************************************************
******************************************************************************/

IPTR _AHIsub_GetAttr(ULONG attribute,
                     LONG argument,
                     IPTR def,
                     struct TagItem *taglist,
                     struct AHIAudioCtrlDrv *AudioCtrl,
                     struct DriverBase *AHIsubBase)
{
    size_t i;

    switch (attribute) {
    case AHIDB_Bits:
        return 16;

    case AHIDB_Frequencies:
        return FREQUENCIES;

    case AHIDB_Frequency:
        return (LONG) frequencies[argument];

    case AHIDB_Index:
        if (argument <= frequencies[0]) {
            return 0;
        }

        if (argument >= frequencies[FREQUENCIES - 1]) {
            return FREQUENCIES - 1;
        }

        for (i = 1; i < FREQUENCIES; i++) {
            if (frequencies[i] > argument) {
                if ((argument - frequencies[i - 1]) < (frequencies[i] - argument)) {
                    return i - 1;
                } else {
                    return i;
                }
            }
        }

        return 0;

    case AHIDB_Author:
        return (IPTR) "AROS Development Team";

    case AHIDB_Copyright:
        return (IPTR) "AROS Public License";

    case AHIDB_Version:
        return (IPTR) LibIDString;

    case AHIDB_Record:
        return FALSE;

    case AHIDB_Realtime:
        return TRUE;

    case AHIDB_Outputs:
        return 1;

    case AHIDB_Output:
        return (IPTR) "3.5mm Headphone Jack";

    default:
        return def;
    }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG
_AHIsub_HardwareControl(ULONG attribute,
                        LONG argument,
                        struct AHIAudioCtrlDrv *AudioCtrl,
                        struct DriverBase *AHIsubBase)
{
    return 0;
}
