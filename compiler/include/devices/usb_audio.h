#ifndef DEVICES_USB_AUDIO_H
#define DEVICES_USB_AUDIO_H
/*
**	$VER: usb_audio.h 2.0 (15.12.07)
**
**	usb definitions include file
**
**	(C) Copyright 2002-2007 Chris Hodges
**	    All Rights Reserved
*/

#include <exec/types.h>

#if defined(__GNUC__)
# pragma pack(1)
#endif

/* Usb Audio Requests */
#define UAUDR_SET_CUR             0x01
#define UAUDR_GET_CUR             0x81
#define UAUDR_SET_MIN             0x02
#define UAUDR_GET_MIN             0x82
#define UAUDR_SET_MAX             0x03
#define UAUDR_GET_MAX             0x83
#define UAUDR_SET_RES             0x04
#define UAUDR_GET_RES             0x84
#define UAUDR_SET_MEM             0x05
#define UAUDR_GET_MEM             0x85
#define UAUDR_GET_STAT            0xff

/* Audio Ctrl class specific interface descriptor subtypes */
#define UDST_AUDIO_CTRL_HEADER          0x01
#define UDST_AUDIO_CTRL_INPUT_TERMINAL  0x02
#define UDST_AUDIO_CTRL_OUTPUT_TERMINAL 0x03
#define UDST_AUDIO_CTRL_MIXER_UNIT      0x04
#define UDST_AUDIO_CTRL_SELECTOR_UNIT   0x05
#define UDST_AUDIO_CTRL_FEATURE_UNIT    0x06
#define UDST_AUDIO_CTRL_PROCESSING_UNIT 0x07
#define UDST_AUDIO_CTRL_EXTENSION_UNIT  0x08

/* Audio Streaming class specific interface descriptor subtypes */
#define UDST_AUDIO_STREAM_GENERAL       0x01
#define UDST_AUDIO_STREAM_FMT_TYPE      0x02
#define UDST_AUDIO_STREAM_FMT_SPECIFIC  0x03

/* Audio MIDI class specific interface descriptor subtypes */
#define UDST_AUDIO_MIDI_HEADER   0x01
#define UDST_AUDIO_MIDI_IN_JACK  0x02
#define UDST_AUDIO_MIDI_OUT_JACK 0x03
#define UDST_AUDIO_MIDI_ELEMENT  0x04

/* Audioclass specific endpoint descriptors subtypes */
#define UDST_AUDIO_EP_GENERAL    0x01

/* Audio MIDI Jack-types */
#define USBMIDI_JACK_TYPE_UNDEFINED 0x00
#define USBMIDI_JACK_EMBEDDED       0x01
#define USBMIDI_JACK_EXTERNAL       0x02

/* Audio classes */
#define AUDIO_NO_SUBCLASS     0x00
#define AUDIO_CTRL_SUBCLASS   0x01
#define AUDIO_STREAM_SUBCLASS 0x02
#define AUDIO_MIDI_SUBCLASS   0x03

/* USB Audio specific stuff */

/* USB Audio USB Terminal types */
#define UAUTT_UNDEFINED        0x0100 /* USB Undefined */
#define UAUTT_STREAMING        0x0101 /* USB streaming */
#define UAUTT_VENDOR           0x01ff /* USB vendor specific */

/* USB Audio Input Terminal types */
#define UAITT_UNDEFINED        0x0200 /* Input Undefined */
#define UAITT_MIC              0x0201 /* Microphone */
#define UAITT_DESKTOP_MIC      0x0202 /* Desktop microphone */
#define UAITT_PERSONAL_MIC     0x0203 /* Personal microphone */
#define UAITT_OMNI_DIR_MIC     0x0204 /* Omni-directional microphone */
#define UAITT_MIC_ARRAY        0x0205 /* Microphone array */
#define UAITT_PROC_MIC_ARRAY   0x0206 /* Processing microphone array */

/* USB Audio Output Terminal types */
#define UAOTT_UNDEFINED        0x0300 /* Output Undefined */
#define UAOTT_SPEAKER          0x0301 /* Speaker */
#define UAOTT_HEADPHONES       0x0302 /* Headphones */
#define UAOTT_DISPLAY          0x0303 /* Head Mounted Display Audio */
#define UAOTT_DESKTOP_SPEAKER  0x0304 /* Desktop speaker */
#define UAOTT_ROOM_SPEAKER     0x0305 /* Room speaker */
#define UAOTT_COMM_SPEAKER     0x0306 /* Communication speaker */
#define UAOTT_LOFI_SPEAKER     0x0307 /* Low frequency effects speaker */

/* USB Audio Bi-directional Terminal types */
#define UABTT_UNDEFINED        0x0400 /* Bi-directional Undefined */
#define UABTT_HANDSET          0x0401 /* Handset */
#define UABTT_HEADSET          0x0402 /* Headset */
#define UABTT_SPEAKERPHONE_NER 0x0403 /* Speakerphone, no echo reduction */
#define UABTT_SPEAKERPHONE_ES  0x0404 /* Echo-suppressing speakerphone */
#define UABTT_SPEAKERPHONE_EC  0x0405 /* Echo-canceling speakerphone */

/* USB Audio Telephony Terminal Types */
#define UATTT_UNDEFINED        0x0500 /* Telephony Undefined */
#define UATTT_PHONE_LINE       0x0501 /* Phone line */
#define UATTT_TELEPHONE        0x0502 /* Telephone */
#define UATTT_DOWN_LINE_PHONE  0x0503 /* Down Line Phone */

/* USB Audio External Terminal Types */
#define UAETT_UNDEFINED        0x0600 /* External Undefined */
#define UAETT_ANALOG           0x0601 /* Analog connector */
#define UAETT_DIGITAL          0x0602 /* Digital audio interface */
#define UAETT_LINE             0x0603 /* Line connector */
#define UAETT_LEGACY           0x0604 /* Legacy audio connector */
#define UAETT_SPDIF            0x0605 /* S/PDIF interface */
#define UAETT_1394DA           0x0606 /* 1394 DA stream */
#define UAETT_1394DV           0x0607 /* 1394 DV stream soundtrack */

/* USB Embedded Function Terminal Types */
#define UAFTT_UNDEFINED        0x0700 /* Embedded Undefined */
#define UAFTT_CALIB_NOISE      0x0701 /* Level Calibration Noise Source */
#define UAFTT_EQ_NOISE         0x0702 /* Equalization Noise */
#define UAFTT_CD               0x0703 /* CD player */
#define UAFTT_DAT              0x0704 /* DAT */
#define UAFTT_DCC              0x0705 /* DCC */
#define UAFTT_MINIDISK         0x0706 /* MiniDisk */
#define UAFTT_TAPE             0x0707 /* Analog Tape */
#define UAFTT_PHONO            0x0708 /* Phonograph */
#define UAFTT_VCR              0x0709 /* VCR Audio */
#define UAFTT_VIDEODISC        0x070a /* Video Disc Audio */
#define UAFTT_DVD              0x070b /* DVD Audio */
#define UAFTT_TV_TUNER         0x070c /* TV Tuner Audio */
#define UAFTT_SAT_RX           0x070d /* Satellite Receiver Audio */
#define UAFTT_CABLE_TUNER      0x070e /* Cable Tuner Audio */
#define UAFTT_DSS              0x070f /* DSS Audio */
#define UAFTT_RADIO_RX         0x0710 /* Radio Receiver */
#define UAFTT_RADIO_TX         0x0711 /* Radio Transmitter */
#define UAFTT_MULTITRACK       0x0712 /* Multi-track Recorder */
#define UAFTT_SYNTHESIZER      0x0713 /* Synthesizer */

/* USB Audio Audio Data Formats */
#define UAADF_TYPE_I_UNDEFINED      0x0000
#define UAADF_PCM                   0x0001
#define UAADF_PCM8                  0x0002
#define UAADF_IEEE_FLOAT            0x0003
#define UAADF_ALAW                  0x0004
#define UAADF_MULAW                 0x0005

#define UAADF_TYPE_II_UNDEFINED     0x1000
#define UAADF_MPEG                  0x1001
#define UAADF_AC3                   0x1002

#define UAADF_TYPE_III_UNDEFINED    0x2000
#define UAADF_IEC1937_AC3           0x2001
#define UAADF_IEC1937_MPEG1_L1      0x2002
#define UAADF_IEC1937_MPEG1_L2_3    0x2003
#define UAADF_IEC1937_MPEG2_NOEXT   0x2003
#define UAADF_IEC1937_MPEG2_EXT     0x2004
#define UAADF_IEC1937_MPEG2_L1_LS   0x2005
#define UAADF_IEC1937_MPEG2_L2_3_LS 0x2006

/* USB Audio Format Type Codes */
#define UAFTC_UNDEFINED             0x00
#define UAFTC_TYPE_I                0x01
#define UAFTC_TYPE_II               0x02
#define UAFTC_TYPE_III              0x03

/* Endpoint control selectors */
#define UAECS_SAMPLE_FREQ           0x01
#define UAECS_PITCH                 0x02

/* Feature Unit Bits */
#define UAFUB_MUTE                   0
#define UAFUB_VOLUME                 1
#define UAFUB_BASS                   2
#define UAFUB_MID                    3
#define UAFUB_TREBLE                 4
#define UAFUB_EQUALIZER              5
#define UAFUB_AUTOMATIC_GAIN         6
#define UAFUB_DELAY                  7
#define UAFUB_BASS_BOOST             8
#define UAFUB_LOUDNESS               9

#define UAFUF_MUTE                  (1<<UAFUB_MUTE)
#define UAFUF_VOLUME                (1<<UAFUB_VOLUME)
#define UAFUF_BASS                  (1<<UAFUB_BASS)
#define UAFUF_MID                   (1<<UAFUB_MID)
#define UAFUF_TREBLE                (1<<UAFUB_TREBLE)
#define UAFUF_EQUALIZER             (1<<UAFUB_EQUALIZER)
#define UAFUF_AUTOMATIC_GAIN        (1<<UAFUB_AUTOMATIC_GAIN)
#define UAFUF_DELAY                 (1<<UAFUB_DELAY)
#define UAFUF_BASS_BOOST            (1<<UAFUB_BASS_BOOST)
#define UAFUF_LOUDNESS              (1<<UAFUB_LOUDNESS)

/* Feature Unit Control Selectors */
#define UAFUCS_MUTE                 0x01
#define UAFUCS_VOLUME               0x02
#define UAFUCS_BASS                 0x03
#define UAFUCS_MID                  0x04
#define UAFUCS_TREBLE               0x05
#define UAFUCS_EQUALIZER            0x06
#define UAFUCS_AUTOMATIC_GAIN       0x07
#define UAFUCS_DELAY                0x08
#define UAFUCS_BASS_BOOST           0x09
#define UAFUCS_LOUDNESS             0x0a

/* USB Audio Spec 1.0 stuff */
struct UsbAudioHeaderDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bcdADC0;             /* Low byte of spec version (0x00) */
    UBYTE bcdADC1;             /* High byte of spec version (0x01) */
    UBYTE wTotalLength0;       /* Total length of all descriptors (low byte) */
    UBYTE wTotalLength1;       /* Total length of all descriptors (high byte) */
    UBYTE bInCollection;       /* The number of Audio/MidiStreaming */
    UBYTE baInterfaceNr[1];    /* Interface number of the first AudioStreaming or MIDIStreaming interface */
};

struct UsbAudioInputTermDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x02) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE bNrChannels;         /* Number of logical output channels in the output terminal */
    UWORD wChannelConfig;      /* Spacial location */
    UBYTE iChannelNames;       /* String descriptor */
    UBYTE iTerminal;           /* String descriptor */
};

struct UsbAudioOutputTermDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x03) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE bSourceID;           /* ID of the Unit or Terminal to which this Terminal is connected. */
    UBYTE iTerminal;           /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbAudioMixerUnitDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x04) */
    UBYTE bUnitID;             /* unique ID */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Mixer Unit is connected. */
    UBYTE bNrChannels;         /* Number of logical output channels in the Mixer's output audio channel cluster. */
    UWORD wChannelConfig;      /* Spatial location */
    UBYTE iChannelNames;       /* String descriptor */
    UBYTE bmControls[0];       /* Controls bitmap */
    UBYTE iMixer;              /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbAudioSelectorUnitDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x05) */
    UBYTE bUnitID;             /* unique ID */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Selector Unit is connected. */
    UBYTE iSelector;           /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbAudioFeatureUnitDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x06) */
    UBYTE bUnitID;             /* unique ID */
    UBYTE bSourceID;           /* ID of the Unit or Terminal to which this Terminal is connected. */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmaControls[0];      /* Controls matrix */
    UBYTE iFeature;            /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbAudioProcessingUnitDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x07) */
    UBYTE bUnitID;             /* unique ID */
    UWORD wProcessType;        /* type of processing */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Processing Unit is connected. */
    UBYTE bNrChannels;         /* Number of logical output channels in the Mixer's output audio channel cluster. */
    UWORD wChannelConfig;      /* Spatial location */
    UBYTE iChannelNames;       /* String descriptor */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
    UBYTE iProcessing;         /* String descriptor */
};

/* this data structure is of no use, as it's highly variable in size */
struct UsbAudioExtensionUnitDesc10
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x08) */
    UBYTE bUnitID;             /* unique ID */
    UWORD wExtensionCode;      /* vendor specific */
    UBYTE bNrInPins;           /* Number of Input Pins */
    UBYTE baSourceID[0];       /* ID of the Unit or Terminal to which the nth Input Pin of this Mixer Unit is connected. */
    UBYTE bNrChannels;         /* Number of logical output channels in the Mixer's output audio channel cluster. */
    UWORD wChannelConfig;      /* Spatial location */
    UBYTE iChannelNames;       /* String descriptor */
    UBYTE bControlSize;        /* Size in bytes of an element of the control array */
    UBYTE bmControls[0];       /* Controls bitmap */
    UBYTE iExtension;          /* String descriptor */
};
 
/* USB Audio Spec 2.0 stuff */
struct UsbAudioHeaderDesc20
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bcdADC0;             /* Low byte of spec version (0x00)*/
    UBYTE bcdADC1;             /* High byte of spec version (0x02) */
    UBYTE bCategory;           /* Primary use of the audio function */
    UWORD wTotalLength;        /* Total length of all descriptors */
    UBYTE bmControls;          /* Latency control (%00=n/a,%01=read only, %11=r/w) */
};

struct UsbAudioInputTermDesc20
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x02) */
    UBYTE bTerminalID;         /* unique ID */
    UWORD wTerminalType;       /* Terminal Type */
    UBYTE bAssocTerminal;      /* Associated Output Terminal ID */
    UBYTE bCSourceID;          /* ID of the Clock Entity */
    UBYTE bNrChannels;         /* Number of logical output channels in the output terminal */
    UBYTE bmChannelConfig[4];  /* Bitmap of spacial location */
    UBYTE iChannelNames;       /* String descriptor */
    UBYTE bmControls;          /* Various controls */
    UBYTE iTerminal;           /* String descriptor */
};

/* generic audio stuff */
struct UsbAudioGeneralIFDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bTerminalLink;       /* Terminal linked to this interface */
    UBYTE bDelay;              /* Delay in frames */
    UBYTE wFormatTag0;         /* Data Format (low byte) */
    UBYTE wFormatTag1;         /* Data Format (high byte) */
};

struct UsbAudioType1FormatDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x24) */
    UBYTE bDescriptorSubType;  /* Subtype (0x02) */
    UBYTE bFormatType;         /* Format Type I (0x01) */
    UBYTE bNrChannels;         /* Number of physical channels */
    UBYTE bSubframeSize;       /* Number of bytes occubied by one audio subframe (1-4) */
    UBYTE bBitResolution;      /* Number of bits used in a subframe */
    UBYTE bSamFreqType;        /* 0 = continuous freq, otherwise number of discrete freqs */
    UBYTE tSamFreq0[3];        /* first sampling freq (or lower range) */
};

struct UsbAudioGeneralEPDesc
{
    UBYTE bLength;             /* Size of this descriptor */
    UBYTE bDescriptorType;     /* Descriptor Type (0x25) */
    UBYTE bDescriptorSubType;  /* Subtype (0x01) */
    UBYTE bmAttributes;        /* bit 0: Sampling freq, bit 1: Pitch, bit 7 = MaxPktsOnly */
    UBYTE bLockDelayUnits;     /* 0=undef, 1=ms, 2=PCM samples */
    UBYTE wLockDelay0;         /* Delay until locked (low byte) */
    UBYTE wLockDelay1;         /* Delay until locked (high byte) */
};


#if defined(__GNUC__)
# pragma pack()
#endif

#endif /* DEVICES_USB_AUDIO_H */
