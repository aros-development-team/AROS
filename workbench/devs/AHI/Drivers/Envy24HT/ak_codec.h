/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef AK_CODEC_H
#define AK_CODEC_H

struct CardData;

enum akm_types {AKM4524, AKM4528, AKM4529, AKM4355, AKM4381, AKM4358, AKM5365};

struct akm_codec
{
    unsigned int caddr; // caddr
    unsigned int cif;
    unsigned int datamask; // mask
    unsigned int clockmask; // mask
    unsigned int csmask; // cs mask
    unsigned int addflags;
    enum akm_types type;
    unsigned int csaddr; // nieuw
    unsigned int csnone;
    unsigned int totalmask;
    unsigned int newflag;
};

#if 0
struct snd_ak4xxx_private {
	unsigned int cif: 1;		/* CIF mode */
	unsigned char caddr;		/* C0 and C1 bits */
	unsigned int data_mask;		/* DATA gpio bit */
	unsigned int clk_mask;		/* CLK gpio bit */
	unsigned int cs_mask;		/* bit mask for select/deselect address */
	unsigned int cs_addr;		/* bits to select address */
	unsigned int cs_none;		/* bits to deselect address */
	unsigned int add_flags;		/* additional bits at init */
	unsigned int mask_flags;	/* total mask bits */
	struct snd_akm4xxx_ops {
		void (*set_rate_val)(struct snd_akm4xxx *ak, unsigned int rate);
	} ops;
};
#endif

void Init_akm4xxx(struct CardData *card, struct akm_codec *codec);
void akm4xxx_write(struct CardData *card, struct akm_codec *codec, int chip, unsigned char addr, unsigned char data);
void akm4xxx_write_new(struct CardData *card, struct akm_codec *codec, int chip, unsigned char addr, unsigned char data);
#endif

