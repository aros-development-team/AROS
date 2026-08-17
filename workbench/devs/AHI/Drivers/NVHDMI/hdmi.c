/*
The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

(C) Copyright 2026 The AROS Dev Team.

All Rights Reserved.

HDMI/DP display-audio codec setup for NVidia GPU audio functions.
The codec follows the HD Audio specification; audio reaches the
monitor over the display cable. There are no amplifiers, no ADCs
and no jacks - just digital converters routed to HDMI/DP pins.
*/

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include <config.h>

#include <exec/memory.h>
#include <devices/timer.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include "library.h"
#include "interrupt.h"
#include "hdmi.h"
#include "hda_hidd.h"

#define MAX_NODES 32

int card_init(struct NVHDMIChip *card);
static void determine_frequencies(struct NVHDMIChip *card);
static void determine_bitsizes(struct NVHDMIChip *card);
static void set_frequency_info(struct Freq *freq, UWORD bitnr);

struct Device *TimerBase = NULL;

#ifdef __AROS__
#define DebugPrintF bug
INTGW(static, void,  playbackinterrupt, PlaybackInterrupt);
#endif

void micro_delay(unsigned int val)
{
    struct MsgPort *replymp;
    struct timerequest *TimerIO;

    replymp = (struct MsgPort *) CreateMsgPort();
    if(!replymp) {
        D(bug("[NVHDMI] Could not create the reply port!\n"));
        return;
    }

    TimerIO = (struct timerequest *) CreateIORequest(replymp, sizeof(struct timerequest));

    if(TimerIO == NULL) {
        D(bug("[NVHDMI] Out of memory.\n"));
        DeleteMsgPort(replymp);
        return;
    }

    if(OpenDevice((CONST_STRPTR) "timer.device", UNIT_MICROHZ, (struct IORequest *) TimerIO, 0) != 0) {
        D(bug("[NVHDMI] Unable to open 'timer.device'.\n"));
        DeleteIORequest((struct IORequest *) TimerIO);
        DeleteMsgPort(replymp);
        return;
    }

    TimerBase = (struct Device *) TimerIO->tr_node.io_Device;

    TimerIO->tr_node.io_Command = TR_ADDREQUEST;
    TimerIO->tr_time.tv_secs = 0;
    TimerIO->tr_time.tv_micro = val;
    DoIO((struct IORequest *) TimerIO);
    CloseDevice((struct IORequest *) TimerIO);
    DeleteIORequest((struct IORequest *) TimerIO);
    DeleteMsgPort(replymp);
}


ULONG get_parameter(UBYTE node, UBYTE parameter, struct NVHDMIChip *card)
{
    return send_command_12(card->codecnr, node, VERB_GET_PARMS, parameter, card);
}


ULONG send_command_4(UBYTE codec, UBYTE node, UBYTE verb, UWORD payload, struct NVHDMIChip *card)
{
    return hda_command(card->hda_ctrl, HDA_COMMAND_4(codec, node, verb, payload));
}


ULONG send_command_12(UBYTE codec, UBYTE node, UWORD verb, UBYTE payload, struct NVHDMIChip *card)
{
    return hda_command(card->hda_ctrl, HDA_COMMAND_12(codec, node, verb, payload));
}


/******************************************************************************
** DriverData allocation ******************************************************
******************************************************************************/

struct NVHDMIChip *AllocDriverData(APTR controller, struct DriverBase *AHIsubBase)
{
    struct NVHDMIChip *card;

    D(bug("[NVHDMI] %s()\n", __func__));

    card = (struct NVHDMIChip *) AllocVec(sizeof(struct NVHDMIChip), MEMF_PUBLIC | MEMF_CLEAR);

    if(card == NULL) {
        Req("Unable to allocate driver structure.");
        return NULL;
    }

    card->ahisubbase = AHIsubBase;

    card->playback_interrupt.is_Node.ln_Type = IRQTYPE;
    card->playback_interrupt.is_Node.ln_Pri  = 0;
    card->playback_interrupt.is_Node.ln_Name = (char *) LibName;
#ifdef __AROS__
    card->playback_interrupt.is_Code         = (APTR)&playbackinterrupt;
#else
    card->playback_interrupt.is_Code         = PlaybackInterrupt;
#endif
    card->playback_interrupt.is_Data         = (APTR) card;

    card->hda_ctrl = controller;

    if(card_init(card) < 0) {
        D(bug("[NVHDMI] Unable to initialize the codec.\n"));
        FreeDriverData(card, AHIsubBase);
        return NULL;
    }

    card->card_initialized = TRUE;
    card->output_volume = 0x10000; // 0dB, not adjustable

    return card;
}


void FreeDriverData(struct NVHDMIChip *card, struct DriverBase *AHIsubBase)
{
    if(card != NULL) {
        if(card->hda_ctrl != NULL) {
            ahi_hda_release_controller(card->hda_ctrl);
        }

        if(card->frequencies) {
            FreeVec(card->frequencies);
        }
        if(card->bitsizes) {
            FreeVec(card->bitsizes);
        }

        FreeVec(card);
    }
}


/******************************************************************************
** Codec setup ****************************************************************
******************************************************************************/

static BOOL power_up_codec(struct NVHDMIChip *card)
{
    ULONG node_count_response = get_parameter(0, VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE node_count = node_count_response & 0xFF;
    UBYTE starting_node = (node_count_response >> 16) & 0xFF;
    BOOL audio_found = FALSE;
    int i, j;

    for(i = 0; i < node_count && !audio_found; i++) {
        ULONG function_group_response = get_parameter(starting_node + i, VERB_GET_PARMS_FUNCTION_GROUP_TYPE, card);
        UBYTE function_group = function_group_response & 0xFF;

        if(function_group == AUDIO_FUNCTION) {
            ULONG subnode_count_response = get_parameter(starting_node + i, VERB_GET_PARMS_NODE_COUNT, card);
            UBYTE subnode_count = subnode_count_response & 0xFF;
            UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;

            audio_found = TRUE;
            card->function_group = starting_node + i;

            send_command_12(card->codecnr, card->function_group, VERB_SET_POWER_STATE, 0, card);
            udelay(20000);

            for(j = 0; j < subnode_count; j++) {
                const UBYTE NID = j + sub_starting_node;
                ULONG widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

                if(AUDIO_WIDGET_POWER_CONTROL(widget_caps) == 1) {
                    send_command_12(card->codecnr, NID, VERB_SET_POWER_STATE, 0, card);
                }
            }
        }
    }

    return audio_found;
}


static UBYTE read_eld_byte(struct NVHDMIChip *card, UBYTE offset, BOOL *valid)
{
    ULONG response = send_command_12(card->codecnr, card->pin_nid,
                                     VERB_GET_HDMI_ELD_DATA, offset, card);

    if(valid) {
        *valid = (response & 0x80000000) ? TRUE : FALSE;
    }

    return (UBYTE)(response & 0xFF);
}


/* Program the audio infoframe (2 channel LPCM) into the pin's Data
   Island Packet buffer and enable its transmission. */
static void setup_infoframe(struct NVHDMIChip *card)
{
    UBYTE frame[10];
    ULONG size, i;

    if(card->pin_is_dp) {
        /* DisplayPort audio infoframe */
        frame[0] = 0x84;
        frame[1] = 0x1B;
        frame[2] = 0x11 << 2;
        frame[3] = 0x01;    /* CC = 1 -> 2 channels */
        frame[4] = 0x00;
        frame[5] = 0x00;
        frame[6] = 0x00;    /* CA = 0 -> FL/FR */
        frame[7] = 0x00;
        size = 8;
    } else {
        /* HDMI audio infoframe with checksum */
        ULONG sum = 0;

        frame[0] = 0x84;
        frame[1] = 0x01;
        frame[2] = 0x0A;
        frame[3] = 0x00;    /* checksum, computed below */
        frame[4] = 0x01;    /* CC = 1 -> 2 channels */
        frame[5] = 0x00;
        frame[6] = 0x00;
        frame[7] = 0x00;    /* CA = 0 -> FL/FR */
        frame[8] = 0x00;
        size = 9;

        for(i = 0; i < size; i++) {
            sum += frame[i];
        }
        frame[3] = (UBYTE)(0x100 - (sum & 0xFF));
    }

    /* Packet index 0, byte index 0, then write the bytes (the byte
       index auto-increments) and enable transmission */
    send_command_12(card->codecnr, card->pin_nid, VERB_SET_HDMI_DIP_INDEX, 0, card);
    for(i = 0; i < size; i++) {
        send_command_12(card->codecnr, card->pin_nid, VERB_SET_HDMI_DIP_DATA, frame[i], card);
    }
    send_command_12(card->codecnr, card->pin_nid, VERB_SET_HDMI_DIP_XMIT, DIP_XMIT_BEST, card);
}


BOOL setup_hdmi_codec(struct NVHDMIChip *card)
{
    ULONG subnode_count_response = get_parameter(card->function_group,
                                                 VERB_GET_PARMS_NODE_COUNT, card);
    UBYTE subnode_count = subnode_count_response & 0xFF;
    UBYTE sub_starting_node = (subnode_count_response >> 16) & 0xFF;
    UBYTE converters[MAX_NODES], pins[MAX_NODES];
    ULONG pin_caps[MAX_NODES];
    UWORD nr_converters = 0, nr_pins = 0;
    UBYTE pin = 0, converter = 0;
    ULONG connections, entry;
    UWORD i, j;
    BOOL pin_has_presence = FALSE;
    ULONG chosen_caps = 0;

    /* Collect the digital output converters and the HDMI/DP pins */
    for(i = 0; i < subnode_count; i++) {
        const UBYTE NID = i + sub_starting_node;
        ULONG widget_caps = get_parameter(NID, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);
        UBYTE type = AUDIO_WIDGET_CAPS(widget_caps);

        if(type == 0x0 && AUDIO_WIDGET_DIGITAL(widget_caps)) {
            if(nr_converters < MAX_NODES) {
                converters[nr_converters++] = NID;
            }
        } else if(type == 0x4) {
            ULONG caps = get_parameter(NID, VERB_GET_PARMS_PIN_CAPS, card);

            if((caps & (PIN_CAPS_HDMI | PIN_CAPS_DP)) &&
               (caps & PIN_CAPS_OUTPUT_CAPABLE) &&
               nr_pins < MAX_NODES) {
                pin_caps[nr_pins] = caps;
                pins[nr_pins++] = NID;
            }
        }
    }

    D(bug("[NVHDMI] Found %u digital converters, %u HDMI/DP pins\n",
          nr_converters, nr_pins));

    if(nr_converters == 0 || nr_pins == 0) {
        bug("[NVHDMI] No digital converter/pin pair found!\n");
        return FALSE;
    }

    /* Choose the pin the monitor is connected to. The display driver
       must have brought the link up for presence to be reported. */
    for(i = 0; i < nr_pins; i++) {
        ULONG sense;

        send_command_12(card->codecnr, pins[i], VERB_EXECUTE_PIN_SENSE, 0, card);
        udelay(2000);
        sense = send_command_12(card->codecnr, pins[i], VERB_GET_PIN_SENSE, 0, card);

        D(bug("[NVHDMI] Pin %xh sense = %08lx\n", pins[i], sense));

        if(sense & PIN_SENSE_PRESENCE) {
            pin = pins[i];
            pin_has_presence = TRUE;
            chosen_caps = pin_caps[i];

            if(sense & PIN_SENSE_ELD_VALID) {
                /* Monitor with a valid ELD - best candidate */
                break;
            }
        }
    }

    if(pin == 0) {
        D(bug("[NVHDMI] No pin reports presence, using first pin\n"));
        pin = pins[0];
        chosen_caps = pin_caps[0];
    }
    card->pin_nid = pin;

    /* Route: find a digital converter in the pin's connection list */
    connections = get_parameter(pin, VERB_GET_PARMS_CONNECTION_LIST_LENGTH,
                                card) & 0x7F;
    for(entry = 0; entry < connections && converter == 0; entry++) {
        ULONG list = send_command_12(card->codecnr, pin,
                                     VERB_GET_CONNECTION_LIST_ENTRY,
                                     entry & ~3, card);
        UBYTE connected = (list >> ((entry % 4) * 8)) & 0xFF;

        for(j = 0; j < nr_converters; j++) {
            if(converters[j] == connected) {
                converter = connected;
                send_command_12(card->codecnr, pin, VERB_SET_CONNECTION_SELECT,
                                entry, card);
                break;
            }
        }
    }

    if(converter == 0) {
        bug("[NVHDMI] Pin %xh has no digital converter input!\n", pin);
        return FALSE;
    }

    card->dac_nid = converter;

    D(bug("[NVHDMI] Using pin %xh, converter %xh%s\n", pin, converter,
          pin_has_presence ? "" : " (no presence!)"));

    /* Power up the chosen path */
    send_command_12(card->codecnr, pin, VERB_SET_POWER_STATE, 0, card);
    send_command_12(card->codecnr, converter, VERB_SET_POWER_STATE, 0, card);

    /* Connection type from the ELD, to pick the infoframe variant
       (ELD byte 5, bits 3:2 - 0 = HDMI, 1 = DisplayPort) */
    card->pin_is_dp = FALSE;
    {
        BOOL eld_valid = FALSE;
        UBYTE conn = read_eld_byte(card, 5, &eld_valid);

        if(eld_valid && ((conn >> 2) & 0x3) == 1) {
            card->pin_is_dp = TRUE;
        } else if(!eld_valid && (chosen_caps & PIN_CAPS_DP) &&
                  !(chosen_caps & PIN_CAPS_HDMI)) {
            card->pin_is_dp = TRUE;
        }
    }
    D(bug("[NVHDMI] Sink type: %s\n", card->pin_is_dp ? "DisplayPort" : "HDMI"));

    /* Unmute the pin's output amplifier, if it has one */
    {
        ULONG widget_caps = get_parameter(pin, VERB_GET_PARMS_AUDIO_WIDGET_CAPS, card);

        if(widget_caps & 0x4) { /* OutAmpPre */
            send_command_4(card->codecnr, pin, VERB_SET_AMP_GAIN,
                           OUTPUT_AMP_GAIN | AMP_GAIN_LR, card);
        }
    }

    /* Enable the pin output */
    send_command_12(card->codecnr, pin, VERB_SET_PIN_WIDGET_CONTROL, 0x40, card);

    /* Enable the digital converter, stereo */
    send_command_12(card->codecnr, converter, VERB_SET_DIGITAL_CONVERTER_1,
                    DIGITAL_CONVERTER_DIGEN, card);
    send_command_12(card->codecnr, converter, VERB_SET_CONVERTER_CHANNEL_COUNT,
                    1, card); /* 2 channels */

    /* Map ASP slots 0/1 to channels 0/1 */
    send_command_12(card->codecnr, converter, VERB_SET_ASP_CHANNEL_SLOT, 0x00, card);
    send_command_12(card->codecnr, converter, VERB_SET_ASP_CHANNEL_SLOT, 0x11, card);

    setup_infoframe(card);

    return TRUE;
}


int card_init(struct NVHDMIChip *card)
{
    int i;
    BOOL codec_found = FALSE;

    card->codecbits = (UWORD)ahi_hda_get_attr(card->hda_ctrl,
                                              aHidd_HDA_CodecMask);

    if(card->codecbits == 0) {
        D(bug("[NVHDMI] No codecs found!\n"));
        return -1;
    }

    D(bug("[NVHDMI] codecs %08x\n", card->codecbits));

    /* Find the first codec with an audio function group */
    for(i = 0; i < 16; i++) {
        if(card->codecbits & (1 << i)) {
            card->codecnr = i;
            if(power_up_codec(card)) {
                codec_found = TRUE;
                break;
            }
        }
    }

    if(!codec_found) {
        D(bug("[NVHDMI] No audio function group found!\n"));
        return -1;
    }

    D(
        ULONG vendor_device = get_parameter(0x0, VERB_GET_PARMS_VENDOR_DEVICE, card);
        bug("[NVHDMI] codec %u: vendor = %x, device = %x\n", card->codecnr,
            (vendor_device >> 16), (vendor_device & 0xFFFF));
    )

    if(!setup_hdmi_codec(card)) {
        return -1;
    }

    determine_frequencies(card);
    determine_bitsizes(card);

    if(card->nr_of_frequencies == 0 || card->nr_of_bitsizes == 0) {
        D(bug("[NVHDMI] No usable sample rates/sizes!\n"));
        return -1;
    }

    D(bug("[NVHDMI] card_init() was a success!\n"));

    return 0;
}


/******************************************************************************
** Format handling ************************************************************
******************************************************************************/

static void determine_bitsizes(struct NVHDMIChip *card)
{
    ULONG verb = get_parameter(card->dac_nid, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card);
    UWORD bitsize_flags = (verb & PCM_SIZE_RATE_BITSIZE_MASK) >> 16;
    int i;
    ULONG bitsizes = 0;

    if(bitsize_flags == 0) {
        verb = get_parameter(card->function_group, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card);
        bitsize_flags = (verb & PCM_SIZE_RATE_BITSIZE_MASK) >> 16;
        D(bug("[NVHDMI] converter has no sample size list, trying AFG node\n"));
    }

    for(i = 0; i < 5; i++) {
        if(bitsize_flags & (1 << i)) {
            bitsizes++;
        }
    }

    D(bug("[NVHDMI] Bitsizes found = %lu\n", bitsizes));
    card->bitsizes = (ULONG *) AllocVec(sizeof(ULONG) * bitsizes, MEMF_PUBLIC | MEMF_CLEAR);
    card->nr_of_bitsizes = bitsizes;

    i = 0;
    if(bitsize_flags & 0x0001)
        card->bitsizes[i++] = 8;
    if(bitsize_flags & 0x0002) {
        /* First, select 16 bit */
        card->selected_bitsize_index = i;
        card->bitsizes[i++] = 16;
    }
    if(bitsize_flags & 0x0004)
        card->bitsizes[i++] = 20;
    if(bitsize_flags & 0x0008) {
        /* Use 24 bit where present */
        card->selected_bitsize_index = i;
        card->bitsizes[i++] = 24;
    }
    if(bitsize_flags & 0x0010)
        card->bitsizes[i++] = 32;

    if(card->nr_of_bitsizes) {
        D(bug("[NVHDMI] Selected bitsize = %lu\n", card->bitsizes[card->selected_bitsize_index]));
    }
}


static void determine_frequencies(struct NVHDMIChip *card)
{
    ULONG verb = get_parameter(card->dac_nid, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card);
    UWORD samplerate_flags = verb & PCM_SIZE_RATE_RATE_MASK;
    int i;
    ULONG freqs = 0;
    BOOL default_freq_found = FALSE;

    if(samplerate_flags == 0) {
        verb = get_parameter(card->function_group, VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE, card);
        samplerate_flags = verb & PCM_SIZE_RATE_RATE_MASK;
        D(bug("[NVHDMI] converter has no sample rate list, trying AFG node\n"));
    }

    for(i = 0; i < 12; i++) {
        if(samplerate_flags & (1 << i)) {
            freqs++;
        }
    }

    D(bug("[NVHDMI] Frequencies found = %lu\n", freqs));
    card->frequencies = (struct Freq *) AllocVec(sizeof(struct Freq) * freqs, MEMF_PUBLIC | MEMF_CLEAR);
    card->nr_of_frequencies = freqs;

    freqs = 0;
    for(i = 0; i < 12; i++) {
        if(samplerate_flags & (1 << i)) {
            set_frequency_info(&(card->frequencies[freqs]), i);

            if(card->frequencies[freqs].frequency == 48000 && !default_freq_found) {
                /* 48 kHz is HDMI's basic audio rate; make it the default */
                card->selected_freq_index = freqs;
                default_freq_found = TRUE;
            }

            freqs++;
        }
    }

    if(default_freq_found == FALSE && freqs > 0) {
        D(bug("[NVHDMI] 48000 Hz is not supported! Using %lu\n",
              card->frequencies[0].frequency));
        card->selected_freq_index = 0;
    }
}


static void set_frequency_info(struct Freq *freq, UWORD bitnr)
{
    switch(bitnr) {
    case 0:
        freq->frequency = 8000;
        freq->base44100 = 0;
        freq->mult = 0;
        freq->div = 5;
        break;

    case 1:
        freq->frequency = 11025;
        freq->base44100 = 1;
        freq->mult = 0;
        freq->div = 3;
        break;

    case 2:
        freq->frequency = 16000;
        freq->base44100 = 0;
        freq->mult = 0;
        freq->div = 2;
        break;

    case 3:
        freq->frequency = 22050;
        freq->base44100 = 1;
        freq->mult = 0;
        freq->div = 1;
        break;

    case 4:
        freq->frequency = 32000;
        freq->base44100 = 0;
        freq->mult = 0;
        freq->div = 2;
        break;

    case 5:
        freq->frequency = 44100;
        freq->base44100 = 1;
        freq->mult = 0;
        freq->div = 0;
        break;

    case 6:
        freq->frequency = 48000;
        freq->base44100 = 0;
        freq->mult = 0;
        freq->div = 0;
        break;

    case 7:
        freq->frequency = 88200;
        freq->base44100 = 1;
        freq->mult = 1;
        freq->div = 0;
        break;

    case 8:
        freq->frequency = 96000;
        freq->base44100 = 0;
        freq->mult = 1;
        freq->div = 0;
        break;

    case 9:
        freq->frequency = 176400;
        freq->base44100 = 1;
        freq->mult = 3;
        freq->div = 0;
        break;

    case 10:
        freq->frequency = 192000;
        freq->base44100 = 0;
        freq->mult = 3;
        freq->div = 0;
        break;

    default:
        D(bug("[NVHDMI] Unsupported frequency!\n"));
        break;
    }
}


UWORD get_hda_format(struct NVHDMIChip *card)
{
    BYTE bitsize = FORMAT_16BITS;
    if(card->bitsizes[card->selected_bitsize_index] == 24)
        bitsize = FORMAT_24BITS;

    return ((card->frequencies[card->selected_freq_index].base44100 > 0) ? BASE44 : 0) | // base freq: 48 or 44.1 kHz
           (card->frequencies[card->selected_freq_index].mult << 11) | // multiplier
           (card->frequencies[card->selected_freq_index].div << 8) | // divisor
           bitsize | FORMAT_STEREO;
}
