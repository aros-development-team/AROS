/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    AHI Sub-driver entry points for Raspberry Pi I2S Audio
*/

#include <config.h>
#include <devices/ahi.h>
#include <dos/dostags.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <proto/ahi_sub.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include "DriverData.h"
#include "library.h"
#include "rpii2s-hwaccess.h"

#define dd ((struct RPiI2SData *) AudioCtrl->ahiac_DriverData)
void SlaveEntry(void);
extern APTR KernelBase;

static const LONG frequencies[] = { 8000, 11025, 22050, 44100, 48000, 96000 };
#define FREQUENCIES (sizeof frequencies / sizeof frequencies[0])

ULONG _AHIsub_AllocAudio(struct TagItem *taglist, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {
    struct RPiI2SBase *RPiI2SBase = (struct RPiI2SBase *)AHIsubBase;
    AudioCtrl->ahiac_DriverData = AllocVec(sizeof(struct RPiI2SData), MEMF_CLEAR | MEMF_PUBLIC);
    if (dd) {
        dd->slavesignal = -1;
        dd->mastersignal = AllocSignal(-1);
        dd->mastertask = (struct Process *)FindTask(NULL);
        dd->ahisubbase = RPiI2SBase;
        dd->periiobase = RPiI2SBase->periiobase;
        if (dd->periiobase >= 0x107C000000)
            dd->i2s_base = dd->periiobase + RP1_I2S_OFFSET;
        else
            dd->i2s_base = dd->periiobase + BCM_I2S_OFFSET;
        dd->dma_channel = I2S_DMA_CHANNEL;
    } else return AHISF_ERROR;
    if (dd->mastersignal == -1) return AHISF_ERROR;
    return (AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING);
}

void _AHIsub_FreeAudio(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {
    if (AudioCtrl->ahiac_DriverData) { FreeSignal(dd->mastersignal); FreeVec(AudioCtrl->ahiac_DriverData); AudioCtrl->ahiac_DriverData = NULL; }
}

void _AHIsub_Disable(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) { Disable(); }
void _AHIsub_Enable(struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) { Enable(); }

ULONG _AHIsub_Start(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {
    struct RPiI2SBase *RPiI2SBase = (struct RPiI2SBase *)AHIsubBase;
    if (flags & AHISF_PLAY) {
        struct TagItem proctags[] = {{NP_Entry,(IPTR)&SlaveEntry},{NP_Name,(IPTR)LibName},{NP_Priority,50},{TAG_DONE,0}};
        ULONG buf_frames = AudioCtrl->ahiac_MaxBuffSamples;
        ULONG buf_bytes = buf_frames * 2 * sizeof(UWORD); /* 16-bit stereo */
        UBYTE *cb_raw; int i;
        dd->samplerate = AudioCtrl->ahiac_MixFreq;
        dd->dmabuf_samples = buf_frames;
        dd->dmabuf_size = buf_bytes;
        dd->mixbuffer = AllocVec(AudioCtrl->ahiac_BuffSize, MEMF_ANY | MEMF_PUBLIC);
        if (!dd->mixbuffer) return AHIE_NOMEM;
        for (i = 0; i < 2; i++) { dd->dmabuf[i] = AllocVec(buf_bytes, MEMF_CLEAR | MEMF_PUBLIC | MEMF_31BIT); if (!dd->dmabuf[i]) return AHIE_NOMEM; }
        cb_raw = AllocVec(sizeof(struct DMAControlBlock) * 2 + 32, MEMF_CLEAR | MEMF_PUBLIC | MEMF_31BIT);
        if (!cb_raw) return AHIE_NOMEM;
        dd->cb_base = (struct DMAControlBlock *)cb_raw;
        cb_raw = (UBYTE *)(((IPTR)cb_raw + 31) & ~31);
        dd->cb[0] = (struct DMAControlBlock *)cb_raw;
        dd->cb[1] = (struct DMAControlBlock *)(cb_raw + sizeof(struct DMAControlBlock));
        dma_build_control_blocks(dd);
        CacheClearE(dd->cb[0], sizeof(struct DMAControlBlock) * 2, CACRF_ClearD);
        i2s_hw_init(dd);
        dd->slavetask = CreateNewProc(proctags);
        if (dd->slavetask) { dd->slavetask->pr_Task.tc_UserData = AudioCtrl; __sync_synchronize(); Wait(1L << dd->mastersignal); if (!dd->slavetask) return AHIE_UNKNOWN; }
        else return AHIE_NOMEM;
        dd->irq_handle = KrnAddIRQHandler(BCM_IRQ_DMA0 + dd->dma_channel, dma_irq_handler, dd, SysBase);
        dma_setup(dd->periiobase, dd->dma_channel, gpu_bus_addr((IPTR)dd->cb[0]));
    }
    return AHIE_OK;
}

void _AHIsub_Update(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {}

void _AHIsub_Stop(ULONG flags, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {
    if (flags & AHISF_PLAY) {
        int i;
        if (dd->slavetask) { Signal((struct Task *)dd->slavetask, SIGBREAKF_CTRL_C); Wait(1L << dd->mastersignal); }
        if (dd->irq_handle) { KrnRemIRQHandler(dd->irq_handle); dd->irq_handle = NULL; }
        dma_stop(dd->periiobase, dd->dma_channel);
        i2s_hw_stop(dd);
        for (i = 0; i < 2; i++) { FreeVec(dd->dmabuf[i]); dd->dmabuf[i] = NULL; }
        FreeVec(dd->cb_base); dd->cb_base = NULL;
        FreeVec(dd->mixbuffer); dd->mixbuffer = NULL;
    }
}

IPTR _AHIsub_GetAttr(ULONG attr, LONG arg, IPTR def, struct TagItem *taglist, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) {
    switch (attr) {
    case AHIDB_Bits: return 16;
    case AHIDB_Frequencies: return FREQUENCIES;
    case AHIDB_Frequency: return (LONG)frequencies[arg];
    case AHIDB_Author: return (IPTR)"AROS Development Team";
    case AHIDB_Copyright: return (IPTR)"AROS Public License";
    case AHIDB_Version: return (IPTR)LibIDString;
    case AHIDB_Record: return FALSE;
    case AHIDB_Realtime: return TRUE;
    case AHIDB_Outputs: return 1;
    case AHIDB_Output: return (IPTR)"I2S Audio";
    default: return def;
    }
}

ULONG _AHIsub_HardwareControl(ULONG attr, LONG arg, struct AHIAudioCtrlDrv *AudioCtrl, struct DriverBase *AHIsubBase) { return 0; }
