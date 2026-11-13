#ifndef HIDD_HDA_H
#define HIDD_HDA_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: HD Audio controller hidd interface
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_INTERRUPTS_H
#include <exec/interrupts.h>
#endif

#ifndef HIDD_HIDD_H
#include <hidd/hidd.h>
#endif

#ifndef OOP_OOP_H
#include <oop/oop.h>
#endif

#ifndef UTILITY_HOOKS_H
#include <utility/hooks.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

/*
 * CLID_Hidd_HDA is the bus-agnostic HD Audio controller base class: it
 * owns the register-level protocol (reset, CORB/RIRB, verbs, streams,
 * interrupts, DMA cache maintenance) but knows nothing about how the
 * controller is attached. A driver subclasses it, implementing the
 * moHidd_HDA_HWInit/HWExit overrides that map the register block, apply
 * bus-specific quirks and install the supplied interrupt with the
 * platform, then calls moHidd_HDA_Setup once at instance creation.
 * aHidd_HDA_DeviceData carries the subclass's device handle (for a PCI
 * subclass, the PCI device object) through NewObject.
 */
#define CLID_Hidd_HDA           "hidd.audio.hda"

/* Stream directions */
#define vHidd_HDA_StreamDir_In  0
#define vHidd_HDA_StreamDir_Out 1

#define HDA_MAXSTREAMBUFS       4

/* Filled in by moHidd_HDA_SetupStream */
struct HDA_StreamInfo
{
    UBYTE si_Tag;           /* stream tag, for the converter stream/channel verb */
    UBYTE si_HWIndex;       /* descriptor index within the controller */
    UWORD si_FIFOSize;
    ULONG si_BufferCount;
    APTR  si_Buffers[HDA_MAXSTREAMBUFS];
};

#include <interface/Hidd_HDA.h>

/*
 * Codec verbs (High Definition Audio specification 7.3).
 * The controller hidd transports them; construction and interpretation
 * is the codec driver's business.
 */
#define VERB_GET_PARMS 0xF00
#define VERB_GET_PARMS_VENDOR_DEVICE 0x0
#define VERB_GET_PARMS_NODE_COUNT 0x04
#define VERB_GET_PARMS_FUNCTION_GROUP_TYPE 0x5
#define VERB_GET_PARMS_AUDIO_WIDGET_CAPS 0x9
#define AUDIO_WIDGET_CAPS(x) (((x) >> 20) & 0xF)
#define AUDIO_WIDGET_POWER_CONTROL(x) (((x) >> 10) & 0x1)
#define AUDIO_WIDGET_DIGITAL(x) (((x) >> 9) & 0x1)
#define VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE 0xA
#define PCM_SIZE_RATE_BITSIZE_MASK 0x001F0000
#define PCM_SIZE_RATE_RATE_MASK 0x0FFF
#define VERB_GET_PARMS_PIN_CAPS 0xC
#define PIN_CAPS_EAPD_CAPABLE (1 << 16)
#define PIN_CAPS_INPUT_CAPABLE (1 << 5)
#define PIN_CAPS_OUTPUT_CAPABLE (1 << 4)
#define PIN_CAPS_HDMI (1 << 7)
#define PIN_CAPS_DP (1 << 24)
#define VERB_GET_PARMS_INPUT_AMP_CAPS 0x0D
#define VERB_GET_PARMS_CONNECTION_LIST_LENGTH 0x0E
#define VERB_GET_PARMS_OUTPUT_AMP_CAPS 0x12
#define AUDIO_FUNCTION 0x01

#define VERB_SET_CONVERTER_FORMAT 0x2
#define BASE44 (1 << 14)
#define FORMAT_24BITS (0x3 << 4)
#define FORMAT_16BITS (0x1 << 4)
#define FORMAT_STEREO 0x1

#define VERB_SET_AMP_GAIN 0x3
#define OUTPUT_AMP_GAIN (1 << 15)
#define INPUT_AMP_GAIN (1 << 14)
#define AMP_GAIN_LR (3 << 12)

#define VERB_SET_CONNECTION_SELECT 0x701
#define VERB_SET_POWER_STATE 0x705
#define VERB_SET_CONVERTER_STREAM_CHANNEL 0x706
#define VERB_SET_PIN_WIDGET_CONTROL 0x707
#define VERB_EXECUTE_PIN_SENSE 0x709
#define VERB_SET_EAPD 0x70C
#define VERB_SET_DIGITAL_CONVERTER_1 0x70D
#define DIGITAL_CONVERTER_DIGEN 0x01
#define VERB_SET_DIGITAL_CONVERTER_2 0x70E
#define VERB_SET_VOLUME_KNOB_CONTROL 0x70F
#define VERB_SET_GPIO_DATA 0x715
#define VERB_SET_GPIO_ENABLE 0x716
#define VERB_SET_GPIO_DIR 0x717
#define VERB_SET_CONVERTER_CHANNEL_COUNT 0x72D
#define VERB_SET_HDMI_DIP_INDEX 0x730
#define VERB_SET_HDMI_DIP_DATA 0x731
#define VERB_SET_HDMI_DIP_XMIT 0x732
#define DIP_XMIT_DISABLE 0x00
#define DIP_XMIT_ONCE 0x80
#define DIP_XMIT_BEST 0xC0
#define VERB_SET_ASP_CHANNEL_SLOT 0x734

#define VERB_GET_CONNECTION_SELECT 0xF01
#define VERB_GET_CONNECTION_LIST_ENTRY 0xF02
#define VERB_GET_POWER_STATE 0xF05
#define VERB_GET_CONVERTER_STREAM_CHANNEL 0xF06
#define VERB_GET_PIN_WIDGET_CONTROL 0xF07
#define VERB_GET_PIN_SENSE 0xF09
#define PIN_SENSE_PRESENCE (1U << 31)
#define PIN_SENSE_ELD_VALID (1U << 30)
#define VERB_GET_DIGITAL_CONVERTER 0xF0D
#define VERB_GET_GPIO_DATA 0xF15
#define VERB_GET_GPIO_ENABLE 0xF16
#define VERB_GET_GPIO_DIR 0xF17
#define VERB_GET_CONFIG_DEFAULT 0xF1C
#define VERB_GET_HDMI_ELD_DATA 0xF2F
#define VERB_FUNCTION_RESET 0x7FF

/* Construct CORB commands (12-bit and 4-bit verb forms) */
#define HDA_COMMAND_12(codec, nid, verb, payload) \
    (((ULONG)(codec) << 28) | ((ULONG)(nid) << 20) | ((ULONG)(verb) << 8) | (UBYTE)(payload))
#define HDA_COMMAND_4(codec, nid, verb, payload) \
    (((ULONG)(codec) << 28) | ((ULONG)(nid) << 20) | ((ULONG)(verb) << 16) | (UWORD)(payload))

#endif /* HIDD_HDA_H */
