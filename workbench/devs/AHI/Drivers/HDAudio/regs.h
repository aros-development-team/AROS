#ifndef _REGS_H
#define _REGS_H

#define HD_GCAP 0x0 // word
  #define HD_GCAP_ISS_MASK 0x0F00
  #define HD_GCAP_OSS_MASK 0xF000
#define HD_VMIN 0x2
#define HD_VMAJ 0x3
#define HD_GCTL 0x8
#define HD_STATESTS 0xE

#define HD_INTCTL 0x20 // interrupt control
  #define HD_INTCTL_GLOBAL 0x80000000
  #define HD_INTCTL_CIE    0x40000000
#define HD_INTSTS 0x24 // interrupt status


// CORB
#define HD_CORB_LOW 0x40
#define HD_CORB_HIGH 0x44
#define HD_CORBWP 0x48 // write pointer
#define HD_CORBRP 0x4A
  #define HD_CORBRPRST 0x8000 // corb read pointer reset
#define HD_CORBCTL 0x4C // CORB Control
  #define HD_CORBRUN 0x2 // DMA Run
#define HD_CORBSIZE 0x4E


// RIRB
#define HD_RIRB_LOW 0x50
#define HD_RIRB_HIGH 0x54
#define HD_RIRBWP 0x58 // write pointer
  #define HD_RIRBWPRST 0x8000
#define HD_RINTCNT 0x5A // repsonse interrupt count, 2 bytes
  #define HD_RINTCNT_MASK 0xFF
#define HD_RIRBCTL 0x5C // RIRB Control
  #define HD_RIRBRUN 0x2 // DMA Run
  #define HD_RINTCTL 0x1
#define HD_RIRBSTS 0x5D // RIRB status
#define HD_RIRBSIZE 0x5E

// DMA position
#define HD_DPLBASE 0x70
  #define HD_DPLBASE_ENABLE 0x1
#define HD_DPUBASE 0x74


// 3.3.35 I/O/B stream descriptor n control

#define HD_SD_BASE_OFFSET 0x80 // 0x80: input stream descriptor 0, 0x80 + (ISS * 0x20) = output stream descriptor 0, descriptor size = 0x20
#define HD_SD_DESCRIPTOR_SIZE 0x20

#define HD_SD_OFFSET_CONTROL 0x00 // 3 bytes
  #define HD_SD_CONTROL_IOCE 0x4 // interrupt on completion enable
  #define HD_SD_CONTROL_STREAM_RUN 0x2
#define HD_SD_OFFSET_STATUS  0x03
  #define HD_SD_STATUS_MASK 0x1C
#define HD_SD_OFFSET_LINKPOS 0x04 // read only
#define HD_SD_OFFSET_CYCLIC_BUFFER_LEN 0x08 // 4 bytes
#define HD_SD_OFFSET_LAST_VALID_INDEX 0x0C // 2 bytes
#define HD_SD_OFFSET_FIFO_SIZE 0x10 // 2 bytes
#define HD_SD_OFFSET_FORMAT 0x12 // 2 bytes
#define HD_SD_OFFSET_BDL_ADDR_LOW 0x18 // 4 bytes
#define HD_SD_OFFSET_BDL_ADDR_HIGH 0x1C // 4 bytes

// verbs
#define VERB_GET_PARMS 0xF00
  #define VERB_GET_PARMS_VENDOR_DEVICE 0x0
  #define VERB_GET_PARMS_NODE_COUNT 0x04
  #define VERB_GET_PARMS_FUNCTION_GROUP_TYPE 0x5
  #define VERB_GET_PARMS_AUDIO_WIDGET_CAPS 0x9
    #define AUDIO_WIDGET_CAPS(x) ((x >> 20) & 0xF)
    #define AUDIO_WIDGET_POWER_CONTROL(x) ((x >> 10) & 0x1)
  #define VERB_GET_PARMS_SUPPORTED_PCM_SIZE_RATE 0xA
  #define VERB_GET_PARMS_PIN_CAPS 0xC
    #define PIN_CAPS_EAPD_CAPABLE (1 << 16)
    #define PIN_CAPS_INPUT_CAPABLE (1 << 5)
    #define PIN_CAPS_OUTPUT_CAPABLE (1 << 4)
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
#define VERB_SET_EAPD 0x70C

#define VERB_GET_CONNECTION_SELECT 0xF01
#define VERB_GET_CONNECTION_LIST_ENTRY 0xF02
#define VERB_GET_POWER_STATE 0xF05
#define VERB_GET_PIN_WIDGET_CONTROL 0xF07
#define VERB_GET_PIN_SENSE 0xF09
#define VERB_EXECUTE_PIN_SENSE 0x709
#define VERB_GET_CONFIG_DEFAULT 0xF1C
#define VERB_FUNCTION_RESET 0x7FF

#endif /* _REGS_H */
