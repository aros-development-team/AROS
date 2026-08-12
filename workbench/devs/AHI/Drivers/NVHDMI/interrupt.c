/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2026 The AROS Dev Team.

All Rights Reserved.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <config.h>

#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <stddef.h>
#include "library.h"
#include "interrupt.h"
#include "hdmi.h"
#include "hda_hidd.h"

/*
    The hardware interrupt lives in hdaudio.hidd; it Cause()s the
    interrupt below when a stream buffer completes.
*/

/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct NVHDMIChip *card)
#else
void
PlaybackInterrupt(struct NVHDMIChip *card)
#endif
{
    struct AHIAudioCtrlDrv *AudioCtrl = card->audioctrl;
    struct DriverBase  *AHIsubBase = (struct DriverBase *) card->ahisubbase;

    /* A buffer has completed; refill it while the other one plays */
    if(card->flip == 1) {
        card->flip = 0;
        card->current_buffer = card->playback_buffer1;
    } else {
        card->flip = 1;
        card->current_buffer = card->playback_buffer2;
    }

    if(card->mix_buffer != NULL && card->current_buffer != NULL && card->is_playing) {
        BOOL   skip_mix;

        int    i;
        LONG *srclong, *dstlong;
        int frames = card->current_frames;

        skip_mix = CallHookPkt(AudioCtrl->ahiac_PreTimerFunc, (Object *) AudioCtrl, 0);
        CallHookPkt(AudioCtrl->ahiac_PlayerFunc, (Object *) AudioCtrl, NULL);

        if(!skip_mix) {
            CallHookPkt(AudioCtrl->ahiac_MixerFunc, (Object *) AudioCtrl, card->mix_buffer);
        }

        /* Now translate and transfer to the DMA buffer */
        srclong = (LONG *) card->mix_buffer;
        dstlong = (LONG *) card->current_buffer;

        i = frames;

        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI) {
            if(card->bitsizes[card->selected_bitsize_index] == 24) {
                /* 32-bit mixing buffer, 24-bit hardware format, stereo => frame size of 8 bytes */
                while(i > 0) {
                    *dstlong++ = *srclong++;
                    *dstlong++ = *srclong++;

                    --i;
                }
            }
        }

        if(!(AudioCtrl->ahiac_Flags & AHIACF_HIFI)) {
            if(card->bitsizes[card->selected_bitsize_index] == 16) {
                /* 16-bit mixing buffer, 16-bit hardware format, stereo => frame size of 4 bytes */
                while(i > 0) {
                    *dstlong++ = *srclong++;

                    --i;
                }
            }
        }

        /* Make the DMA buffer visible to the controller */
        hda_stream_sync(card->hda_ctrl, card->output_stream,
                        (card->current_buffer == card->playback_buffer1) ? 0 : 1);

        CallHookPkt(AudioCtrl->ahiac_PostTimerFunc, (Object *) AudioCtrl, 0);
    }
}
