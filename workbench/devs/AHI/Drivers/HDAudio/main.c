/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2010-2026 The AROS Dev Team
(C) Copyright 2009-2010 Stephen Jones.
(C) Copyright xxxx-2009 Davy Wentzler.

The Initial Developer of the Original Code is Davy Wentzler.

All Rights Reserved.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <config.h>

#include <devices/ahi.h>
#include <exec/memory.h>
#include <libraries/ahi_sub.h>
#include <math.h>

#include <proto/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#include <string.h>

#include "library.h"
#include "misc.h"
#include "hda_hidd.h"


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

static const char *Inputs[INPUTS] = {
    "Line in",
    "Mic 1",
    "Mic 2",
    "CD",
    "Mixer"
};


#define OUTPUTS 1


static const char *Outputs[OUTPUTS] = {
    "Line",
};



#define uint32 unsigned int

static const ULONG nr_of_playback_buffers = 2;

/******************************************************************************
** AHIsub_AllocAudio **********************************************************
******************************************************************************/

ULONG _AHIsub_AllocAudio(struct TagItem *taglist,
                         struct AHIAudioCtrlDrv *AudioCtrl,
                         struct DriverBase *AHIsubBase)
{
    struct HDAudioBase *card_base = (struct HDAudioBase *) AHIsubBase;

    int   card_num;
    ULONG ret;
    ULONG i;

    card_num = (GetTagData(AHIDB_AudioID, 0, taglist) & 0x0000f000) >> 12;

    if(card_num >= card_base->cards_found ||
            card_base->driverdatas[card_num] == NULL) {
        D(bug("[HDAudio] no data for card = %ld\n", card_num));
        Req("No HDAudioChip for card %ld.", card_num);
        return AHISF_ERROR;
    } else {
        BOOL in_use;
        struct HDAudioChip *card;

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

        //bug("AudioCtrl->ahiac_MixFreq = %lu\n", AudioCtrl->ahiac_MixFreq);
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

        //bug("card->selected_freq_index = %lu\n", card->selected_freq_index);

        ret = AHISF_KNOWSTEREO | AHISF_MIXING | AHISF_TIMING;

        if(card->adc_nid != 0) {
            for(i = 0; i < card->nr_of_frequencies; i++) {
                if(AudioCtrl->ahiac_MixFreq == card->frequencies[i].frequency) {
                    ret |= AHISF_CANRECORD;
                    break;
                }
            }
        }

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
    struct HDAudioBase *card_base = (struct HDAudioBase *) AHIsubBase;
    struct HDAudioChip *card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;

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
    struct HDAudioChip *card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;
    ULONG dma_buffer_size = 0;

    if(flags & AHISF_PLAY) {
        ULONG dma_sample_frame_size = 2; /* 16-bit */;

        detect_headphone_change(card);

        card->mix_buffer = AllocVec(AudioCtrl->ahiac_BuffSize, MEMF_PUBLIC | MEMF_CLEAR);
        if(card->mix_buffer == NULL) {
            D(bug("[HDAudio] Unable to allocate %ld bytes for mixing buffer.", AudioCtrl->ahiac_BuffSize));
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
            D(bug("[HDAudio] No free output stream!\n"));
            return AHIE_UNKNOWN;
        }

        if(!hda_stream_setup(card->hda_ctrl, card->output_stream,
                             get_hda_format(card), dma_buffer_size,
                             nr_of_playback_buffers, &card->output_info)) {
            D(bug("[HDAudio] Output stream setup failed!\n"));
            hda_stream_free(card->hda_ctrl, card->output_stream);
            card->output_stream = NULL;
            return AHIE_UNKNOWN;
        }

        card->playback_buffer1 = card->output_info.si_Buffers[0];
        card->playback_buffer2 = card->output_info.si_Buffers[1];

        send_command_4(card->codecnr, card->dac_nid, VERB_SET_CONVERTER_FORMAT, get_hda_format(card), card);

        // set stream ID and channel for DAC
        send_command_12(card->codecnr, card->dac_nid, VERB_SET_CONVERTER_STREAM_CHANNEL,
                        (card->output_info.si_Tag << 4), card); // channel 0

        card->current_bytesize = dma_buffer_size;
        card->current_frames = AudioCtrl->ahiac_MaxBuffSamples;
        card->current_buffer   = card->playback_buffer1;
        card->flip = 1;
        card->is_playing = TRUE;
    }

    if(flags & AHISF_RECORD) {
        UWORD record_format;

        if(card->adc_nid == 0) {
            return AHIE_UNKNOWN;
        }

        dma_buffer_size = RECORD_BUFFER_SAMPLES * 4;

        record_format =
            ((card->frequencies[card->selected_freq_index].base44100 > 0) ? BASE44 : 0) | // base freq: 48 or 44.1 kHz
            (card->frequencies[card->selected_freq_index].mult << 11) | // multiplier
            (card->frequencies[card->selected_freq_index].div << 8) | // divisor
            FORMAT_16BITS | // set to 16-bit for now
            FORMAT_STEREO;

        card->input_stream = hda_stream_alloc(card->hda_ctrl,
                                              vHidd_HDA_StreamDir_In,
                                              &card->record_interrupt);
        if(card->input_stream == NULL) {
            D(bug("[HDAudio] No free input stream!\n"));
            return AHIE_UNKNOWN;
        }

        if(!hda_stream_setup(card->hda_ctrl, card->input_stream,
                             record_format, dma_buffer_size, 2,
                             &card->input_info)) {
            D(bug("[HDAudio] Input stream setup failed!\n"));
            hda_stream_free(card->hda_ctrl, card->input_stream);
            card->input_stream = NULL;
            return AHIE_UNKNOWN;
        }

        card->record_buffer1 = card->input_info.si_Buffers[0];
        card->record_buffer2 = card->input_info.si_Buffers[1];

        send_command_4(card->codecnr, card->adc_nid, VERB_SET_CONVERTER_FORMAT,
                       record_format, card);

        // set stream ID and channel for ADC
        send_command_12(card->codecnr, card->adc_nid, VERB_SET_CONVERTER_STREAM_CHANNEL,
                        (card->input_info.si_Tag << 4), card);

        D(bug("[HDAudio] RECORD\n"));

        card->current_record_bytesize = dma_buffer_size;
        card->current_record_buffer = card->record_buffer1;
        card->recflip = 1;
        card->is_recording = TRUE;
    }

    if(flags & AHISF_PLAY) {
        hda_stream_start(card->hda_ctrl, card->output_stream);
    }

    if(flags & AHISF_RECORD) {
        hda_stream_start(card->hda_ctrl, card->input_stream);
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
    struct HDAudioChip *card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;

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
    struct HDAudioChip *card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;

    //bug("Stop\n");

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

    if((flags & AHISF_RECORD) && card->is_recording) {
        card->is_recording = FALSE;

        hda_stream_stop(card->hda_ctrl, card->input_stream);
        hda_stream_free(card->hda_ctrl, card->input_stream);
        card->input_stream = NULL;

        card->record_buffer1 = NULL;
        card->record_buffer2 = NULL;
        card->current_record_buffer = NULL;
        card->current_record_bytesize = 0;
    }

    card->current_bytesize = 0;
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
    struct HDAudioBase *card_base = (struct HDAudioBase *) AHIsubBase;
    struct HDAudioChip *card = NULL;
    ULONG i;

    if(AudioCtrl != NULL) {
        card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;
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
        return (IPTR) "Davy Wentzler";

    case AHIDB_Copyright:
        return (IPTR) "(C) 2010 Stephen Jones, (C) 2010-2026 The AROS Dev Team";

    case AHIDB_Version:
        return (IPTR) LibIDString;

    case AHIDB_Annotation:
        return (IPTR) "HD Audio driver";

    case AHIDB_Record:
        return (card->adc_nid != 0);

    case AHIDB_FullDuplex:
        return (card->adc_nid != 0);

    case AHIDB_Realtime:
        return TRUE;

    case AHIDB_MaxRecordSamples:
        return RECORD_BUFFER_SAMPLES;

    /* formula's:
    #include <math.h>

    unsigned long res = (unsigned long) (0x10000 * pow (10.0, dB / 20.0));
    double dB = 20.0 * log10(0xVALUE / 65536.0);

    printf("dB = %f, res = %lx\n", dB, res);*/

    case AHIDB_MinMonitorVolume:
        return (unsigned long)(0x10000 * pow(10.0, -34.5 / 20.0));   // -34.5 dB

    case AHIDB_MaxMonitorVolume:
        return (unsigned long)(0x10000 * pow(10.0, 12.0 / 20.0));   // 12 dB

    case AHIDB_MinInputGain:
        return (unsigned long)(0x10000 * pow(10.0, card->adc_min_gain / 20.0));

    case AHIDB_MaxInputGain:
        return (unsigned long)(0x10000 * pow(10.0, card->adc_max_gain / 20.0));

    case AHIDB_MinOutputVolume:
        return (unsigned long)(0x10000 * pow(10.0, card->dac_min_gain / 20.0));

    case AHIDB_MaxOutputVolume:
        return (unsigned long)(0x10000 * pow(10.0, card->dac_max_gain / 20.0));

    case AHIDB_Inputs:
        return (card->adc_nid != 0) ? INPUTS : 0;

    case AHIDB_Input:
        return (IPTR) Inputs[argument];

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
    struct HDAudioBase *card_base = (struct HDAudioBase *) AHIsubBase;
    struct HDAudioChip *card = NULL;

    if(AudioCtrl != NULL) {
        card = (struct HDAudioChip *) AudioCtrl->ahiac_DriverData;
    }
    if(card == NULL) {
        card = card_base->driverdatas[0];
    }

    switch(attribute) {
    case AHIC_MonitorVolume: {
        double dB = 20.0 * log10(argument / 65536.0);

        card->monitor_volume = argument;
        set_monitor_volumes(card, dB);

        return TRUE;
    }

    case AHIC_MonitorVolume_Query: {
        return card->monitor_volume;
    }

    case AHIC_InputGain: {
        double dB = 20.0 * log10(argument / 65536.0);
        card->input_gain = argument;

        set_adc_gain(card, dB);
        return TRUE;
    }

    case AHIC_InputGain_Query:
        return card->input_gain;

    case AHIC_OutputVolume: {
        double dB = 20.0 * log10(argument / 65536.0);
        card->output_volume = argument;

        set_dac_gain(card, dB);

        return TRUE;
    }

    case AHIC_OutputVolume_Query:
        return card->output_volume;

    case AHIC_Input:
        card->input = argument;
        set_adc_input(card);

        return TRUE;

    case AHIC_Input_Query:
        return card->input;

    case AHIC_Output:
        card->output = argument;

        return TRUE;

    case AHIC_Output_Query:
        return card->output;

    default:
        return FALSE;
    }
}


