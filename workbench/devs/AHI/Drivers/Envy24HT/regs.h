/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef _REGS_H
#define _REGS_H


#define SUBVENDOR_AUREON_SKY    0x3b154711
#define SUBVENDOR_AUREON_SPACE  0x3b154511
#define SUBVENDOR_PHASE28	    0x3b154911
#define SUBVENDOR_PHASE22	    0x3b155011

#define SUBVENDOR_MAUDIO_REVOLUTION51   0x12143136
#define SUBVENDOR_MAUDIO_REVOLUTION71   0x12143036

#define SUBVENDOR_JULIA         0x31305345




#define CCS_CTRL                0x0000
    #define CCS_RESET_ALL           0x80

#define CCS_INTR_MASK           0x0001
    #define CCS_ENABLE_PLAYREC      0x10

#define CCS_INTR_STATUS         0x0002
    #define CCS_INTR_PLAYREC        0x10 // Macro interrupt for any PDMAx and RDMAx.
    #define CCS_INTR_MIDI_IN        0x80
    #define CCS_INTR_MIDI_OUT       0x20
    
                                         // To clear individual bits: write a 1 to a bit in MT00 reg
#define CCS_SYSTEM_CONFIG       0x0004
    #define CCS_CONFIG_ADC_MASK     0x03 // 00: 1 stereo adc, 01: 2, 10: 1 + 1 S/PDIF, 11: none
    #define CCS_CONFIG_ADC_SHIFT    2
    #define CCS_CONFIG_DAC_MASK     0x03 // 00: 1 stereo adc, 01: 2, 10: 3, 11: 4
    #define CCS_CONFIG_DAC_SHIFT    0

#define CCS_ACLINK_CONFIG       0x0005
    #define CCS_ACLINK_I2S          0x80 // 0: AC97, 1:I2S
    #define CCS_ACLINK_MODE         0x02 // 0: split mode, 1: packed mode
    

#define CCS_I2S_FEATURES        0x0006   // when CCS_ACLINK_CONFIG:CCS_ACLINK_TYPE == 1
    #define CCS_I2S_VOLMUTE         0x80 // 1 when true
    #define CCS_I2S_96KHZ           0X40 // 1 when supported
    #define CCS_I2S_24BIT           0x30 // bit(5:4) 00:16, 01:18, 10:20, 11:24
    #define CCS_I2S_192KHZ          0X08 // 1 when supported


#define CCS_SPDIF_CONFIG        0x0007
    #define CCS_SPDIF_INTEGRATED    0x80 // 1 enable integrated SPDIF transmitter. Only valid when bit 6 is '1'
    #define CCS_SPDIF_INTERNAL_OUT  0X40 // 1 when implemented (read-only)
    #define CCS_SPDIF_ID_MASK       0xF  // SPDIF chip ID's
    #define CCS_SPDIF_ID_SHIFT      2
    #define CCS_SPDIF_IN_PRESENT    0x02 // 1 when present
    #define CCS_SPDIF_EXTERNAL_OUT  0x01 // 1 when present (default)

#define CCS_UART_TX_STATUS      0x000A   // number of bytes in transmit queue
    #define UART_TX_QUEUE_MASK      0x1F

#define CCS_UART_RX_STATUS      0x000B   // number of bytes in receive queue
    #define UART_RX_QUEUE_MASK      0x1F

#define CCS_UART_DATA           0x000C
#define CCS_UART_COMMAND        0x000D   // MIDI command/status
#define CCS_UART_SETTINGS       0x000E



#define CCS_I2C_DEV_ADDRESS     0x0010   // check CCS13 before accessing
    #define CCS_ADDRESS_MASK        0xFE // I2C device address (bit 7:1)
    #define CCS_ADDRESS_SHIFT       1
    #define CCS_ADDRESS_WRITE       0x01 // 0: read, 1:write


#define CCS_I2C_ADDR            0x0011   // byte address to read/write
#define CCS_I2C_DATA            0x0012
#define CCS_I2C_STATUS          0X0013
    #define CCS_I2C_EPROM           0x80 // 1: E2PROM connected
    #define CCS_I2C_BUSY            0x01 // 0: idle, 1: busy

#define CCS_GPIO_DATA           0x0014 // word: GPIO 0 - 15
#define CCS_GPIO_MASK           0x0016 // 0 in mask means CCS14 register bit can be written, GPIO 0 - 15
#define CCS_GPIO_DIR            0x0018 // direction control: 3 bytes! GPIO 0 - 22, 1 = output

#define CCS_POWER_DOWN          0x001C
#define CCS_GPIO_DATA2          0x001E // byte: GPIO22 to GPIO16 (MSB!)
#define CCS_GPIO_MASK2          0x001F // mask for GPIO22 to GPIO16

// ----------MT regs-------------------
#define MT_INTR_STATUS          0x0000   // masks:
    #define MT_PDMA4                0x80 // SPDIF out pair playback
    #define MT_PDMA3                0x40 // pair playback
    #define MT_PDMA2                0x20
    #define MT_PDMA1                0x10
    #define MT_DMA_FIFO             0x08 // see MT1A
    #define MT_RDMA1                0x04 // SPDIF in
    #define MT_RDMA0                0x02 // ADC
    #define MT_PDMA0                0x01 // multi-channel interleaved/PDMA0 pair playback

#define MT_SAMPLERATE           0x0001   // in slave mode (SPDIF is master): 256X master clock alone selects rate
    #define MT_SPDIF_MASTER         0x10 // when SPDIF is master, set primary codec to slave mode!
    #define MT_RATE_MASK            0x0F // ignored if MT_SPDIF_MASTER = 

#define MT_I2S_FORMAT           0x0002
    #define MT_CLOCK_128x           0x8  // 0: 256x (default), 1: 128x

#define MT_INTR_MASK            0x0003   // default: all are off = '1'
    #define MT_PDMA4_MASK           0x80 // SPDIF out pair playback
    #define MT_PDMA3_MASK           0x40 // valid when MT19 > 0
    #define MT_PDMA2_MASK           0x20 // valid when MT19 > 1
    #define MT_PDMA1_MASK           0x10 // valid when MT19 = 3
    #define MT_DMA_FIFO_MASK        0x08 // MT1A reports offending channel
    #define MT_RDMA1_MASK           0x04 // SPDIF in
    #define MT_RDMA0_MASK           0x02 // ADC
    #define MT_PDMA0_MASK           0x01 // multi-channel interleaved/PDMA0 pair playback


#define MT_AC97_REG             0x0004 // AC'97 register index
#define MT_AC97_CMD_STATUS      0x0005 // valid when CCS_ACLINK_TYPE == 0
    #define MT_AC97_RESET           0x80 // cold reset (alone will put it into master mode)
    #define MT_AC97_WARM_RESET      0X40 // when used together with MT_AC97_RESET, will set external VIA AC'97 to slave mode
    #define MT_AC97_WRITE           0X20 // write 1 for write mode, reading a 1 is WIP
    #define MT_AC97_READ            0x10 // write 1 for read mode, reading a 1 is RIP
    #define MT_AC97_READY           0x08 // codec ready status
    #define MT_AC97_ID_MASK         0x03 // bit 0:1 is ID mode when is split mode.
#define MT_AC97_DATA            0x0006
    
#define MT_DMAI_PB_ADDRESS      0x0010 // long: start address of interleaved playback buffer (long boundary)
#define MT_DMAI_PB_LENGTH       0X0014 // long, but 3 bytes (0x14-0x16), DMA size - 1, read: counter

#define MT_DMA_CONTROL          0x0018 // start/stop (use read-modify-write)
    #define MT_PDMA4_START           0x80 // SPDIF out / PDMA4 pair playback
    #define MT_PDMA3_START           0x40 // valid when MT19 > 0
    #define MT_PDMA2_START           0x20 // valid when MT19 > 1
    #define MT_PDMA1_START           0x10 // valid when MT19 = 3
    #define MT_RDMA1_START           0x04 // SPDIF in
    #define MT_RDMA0_START           0x02 // ADC
    #define MT_PDMA0_START           0x01 // multi-channel interleaved/PDMA0 pair playback

#define MT_DMAI_BURSTSIZE       0x0019 // bits 0 and 1 only:
                                       // 00: default 8 ch. on PDMAi
                                       // 01: 6 ch on PDMAi, PDMA3 is available independently
                                       // 10: 4 ch on PDMAi, PDMA3 and PDMA2 are available independently
                                       // 11: 2 ch on PDMA0: 4 stereo pairs

#define MT_DMA_UNDERRUN         0x001A // FIFO overrun/underrun register
    #define MT_PDMA4_UNDERRUN       0x80 // SPDIF out / PDMA4 pair playback
    #define MT_PDMA3_UNDERRUN       0x40 // valid when MT19 > 0
    #define MT_PDMA2_UNDERRUN       0x20 // valid when MT19 > 1
    #define MT_PDMA1_UNDERRUN       0x10 // valid when MT19 = 3
    #define MT_RDMA1_OVERRUN        0x04 // SPDIF in
    #define MT_RDMA0_OVERRUN        0x02 // ADC
    #define MT_PDMA0_UNDERRUN       0x01 // multi-channel interleaved/PDMA0 pair playback

#define MT_DMA_PAUSE            0x001B


#define MT_DMAI_INTLEN          0x001C // interrupt after this size - 1 (word)

// record pair registers
#define MT_RDMA0_ADDRESS        0x0020
#define MT_RDMA0_LENGTH         0x0024
#define MT_RDMA0_INTLEN         0x0026

#define MT_RDMA1_ADDRESS        0x0030
#define MT_RDMA1_LENGTH         0x0034
#define MT_RDMA1_INTLEN         0x0036


// stereo pair registers
#define MT_PDMA4_ADDRESS        0x0040
#define MT_PDMA4_LENGTH         0x0044
#define MT_PDMA4_INTLEN         0x0046

#define MT_PDMA3_ADDRESS        0x0050
#define MT_PDMA3_LENGTH         0x0054
#define MT_PDMA3_INTLEN         0x0056

#define MT_PDMA2_ADDRESS        0x0060
#define MT_PDMA2_LENGTH         0x0064
#define MT_PDMA2_INTLEN         0x0066

#define MT_PDMA1_ADDRESS        0x0070 // stereo pair 1
#define MT_PDMA1_LENGTH         0x0074
#define MT_PDMA1_INTLEN         0x0076

#define MT_SPDIF_TRANSMIT       0x003C



// ----------AC97 regs-----------------
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

// -----------------------------
/* GPIO bits */
#define AUREON_CS8415_CS	(1 << 22)
#define AUREON_CS8415_CDOUT	(1 << 21) // output data from the control port to GPIO 21
#define AUREON_WM_RESET		(1 << 20)
#define AUREON_WM_CLK		(1 << 19)
#define AUREON_WM_DATA		(1 << 18)
#define AUREON_WM_RW		(1 << 17)
#define AUREON_AC97_RESET	(1 << 16)
#define AUREON_DIGITAL_SEL1	(1 << 15)
#define AUREON_HP_SEL		(1 << 14)
#define AUREON_WM_CS		(1 << 12)
#define AUREON_AC97_COMMIT	(1 << 11)
#define AUREON_AC97_ADDR	(1 << 10)
#define AUREON_AC97_DATA_LOW	(1 << 9)
#define AUREON_AC97_DATA_HIGH	(1 << 8)
#define AUREON_AC97_DATA_MASK	0xFF


#define PHASE28_FREQ0   	(1 << 22) // input
#define PHASE28_FREQ1   	(1 << 21)
#define PHASE28_WM_RESET	(1 << 20)
#define PHASE28_SPI_CLK		(1 << 19)
#define PHASE28_SPI_MOSI	(1 << 18)
#define PHASE28_WM_RW		(1 << 17)
#define PHASE28_FREQ2   	(1 << 16)
#define PHASE28_DIGITAL_SEL1	(1 << 15)
#define PHASE28_HP_SEL		(1 << 14)
#define PHASE28_WM_CS		(1 << 12)
#define PHASE28_AC97_COMMIT	(1 << 11)
#define PHASE28_AC97_ADDR	(1 << 10)
#define PHASE28_AC97_DATA_LOW	(1 << 9)
#define PHASE28_AC97_DATA_HIGH	(1 << 8)
#define PHASE28_AC97_DATA_MASK	0xFF


#define REVO_CCLK	    0x02 // control data input pin on AKM (pin 7 on 4381)
#define REVO_CDIN	    0x04	/* not used */
#define REVO_CDOUT	    0x08
#define REVO_CS0		0x10	/* not used */
#define REVO_CS1		0x20	/* front AKM4381 chipselect */
#define REVO_CS2		0x40	/* surround AKM4355 chipselect */
#define REVO_MUTE	(1<<22)	/* 0 = all mute, 1 = normal operation */
#define VT1724_REVO_CS3		0x80	/* AK4114 for AP192 */


// ESI JULI@
#define AK4114_ADDR		0x20		/* S/PDIF receiver */
#define AK4358_ADDR		0x22		/* DAC */

#define GPIO_FREQ_MASK		(3<<0)
#define GPIO_FREQ_32KHZ		(0<<0)
#define GPIO_FREQ_44KHZ		(1<<0)
#define GPIO_FREQ_48KHZ		(2<<0)
#define GPIO_MULTI_MASK		(3<<2)
#define GPIO_MULTI_4X		(0<<2)
#define GPIO_MULTI_2X		(1<<2)
#define GPIO_MULTI_1X		(2<<2)		/* also external */
#define GPIO_MULTI_HALF		(3<<2)
#define GPIO_INTERNAL_CLOCK	(1<<4)
#define GPIO_ANALOG_PRESENT	(1<<5)		/* RO only: 0 = present */
#define GPIO_RXMCLK_SEL		(1<<7)		/* must be 0 */
#define GPIO_AK5385A_CKS0	(1<<8)  // master clock select
#define GPIO_AK5385A_DFS0	(1<<9)		/* swapped with DFS1 according doc? */
#define GPIO_AK5385A_DFS1	(1<<10)   // dfs0 + dfs1 = sampling rate select
#define GPIO_DIGOUT_MONITOR	(1<<11)		/* 1 = active */
#define GPIO_DIGIN_MONITOR	(1<<12)		/* 1 = active */
#define GPIO_ANAIN_MONITOR	(1<<13)		/* 1 = active */
#define GPIO_AK5385A_MCLK	(1<<14)		/* must be 0 */
#define GPIO_MUTE_CONTROL	(1<<15)		/* 0 = off, 1 = on */


#endif /* _REGS_H */
