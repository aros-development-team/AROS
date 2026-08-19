/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI Audio AHI Playback Engine & DMA Interrupt Slave
*/

#include <exec/types.h>
#include <exec/interrupts.h>
#include <proto/exec.h>
#include <proto/ahi_sub.h>

#include "DriverData.h"
#include "rpihdmi-hwaccess.h"

void rpihdmi_fill_buffer(struct RPiHDMIData *dd, ULONG buf_idx)
{
    ULONG *dst = dd->dmabuf[buf_idx];
    WORD *src = (WORD *)dd->mixbuffer;
    ULONG samples = dd->dmabuf_samples;
    ULONG i;

    /* Request next chunk of audio from AHI client */
    if (dd->driverdata.ahiac_PlayerFunc) {
        /* Mix signed 16-bit stereo samples into mixbuffer */
        CallHookPkt(dd->driverdata.ahiac_PlayerFunc, &dd->driverdata, dd->mixbuffer);
    } else {
        /* Silence */
        for (i = 0; i < samples * 2; i++) {
            src[i] = 0;
        }
    }

    /* Convert/pack 16-bit stereo (L + R) into 32-bit MAI FIFO words */
    for (i = 0; i < samples; i++) {
        uint16_t left  = (uint16_t)src[i * 2];
        uint16_t right = (uint16_t)src[i * 2 + 1];
        dst[i] = ((uint32_t)right << 16) | (uint32_t)left;
    }
}

AROS_UFH3(void, rpihdmi_interrupt_handler,
          AROS_UFHA(struct ExceptionContext *, ctx, A0),
          AROS_UFHA(struct ExecBase *, sysBase, A6),
          AROS_UFHA(struct RPiHDMIData *, dd, A1))
{
    AROS_USERFUNC_INIT

    (void)ctx;
    (void)sysBase;

    if (rpihdmi_hw_irq_handler(dd)) {
        /* Toggle active buffer and refill */
        static ULONG current_buf = 0;
        current_buf ^= 1;
        rpihdmi_fill_buffer(dd, current_buf);
    }

    AROS_USERFUNC_EXIT
}
