/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef _REGS_H
#define _REGS_H

#define CCS_CTRL                0x0000
    #define CCS_RESET_ALL           0x80
    #define CCS_NATIVE_MODE         0x01

#define CCS_INTR_MASK           0x0001
    #define CCS_ENABLE_MIDI1        0x80
    #define CCS_ENABLE_TIMER        0x40
    #define CCS_ENABLE_MIDI2        0x20
    #define CCS_ENABLE_PRO_MACRO    0x10
    #define CCS_ENABLE_FM           0x08
    #define CCS_ENABLE_PLAY_DS      0x04
    #define CCS_ENABLE_CONS_REC     0x02
    #define CCS_ENABLE_CONS_PLAY    0x01

#define CCS_INTR_STATUS         0x0002
    #define CCS_INTR_MIDI1          0x80
    #define CCS_INTR_TIMER          0x40
    #define CCS_INTR_MIDI2          0x20
    #define CCS_INTR_PRO_MACRO      0x10
    #define CCS_INTR_FM             0x08
    #define CCS_INTR_PLAY_DS        0x04
    #define CCS_INTR_CONS_REC       0x02
    #define CCS_INTR_CONS_PLAY      0x01

#define CCS_ENVY_INDEX          0x0003 // CCI registers
#define CCS_ENVY_DATA           0x0004

#define CCS_MIDI1_DATA          0x000C
#define CCS_MIDI1_CMD_STATUS    0x000D

#define CCS_I2C_DEV_ADDRESS     0x0010   // check CCS13 before accessing
    #define CCS_ADDRESS_MASK        0xFE // I2C device address (bit 7:1)
    #define CCS_ADDRESS_SHIFT       1
    #define CCS_ADDRESS_WRITE       0x01 // 0: read, 1:write


#define CCS_I2C_ADDR            0x0011   // byte address to read/write
#define CCS_I2C_DATA            0x0012
#define CCS_I2C_STATUS          0X0013
    #define CCS_I2C_EPROM           0x80 // 1: E2PROM connected
    #define CCS_I2C_BUSY            0x01 // 0: idle, 1: busy


#define CCS_CONS_REC_ADDRESS    0x0014
#define CCS_CONS_REC_LENGTH     0x0018


#define CCS_MIDI2_DATA          0x001C
#define CCS_MIDI2_CMD_STATUS    0x001D

#define CCS_TIMER               0x001E // word
    #define CCS_TIMER_ENABLE        0x8000
    #define CCS_TIMER_MASK          0x7FFF // bits 0 - 14: write to set up the period for the internal 15 bits timer to generate an interrupt


#define CCI_GPIO_DATA           0x20
#define CCI_GPIO_MASK           0x21 // 0 in mask means CCI_GPIO_DATA register bit can be written
#define CCI_GPIO_DIR            0x22 // 1 = output

#define CCI_CONS_POWER_DOWN     0x30
#define CCI_PRO_POWER_DOWN      0x31




// ----------MT regs-------------------

#define MT_INTR_MASK_STATUS     0x0000
    #define MT_REC_MASK             0x80
    #define MT_PLAY_MASK            0x40
    #define MT_REC_STATUS           0x02
    #define MT_PLAY_STATUS          0x01

#define MT_SAMPLERATE           0x0001   // in slave mode (SPDIF is master): 256X master clock alone selects rate
    #define MT_SPDIF_MASTER         0x10 // 
    #define MT_RATE_MASK            0x0F // ignored if MT_SPDIF_MASTER = 

#define MT_I2S_FORMAT           0x0002
    #define MT_CLOCK_128x           0x08 // 0: 256x (default), 1: 128x
    #define MT_CLOCK_48bpf          0x04 // 0: 256x (default), 1: 128x

#define MT_AC97_REG             0x0004 // AC'97 register index
#define MT_AC97_CMD_STATUS      0x0005 // valid when CCS_ACLINK_TYPE == 0
    #define MT_AC97_RESET           0x80 // cold reset (alone will put it into master mode)
    #define MT_AC97_WARM_RESET      0X40 // when used together with MT_AC97_RESET, will set external VIA AC'97 to slave mode
    #define MT_AC97_WRITE           0X20 // write 1 for write mode, reading a 1 is WIP
    #define MT_AC97_READ            0x10 // write 1 for read mode, reading a 1 is RIP
    #define MT_AC97_READY           0x08 // codec ready status
    #define MT_AC97_ID_MASK         0x03 // bit 0:1 is ID mode when is split mode.
#define MT_AC97_DATA            0x0006
    
    
#define MT_DMA_PB_ADDRESS       0x0010 // long: start address of interleaved playback buffer (long boundary), in long units
#define MT_DMA_PB_LENGTH        0x0014 // word: DMA size - 1, read: counter
#define MT_DMA_PB_INTLEN        0x0016 

#define MT_DMA_CONTROL          0x0018 // start/stop (use read-modify-write)
    #define MT_REC_START             0x04
    #define MT_PAUSE                 0x02
    #define MT_PLAY_START            0x01


// record pair registers
#define MT_DMA_REC_ADDRESS      0x0020
#define MT_DMA_REC_LENGTH       0x0024
#define MT_DMA_REC_INTLEN       0x0026


#define ICE1712_DELTA_DFS 0x01		/* fast/slow sample rate mode */
#define ICE1712_DELTA_AP_CCLK	0x02	/* SPI clock */
					/* (clocking on rising edge - 0->1) */
#define ICE1712_DELTA_AP_DIN	0x04	/* data input */
#define ICE1712_DELTA_AP_DOUT	0x08	/* data output */
#define ICE1712_DELTA_AP_CS_DIGITAL 0x10 /* CS8427 chip select */
					/* low signal = select */
#define ICE1712_DELTA_AP_CS_CODEC 0x20	/* AK4528 (audiophile), AK4529 (Delta410) chip select */
					/* low signal = select */


/* 0x01 = DFS */
#define ICE1712_DELTA_1010LT_CCLK	0x02	/* SPI clock (AK4524 + CS8427) */
#define ICE1712_DELTA_1010LT_DIN	0x04	/* data input (CS8427) */
#define ICE1712_DELTA_1010LT_DOUT	0x08	/* data output (AK4524 + CS8427) */
#define ICE1712_DELTA_1010LT_CS		0x70	/* mask for CS address */
#define ICE1712_DELTA_1010LT_CS_CHIP_A	0x00	/* AK4524 #0 */
#define ICE1712_DELTA_1010LT_CS_CHIP_B	0x10	/* AK4524 #1 */
#define ICE1712_DELTA_1010LT_CS_CHIP_C	0x20	/* AK4524 #2 */
#define ICE1712_DELTA_1010LT_CS_CHIP_D	0x30	/* AK4524 #3 */
#define ICE1712_DELTA_1010LT_CS_CS8427	0x40	/* CS8427 */
#define ICE1712_DELTA_1010LT_CS_NONE	0x50	/* nothing */
#define ICE1712_DELTA_1010LT_WORDCLOCK 0x80	/* sample clock source: 0 = Word Clock Input, 1 = S/PDIF Input ??? */




#endif /* _REGS_H */
