/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _REGS_H
#define _REGS_H


#define VIA_CODEC_CMD		0x80
#define  VIA_CODEC_CMD_READ		(1L << 23)
#define  VIA_CODEC_CMD_VALID		(1L << 25)
#define  VIA_CODEC_CMD_BUSY		(1L << 24)

#define PAGE_SIZE      65536


#define AC97_RESET               0x0000

// Play master volume registers
#define AC97_MASTER_VOL_STEREO   0x0002 // -94.5 to 0.0 dB attenuation
#define AC97_AUXOUT_VOL          0x0004 // "" + can be impl. as line level out, headphone out or 4ch out. Most likely 4ch out vol.
#define AC97_MASTER_VOL_MONO     0x0006 // ""

#define AC97_MASTER_TONE         0x0008 // bass / treble
#define AC97_PCBEEP_VOL          0x000a

// Analog mixer input gain registers
// 5-bit gain: -32.5 dB attenuation to +12.0 dB gain
// 0x0008 is 0dB gain, 0x8008 is 0dB gain with mute on
#define AC97_PHONE_VOL           0x000c // mono: only bits 0-4
#define AC97_MIC_VOL             0x000e // mono + bit 6 is 20dB boost switch
#define AC97_LINEIN_VOL          0x0010 // stereo
#define AC97_CD_VOL              0x0012
#define AC97_VIDEO_VOL           0x0014
#define AC97_AUX_VOL             0x0016
#define AC97_PCMOUT_VOL          0x0018

#define AC97_RECORD_SELECT       0x001a
// 0dB to 22.5 dB gain on the stereo input
#define AC97_RECORD_GAIN         0x001c
#define AC97_RECORD_GAIN_MIC     0x001e

#define AC97_GENERAL_PURPOSE     0x0020
#define AC97_3D_CONTROL          0x0022

// mostly a read-only register (except D5 and D4 which control optional DAC slot assignment)
// controls variable SRC, double-rate output, multi-channel output and S/PDIF output
#define AC97_EXTENDED_ID         0x0028
#define AC97_EXTENDED_CTRL       0x002a

#define AC97_SPDIF_CTRL          0x003a

#define AC97_SURROUND_MASTER     0x0038

#define AC97_VENDOR_ID0          0x007C
#define AC97_VENDOR_ID1          0x007E

#define AC97_MUTE                0x8000

#define AC97_RECMUX_MIC          0x0000
#define AC97_RECMUX_CD           0x0101
#define AC97_RECMUX_VIDEO        0x0202
#define AC97_RECMUX_AUX          0x0303
#define AC97_RECMUX_LINE         0x0404
#define AC97_RECMUX_STEREO_MIX   0x0505
#define AC97_RECMUX_MONO_MIX     0x0606
#define AC97_RECMUX_PHONE        0x0707


#endif /* _REGS_H */
