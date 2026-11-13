/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef PHASE88_H
#define PHASE88_H

int Phase88_Init(struct CardData *card);
void Phase88_ak4524_lock(struct CardData *card, int chip);
void Phase88_ak4524_unlock(struct CardData *card, int chip);


#define PHASE88_RW      0x08 // 1 = write
#define PHASE88_DATA    0x10
#define PHASE88_CLOCK   0x20

#endif

