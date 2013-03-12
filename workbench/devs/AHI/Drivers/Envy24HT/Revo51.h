/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef REVO51_H
#define REVO51_H

void Revo51_Init(struct CardData *card);

//#define PHASE88_RW      0x08 // 1 = write
#define REVO51_I2C_DATA    0x40    /* I2C: PT 2258 SDA (on revo51) */
#define REVO51_I2C_CLOCK   0x80    /* I2C: PT 2258 SCL (on revo51) */

#define REVO51_2258_MUTE 0xF9
#define REVO51_2258_UNMUTE 0xF8

static const unsigned char pt2258_db_codes[12] = {
	0x80, 0x90,		/* channel 1: -10dB, -1dB */
	0x40, 0x50,		/* channel 2: -10dB, -1dB */
	0x00, 0x10,		/* channel 3: -10dB, -1dB */
	0x20, 0x30,		/* channel 4: -10dB, -1dB */
	0x60, 0x70,		/* channel 5: -10dB, -1dB */
	0xa0, 0xb0		/* channel 6: -10dB, -1dB */
};

#endif

