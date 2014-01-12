/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AK_CODEC_H
#define AK_CODEC_H


#include "DriverData.h"

enum akm_types {AKM4524, AKM4528, AKM4529, AKM4355, AKM4381};

struct akm_codec
{
    unsigned int caddr;
    unsigned int cif;
    unsigned int data_mask;
    unsigned int clk_mask;
    
    unsigned int cs_mask;
    unsigned int cs_addr;
    unsigned int cs_none;
    unsigned int add_flags;
    unsigned int mask_flags;
    
    enum akm_types type;
};

void Init_akm4xxx(struct CardData *card, enum akm_types type, enum Model CardModel);
void akm4xxx_write(struct CardData *card, struct akm_codec *codec, int chip, unsigned char addr, unsigned char data);


#endif

