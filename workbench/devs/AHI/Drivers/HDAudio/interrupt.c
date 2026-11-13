/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2010-2026 The AROS Dev Team.
(C) Copyright 2009-2010 Stephen Jones.
(C) Copyright xxxx-2009 Davy Wentzler.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <config.h>

#include <proto/expansion.h>
#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <stddef.h>
#include "library.h"
#include "interrupt.h"
#include "misc.h"
#include "hda_hidd.h"

#define min(a,b) ((a)<(b)?(a):(b))

/*
    The hardware interrupt lives in hdaudio.hidd; it Cause()s the
    interrupts below when a stream buffer completes.
*/

/******************************************************************************
** Playback interrupt handler *************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
PlaybackInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct HDAudioChip *card)
#else
void
PlaybackInterrupt(struct HDAudioChip *card)
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

        /* Note: other combinations of mixing buffer and hardware format are not supported at this moment */

        /* Make the DMA buffer visible to the controller */
        hda_stream_sync(card->hda_ctrl, card->output_stream,
                        (card->current_buffer == card->playback_buffer1) ? 0 : 1);

        CallHookPkt(AudioCtrl->ahiac_PostTimerFunc, (Object *) AudioCtrl, 0);
    }
}


/******************************************************************************
** Record interrupt handler ***************************************************
******************************************************************************/

#ifdef __AMIGAOS4__
void
RecordInterrupt(struct ExceptionContext *pContext, struct ExecBase *SysBase, struct HDAudioChip *card)
#else
void
RecordInterrupt(struct HDAudioChip *card)
#endif
{
    struct AHIAudioCtrlDrv *AudioCtrl = card->audioctrl;
    struct DriverBase  *AHIsubBase = (struct DriverBase *) card->ahisubbase;

    /* A buffer has completed; hand it over while the other one records */
    if(card->recflip == 1) {
        card->recflip = 0;
        card->current_record_buffer = card->record_buffer1;
    } else {
        card->recflip = 1;
        card->current_record_buffer = card->record_buffer2;
    }

    if(card->current_record_buffer == NULL || !card->is_recording) {
        return;
    }

    /* Pick up the controller's writes before the CPU reads them */
    hda_stream_sync(card->hda_ctrl, card->input_stream,
                    (card->current_record_buffer == card->record_buffer1) ? 0 : 1);

    {
        struct AHIRecordMessage rm = {
            AHIST_S16S,
            card->current_record_buffer,
            RECORD_BUFFER_SAMPLES
        };
#ifdef __AMIGAOS4__
        int i = 0;
        int frames = card->current_record_bytesize / 2;
        WORD *src = card->current_record_buffer;
        WORD *dst = card->current_record_buffer;

        while(i < frames) {
            *dst = ((*src & 0x00FF) << 8) | ((*src & 0xFF00) >> 8);

            ++i;
            ++src;
            ++dst;
        }
#endif

        CallHookPkt(AudioCtrl->ahiac_SamplerFunc, (Object *) AudioCtrl, &rm);
    }
}
