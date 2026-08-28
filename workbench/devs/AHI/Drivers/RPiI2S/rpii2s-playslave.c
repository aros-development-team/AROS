/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Raspberry Pi I2S Audio Mixing Slave Process and IRQ Handler
*/

#include <config.h>
#include <devices/ahi.h>
#include <exec/execbase.h>
#include <libraries/ahi_sub.h>
#include <aros/macros.h>
#include <aros/debug.h>
#include "DriverData.h"
#include "library.h"
#include "rpii2s-hwaccess.h"

#define dd ((struct RPiI2SData *) AudioCtrl->ahiac_DriverData)
#undef SysBase

void dma_irq_handler(struct RPiI2SData *data, void *data2) {
    struct ExecBase *SysBase = (struct ExecBase *)data2;
    IPTR dma_base = data->periiobase + 0x007000 + data->dma_channel * 0x100;
    ULONG cs = AROS_LE2LONG(*(volatile ULONG *)dma_base);
    if (cs & DMA_CS_INT) {
        *(volatile ULONG *)dma_base = AROS_LONG2LE(DMA_CS_INT | DMA_CS_END | DMA_CS_ACTIVE);
        if (data->slavetask && data->slavesignal != -1)
            Signal((struct Task *)data->slavetask, 1L << data->slavesignal);
    }
}

#include <aros/asmcall.h>
AROS_UFH3(void, SlaveEntry, AROS_UFHA(STRPTR, argPtr, A0), AROS_UFHA(ULONG, argSize, D0), AROS_UFHA(struct ExecBase *, SysBase, A6)) {
    AROS_USERFUNC_INIT
    struct AHIAudioCtrlDrv *AudioCtrl;
    struct DriverBase *AHIsubBase;
    BOOL running; ULONG signals;
    while (!(AudioCtrl = (struct AHIAudioCtrlDrv *)FindTask(NULL)->tc_UserData)) __sync_synchronize();
    AHIsubBase = (struct DriverBase *)dd->ahisubbase;
    dd->slavesignal = AllocSignal(-1);
    if (dd->slavesignal != -1) {
        /* Pre-fill with silence */
        ULONG i; for (i = 0; i < dd->dmabuf_size / 4; i++) { dd->dmabuf[0][i] = 0; dd->dmabuf[1][i] = 0; }
        CacheClearE(dd->dmabuf[0], dd->dmabuf_size, CACRF_ClearD);
        CacheClearE(dd->dmabuf[1], dd->dmabuf_size, CACRF_ClearD);
        Signal((struct Task *)dd->mastertask, 1L << dd->mastersignal);
        running = TRUE;
        while (running) {
            signals = Wait(SIGBREAKF_CTRL_C | (1L << dd->slavesignal));
            if (signals & SIGBREAKF_CTRL_C) { running = FALSE; continue; }
            CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
            CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->mixbuffer);
            /* I2S: just copy 16-bit stereo PCM directly (no encoding needed) */
            {
                IPTR dma_base = dd->periiobase + 0x007000 + dd->dma_channel * 0x100;
                ULONG cbaddr = AROS_LE2LONG(*(volatile ULONG *)(dma_base + 0x04));
                ULONG fillbuf = (cbaddr == gpu_bus_addr((IPTR)dd->cb[0])) ? 1 : 0;
                CopyMem(dd->mixbuffer, dd->dmabuf[fillbuf], dd->dmabuf_size);
                CacheClearE(dd->dmabuf[fillbuf], dd->dmabuf_size, CACRF_ClearD);
            }
        }
    }
    Forbid();
    dd->slavetask = NULL; __sync_synchronize();
    Signal((struct Task *)dd->mastertask, 1L << dd->mastersignal);
    AROS_USERFUNC_EXIT
}
