/* 

The contents of this file are subject to the AROS Public License Version 1.1 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at
http://www.aros.org/license.html

Software distributed under the License is distributed on an "AS IS" basis, WITHOUT WARRANTY OF
ANY KIND, either express or implied. See the License for the specific language governing rights and
limitations under the License.

The Original Code is (C) Copyright 2004-2011 Ross Vumbaca.

The Initial Developer of the Original Code is Ross Vumbaca.

All Rights Reserved.

*/

#ifndef _REGS_H
#define _REGS_H

#define CACHELINE_SIZE 32

/* Basic registers */

#define SB128_CONTROL   0x00
#define SB128_STATUS    0x04

#define SB128_MIDI_DATA     0x08
#define SB128_MIDI_STATUS   0x09
#define SB128_MIDI_TEST     0x0A

#define SB128_MEMPAGE   0x0C

#define SB128_SRC       0x10

#define SB128_CODEC     0x14

#define SB128_LEGACY    0x18

#define SB128_SCON      0x20

#define SB128_DAC1_SCOUNT   0x24
#define SB128_DAC2_SCOUNT   0x28
#define SB128_ADC_SCOUNT    0x2C

#define SB128_PAGE_DAC      0x0C
#define SB128_PAGE_ADC      0x0D

#define SB128_DAC1_FRAME    0x30 
#define SB128_DAC1_COUNT    0x34
#define SB128_DAC2_FRAME    0x38
#define SB128_DAC2_COUNT    0x3C
#define SB128_ADC_FRAME     0x30
#define SB128_ADC_COUNT     0x34

  /* ES1370 specific, not present on other chips */

  #define ES1370_SB128_CODEC  0x10



/* DAC/ADC and misc Control */

#define CTRL_DAC1_EN    0x00000040 /* Enable DAC1 */
#define CTRL_DAC2_EN    0x00000020 /* Enable DAC2 */
#define CTRL_ADC_EN     0x00000010 /* Enable ADC */

  /* ES1370 specific, not present on other chips */

  #define CTRL_CDC_EN    0x00000002 /* Enable Codec Control */
 
    /* Magical formula for DAC2 frequency */
  #define DAC2_SRTODIV(x) (((1411200 + (x) / 2) / (x) - 2) & 0x1fff)

  #define DAC2_DIV_SHIFT 16

  #define DAC2_DIV_MASK  0x1fff0000


/* Codec Control */

#define CODEC_RESET     0x00004000

#define CODEC_WIP       0x40000000
#define CODEC_RDY       0x80000000

#define CODEC_ADD_SHIFT 16
#define CODEC_ADD_MASK  0x007F0000
#define CODEC_READ      0x00800000

  /* ES1370 specific, not present on other chips */

  #define CODEC_CSTAT            0x00000200

  #define ES1370_CODEC_ADD_SHIFT 8

/* Sample Rate Converter */

#define SRC_DISABLE     0x00400000
#define SRC_BUSY        0x00800000
#define SRC_DIS_DAC1    0x00200000
#define SRC_DIS_DAC2    0x00100000
#define SRC_DIS_ADC     0x00080000

#define SRC_DAC1        0x70
#define SRC_DAC2        0x74
#define SRC_ADC         0x78

#define SRC_VOL_ADC     0x6C
#define SRC_VOL_DAC1    0x7C
#define SRC_VOL_DAC2    0x7E


/* Offsets for some "Magic" SRC bits */

#define SRC_TRUNC       0x00
#define SRC_INT         0x01
#define SRC_VF          0x03

#define SRC_ADDR_SHIFT  25
#define SRC_WE          0x01000000


/* Interrupts */

#define SB128_IRQ_MASK    0xFFFFF8FF /* To disable all playback/record interrupts */
#define SB128_INT_PENDING 0x80000000 /* Interrupt pending */
#define SB128_INT_DAC1    0x00000004 /* DAC1 Interrupt pending */
#define SB128_INT_DAC2    0x00000002 /* DAC2 Interrupt pending */
#define SB128_INT_ADC     0x00000001 /* ADC Interrupt pending */
#define SB128_DAC1_INTEN  0x00000100 /* Enable DAC1 Interrupts */
#define SB128_DAC2_INTEN  0x00000200 /* Enable DAC2 Interrupts */
#define SB128_ADC_INTEN   0x00000400 /* Enable ADC Interrupts */


/* Data Format */

#define SB128_STEREO      0x1
#define SB128_16BIT       0x2



/* ES1370 specific - AK4531 registers */

  /* Output volumes */

#define AK4531_MASTER_VOL_L     0x00 /* -62 dB to 0 dB attentuation */ 
#define AK4531_MASTER_VOL_R     0x01 /* -62 dB to 0 dB attentuation */ 

#define AK4531_MASTER_VOL_MONO  0x0F /* -28 dB to 0 dB attentuation */ 

  /* Input gain */

#define AK4531_PCMOUT_VOL_L     0x02 /* Adjustable gain from -50 dB to +12 dB for all below */
#define AK4531_PCMOUT_VOL_R     0x03
#define AK4531_CD_VOL_L         0x06
#define AK4531_CD_VOL_R         0x07
#define AK4531_LINEIN_VOL_L     0x08
#define AK4531_LINEIN_VOL_R     0x09
#define AK4531_AUX_VOL_L        0x0A
#define AK4531_AUX_VOL_R        0x0B
#define AK4531_PHONE_VOL_L      0x0C
#define AK4531_PHONE_VOL_R      0x0D
#define AK4531_MIC_VOL          0x0E

  /* Mixer registers */

#define AK4531_OUTPUT_MUX_1     0x10
#define AK4531_OUTPUT_MUX_2     0x11

#define AK4531_INPUT_MUX_L_1    0x12
#define AK4531_INPUT_MUX_R_1    0x13

#define AK4531_INPUT_MUX_L_2    0x14
#define AK4531_INPUT_MUX_R_2    0x15

  /* Output MUX bits */

#define AK4531_OUTPUT_CD        0x06
#define AK4531_OUTPUT_LINE      0x18
#define AK4531_OUTPUT_AUX       0x30

  /* Other stuff */

#define AK4531_MUTE             0x80

#define AK4531_RECORD_SELECT    0x18 /* Input from mixer or external pins */

#define AK4531_RECORD_GAIN_MIC  0x19 /* Either 0 dB or 30 dB gain */

#define AK4531_RESET            0x16
#define AK4531_CLOCK_SEL        0x17



/* AC97 Registers */

#define AC97_RESET               0x0000


/* Play master volume registers */

#define AC97_MASTER_VOL_STEREO   0x0002 /* -94.5 to 0.0 dB attenuation */
#define AC97_AUXOUT_VOL          0x0004 /* "" + can be impl. as line level out, headphone out or 4ch out. Most likely 4ch out vol. */
#define AC97_MASTER_VOL_MONO     0x0006 /* "" */

#define AC97_MASTER_TONE         0x0008 /* bass / treble */
#define AC97_PCBEEP_VOL          0x000a


/* Analog mixer input gain registers
   5-bit gain: -32.5 dB attenuation to +12.0 dB gain
   0x0008 is 0dB gain, 0x8008 is 0dB gain with mute on */

#define AC97_PHONE_VOL           0x000c /* mono: only bits 0-4 */
#define AC97_MIC_VOL             0x000e /* mono + bit 6 is 20dB boost switch */
#define AC97_LINEIN_VOL          0x0010 /* stereo */
#define AC97_CD_VOL              0x0012
#define AC97_VIDEO_VOL           0x0014
#define AC97_AUX_VOL             0x0016
#define AC97_PCMOUT_VOL          0x0018

#define AC97_RECORD_SELECT       0x001a


/* 0dB to 22.5 dB gain on the stereo input */

#define AC97_RECORD_GAIN         0x001c
#define AC97_RECORD_GAIN_MIC     0x001e

#define AC97_GENERAL_PURPOSE     0x0020
#define AC97_3D_CONTROL          0x0022


/* Mostly a read-only register (except D5 and D4 which control optional DAC slot assignment)
   controls variable SRC, double-rate output, multi-channel output and S/PDIF output */

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
