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

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "library.h"
#include "hdmi.h"
#include "hda_hidd.h"


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

#define OUTPUTS 1

static const char *Outputs[OUTPUTS] = {
    "HDMI",
};

static const ULONG nr_of_playback_buffers = 2;

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG _AHIsub_AllocAudio(struct TagItem *taglist,
                         struct AHIAudioCtrlDrv *AudioCtrl,
                         struct DriverBase *AHIsubBase)
{
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) AHIsubBase;

    int   card_num;
    ULONG ret;
    ULONG i;

    card_num = (GetTagData(AHIDB_AudioID, 0, taglist) & 0x0000f000) >> 12;

    if(card_num >= card_base->cards_found ||
            card_base->driverdatas[card_num] == NULL) {
        D(bug("[NVHDMI] no data for card = %ld\n", card_num));
        Req("No NVHDMIChip for card %ld.", card_num);
        return AHISF_ERROR;
    } else {
        BOOL in_use;
        struct NVHDMIChip *card;

        card  = card_base->driverdatas[card_num];
        AudioCtrl->ahiac_DriverData = card;

        ObtainSemaphore(&card_base->semaphore);
        in_use = (card->audioctrl != NULL);
        if(!in_use) {
            card->audioctrl = AudioCtrl;
        }
        ReleaseSemaphore(&card_base->semaphore);

        if(in_use) {
            return AHISF_ERROR;
        }

        if(AudioCtrl->ahiac_MixFreq < card->frequencies[0].frequency)
            AudioCtrl->ahiac_MixFreq = card->frequencies[0].frequency;

        card->selected_freq_index = 0;
        for(i = 1; i < card->nr_of_frequencies; i++) {
            if((ULONG) card->frequencies[i].frequency >= AudioCtrl->ahiac_MixFreq) {
                if((AudioCtrl->ahiac_MixFreq - (LONG) card->frequencies[i - 1].frequency)
                        < ((LONG) card->frequencies[i].frequency - AudioCtrl->ahiac_MixFreq)) {
                    card->selected_freq_index = i - 1;
                    break;
                } else {
                    card->selected_freq_index = i;
                    break;
                }
            }
        }

        ret = AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;

        if(card->bitsizes[card->selected_bitsize_index] == 24)
            ret |= AHISF_KNOWHIFI;

        return ret;
    }
}



/******************************************************************************
** AHIsub_FreeAudio ***********************************************************
******************************************************************************/

void _AHIsub_FreeAudio(struct AHIAudioCtrlDrv *AudioCtrl,
                       struct DriverBase *AHIsubBase)
{
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) AHIsubBase;
    struct NVHDMIChip *card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;

    if(card != NULL) {
        ObtainSemaphore(&card_base->semaphore);
        if(card->audioctrl == AudioCtrl) {
            // Release it if we own it.
            card->audioctrl = NULL;
        }

        ReleaseSemaphore(&card_base->semaphore);

        AudioCtrl->ahiac_DriverData = NULL;
    }
}


/******************************************************************************
** AHIsub_Disable *************************************************************
******************************************************************************/

void _AHIsub_Disable(struct AHIAudioCtrlDrv *AudioCtrl,
                     struct DriverBase *AHIsubBase)
{
    // V6 drivers do not have to preserve all registers

    Disable();
}


/******************************************************************************
** AHIsub_Enable **************************************************************
******************************************************************************/

void _AHIsub_Enable(struct AHIAudioCtrlDrv *AudioCtrl,
                    struct DriverBase *AHIsubBase)
{
    // V6 drivers do not have to preserve all registers

    Enable();
}


/******************************************************************************
** AHIsub_Start ***************************************************************
******************************************************************************/

ULONG _AHIsub_Start(ULONG flags,
                    struct AHIAudioCtrlDrv *AudioCtrl,
                    struct DriverBase *AHIsubBase)
{
    struct NVHDMIChip *card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;
    ULONG dma_buffer_size = 0;

    if(flags & AHISF_PLAY) {
        ULONG dma_sample_frame_size = 2; /* 16-bit */;

        card->mix_buffer = AllocVec(AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR);
        if(card->mix_buffer == NULL) {
            D(bug("[NVHDMI] Unable to allocate %ld bytes for mixing buffer.", AudioCtrl->ahiac_BuffSize));
            return AHIE_NOMEM;
        }

        /* Allocate a buffer large enough for 16-bit or 32-bit double-buffered playback (mono or stereo) */
        if(card->bitsizes[card->selected_bitsize_index] == 24)
            dma_sample_frame_size = 4; /* 32-bit */

        if(AudioCtrl->ahiac_Flags & AHIACF_STEREO)
            dma_sample_frame_size *= 2; /* STEREO */

        dma_buffer_size = AudioCtrl->ahiac_MaxBuffSamples * dma_sample_frame_size;

        D(bug("dma_sample_frame_size = %ld, dma_buffer_size = %ld, freq = %d\n",
              dma_sample_frame_size, dma_buffer_size, AudioCtrl->ahiac_MixFreq));

        card->output_stream = hda_stream_alloc(card->hda_ctrl,
                                               vHidd_HDA_StreamDir_Out,
                                               &card->playback_interrupt);
        if(card->output_stream == NULL) {
            D(bug("[NVHDMI] No free output stream!\n"));
            return AHIE_UNKNOWN;
        }

        if(!hda_stream_setup(card->hda_ctrl, card->output_stream,
                             get_hda_format(card), dma_buffer_size,
                             nr_of_playback_buffers, &card->output_info)) {
            D(bug("[NVHDMI] Output stream setup failed!\n"));
            hda_stream_free(card->hda_ctrl, card->output_stream);
            card->output_stream = NULL;
            return AHIE_UNKNOWN;
        }

        card->playback_buffer1 = card->output_info.si_Buffers[0];
        card->playback_buffer2 = card->output_info.si_Buffers[1];

        send_command_4(card->codecnr, card->dac_nid, VERB_SET_CONVERTER_FORMAT, get_hda_format(card), card);

        // set stream ID and channel for the converter
        send_command_12(card->codecnr, card->dac_nid, VERB_SET_CONVERTER_STREAM_CHANNEL,
                        (card->output_info.si_Tag << 4), card); // channel 0

        card->current_bytesize = dma_buffer_size;
        card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
        card->current_buffer   = card->playback_buffer1;
        card->flip = 1;
        card->is_playing = TRUE;

        hda_stream_start(card->hda_ctrl, card->output_stream);
    }

    if(flags & AHISF_RECORD) {
        return AHIE_UNKNOWN;
    }

    return AHIE_OK;
}


/******************************************************************************
** AHIsub_Update **************************************************************
******************************************************************************/

void _AHIsub_Update(ULONG flags,
                    struct AHIAudioCtrlDrv *AudioCtrl,
                    struct DriverBase *AHIsubBase)
{
    struct NVHDMIChip *card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;

    card->current_frames = AudioCtrl->ahiac_BuffSamples;

    if(AudioCtrl->ahiac_Flags & AHIACF_STEREO) {
        card->current_bytesize = card->current_frames * 4;
    } else {
        card->current_bytesize = card->current_frames * 2;
    }
}


/******************************************************************************
** AHIsub_Stop ****************************************************************
******************************************************************************/

void _AHIsub_Stop(ULONG flags,
                  struct AHIAudioCtrlDrv *AudioCtrl,
                  struct DriverBase *AHIsubBase)
{
    struct NVHDMIChip *card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;

    if((flags & AHISF_PLAY) && card->is_playing) {
        card->is_playing = FALSE;

        hda_stream_stop(card->hda_ctrl, card->output_stream);
        hda_stream_free(card->hda_ctrl, card->output_stream);
        card->output_stream = NULL;

        card->current_bytesize = 0;
        card->current_frames = 0;
        card->current_buffer = NULL;
        card->playback_buffer1 = NULL;
        card->playback_buffer2 = NULL;

        if(card->mix_buffer) {
            FreeVec(card->mix_buffer);
        }
        card->mix_buffer = NULL;
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
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) AHIsubBase;
    struct NVHDMIChip *card = NULL;
    ULONG i;

    if(AudioCtrl != NULL) {
        card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;
    }
    if(card == NULL) {
        card = card_base->driverdatas[0];
    }

    switch(attribute) {
    case AHIDB_Bits:
        return 16;

    case AHIDB_Frequencies: {
        return card->nr_of_frequencies;
    }

    case AHIDB_Frequency: // Index->Frequency
        return (LONG) card->frequencies[argument].frequency;

    case AHIDB_Index: { // Frequency->Index
        if(argument <= (LONG) card->frequencies[0].frequency) {
            return 0;
        }

        if(argument >= (LONG) card->frequencies[card->nr_of_frequencies - 1].frequency) {
            return card->nr_of_frequencies - 1;
        }

        for(i = 1; i < card->nr_of_frequencies; i++) {
            if((LONG) card->frequencies[i].frequency > argument) {
                if((argument - (LONG) card->frequencies[i - 1].frequency) < ((LONG) card->frequencies[i].frequency - argument)) {
                    return (IPTR)i - 1;
                } else {
                    return (IPTR)i;
                }
            }
        }

        return 0;  // Will not happen
    }

    case AHIDB_Author:
        return (IPTR) "The AROS Dev Team";

    case AHIDB_Copyright:
        return (IPTR) "(C) 2026 The AROS Dev Team";

    case AHIDB_Version:
        return (IPTR) LibIDString;

    case AHIDB_Annotation:
        return (IPTR) "NVidia HDMI/DP audio driver";

    case AHIDB_Record:
        return FALSE;

    case AHIDB_FullDuplex:
        return FALSE;

    case AHIDB_Realtime:
        return TRUE;

    case AHIDB_MaxRecordSamples:
        return 0;

    case AHIDB_MinOutputVolume:
        return 0x10000; // 0dB, not adjustable

    case AHIDB_MaxOutputVolume:
        return 0x10000; // 0dB

    case AHIDB_Inputs:
        return 0;

    case AHIDB_Outputs:
        return OUTPUTS;

    case AHIDB_Output:
        return (IPTR) Outputs[argument];

    default:
        return def;
    }
}


/******************************************************************************
** AHIsub_HardwareControl *****************************************************
******************************************************************************/

ULONG _AHIsub_HardwareControl(ULONG attribute,
                              LONG argument,
                              struct AHIAudioCtrlDrv *AudioCtrl,
                              struct DriverBase *AHIsubBase)
{
    struct NVHDMIBase *card_base = (struct NVHDMIBase *) AHIsubBase;
    struct NVHDMIChip *card = NULL;

    if(AudioCtrl != NULL) {
        card = (struct NVHDMIChip *) AudioCtrl->ahiac_DriverData;
    }
    if(card == NULL) {
        card = card_base->driverdatas[0];
    }

    switch(attribute) {
    case AHIC_OutputVolume:
        // The digital output has no volume control
        return TRUE;

    case AHIC_OutputVolume_Query:
        return card->output_volume;

    case AHIC_Output:
        return TRUE;

    case AHIC_Output_Query:
        return 0;

    default:
        return FALSE;
    }
}
