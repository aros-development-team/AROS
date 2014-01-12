/*
    Copyright � 2004-2014, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include "DriverData.h"
#include "Phase88.h"
#include "ak_codec.h"
#include "regs.h"
#include "misc.h"


#define snd_akm4xxx_get(ak,chip,reg) (ak)->images[(chip) * 16 + (reg)]
#define snd_akm4xxx_set(ak,chip,reg,val) ((ak)->images[(chip) * 16 + (reg)] = (val))
#define snd_akm4xxx_get_ipga(ak,chip,reg) (ak)->ipga_gain[chip][(reg)-4]
#define snd_akm4xxx_set_ipga(ak,chip,reg,val) ((ak)->ipga_gain[chip][(reg)-4] = (val))

#define ice card
#define snd_ice1712_gpio_read GetGPIOData
#define snd_ice1712_gpio_write SetGPIOData
#define udelay MicroDelay

void akm4xxx_write(struct CardData *card, struct akm_codec *priv, int chip, unsigned char addr, unsigned char data)
{
    unsigned int tmp;
    int idx;
    unsigned int addrdata = 0;
    struct PCIDevice *dev = card->pci_dev;
    unsigned long base = card->iobase;

    tmp = snd_ice1712_gpio_read(ice);
	tmp |= priv->add_flags;
	tmp &= ~priv->mask_flags;
	if (priv->cs_mask == priv->cs_addr) {
		if (priv->cif) {
			tmp |= priv->cs_mask; /* start without chip select */
		}  else {
			tmp &= ~priv->cs_mask; /* chip select low */
			snd_ice1712_gpio_write(ice, tmp);
			udelay(1);
		}
	} else {
		/* doesn't handle cf=1 yet */
		tmp &= ~priv->cs_mask;
		tmp |= priv->cs_addr;
		snd_ice1712_gpio_write(ice, tmp);
		udelay(1);
	}

	/* build I2C address + data byte */
	addrdata = (priv->caddr << 6) | 0x20 | (addr & 0x1f);
	addrdata = (addrdata << 8) | data;
	for (idx = 15; idx >= 0; idx--) {
		/* drop clock */
		tmp &= ~priv->clk_mask;
		snd_ice1712_gpio_write(ice, tmp);
		udelay(1);
		/* set data */
		if (addrdata & (1 << idx))
			tmp |= priv->data_mask;
		else
			tmp &= ~priv->data_mask;
		snd_ice1712_gpio_write(ice, tmp);
		udelay(1);
		/* raise clock */
		tmp |= priv->clk_mask;
		snd_ice1712_gpio_write(ice, tmp);
		udelay(1);
	}

	if (priv->cs_mask == priv->cs_addr) {
		if (priv->cif) {
			/* assert a cs pulse to trigger */
			tmp &= ~priv->cs_mask;
			snd_ice1712_gpio_write(ice, tmp);
			udelay(1);
		}
		tmp |= priv->cs_mask; /* chip select high to trigger */
	} else {
		tmp &= ~priv->cs_mask;
		tmp |= priv->cs_none; /* deselect address */
	}
	snd_ice1712_gpio_write(ice, tmp);
	udelay(1);
}

void akm4xxx_write_old(struct CardData *card, struct akm_codec *codec, int chip, unsigned char addr, unsigned char data)
{
    unsigned char tmp;
	int idx;
	unsigned int addrdata = 0;
    struct PCIDevice *dev = card->pci_dev;
    unsigned long base = card->iobase;

    //IExec->DebugPrintF("m = %x, %x, data = %x\n", dev->InByte(base + 0x1F), dev->InWord(base + 0x16), data);

	tmp = GetGPIOData(card);
    
	//tmp |= codec->clock; // clock hi
    tmp &= ~codec->cs_mask; // cs down
    SetGPIOData(card, tmp); // set CS low
	MicroDelay(1);
   

	/* build I2C address + data byte */
	addrdata = (codec->caddr << 6) | 0x20 | (addr & 0x1f); // Chip Address in C1/C0 | r/w bit on (=write) | address & 5 left over bit positions
	addrdata = (addrdata << 8) | data;

	for (idx = 15; idx >= 0; idx--) {

		/* drop clock */
		tmp &= ~codec->clk_mask;
		SetGPIOData(card, tmp);
        MicroDelay(1);

		/* set data */
		if (addrdata & (1 << idx))
			tmp |= codec->data_mask;
		else
			tmp &= ~codec->data_mask;

		SetGPIOData(card, tmp);
		MicroDelay(1);
      
		/* raise clock */
		tmp |= codec->clk_mask;
		SetGPIOData(card, tmp);
		MicroDelay(1);
	}

	/* assert a cs pulse to trigger */
   //tmp &= ~codec->clock;
	tmp |= codec->cs_mask;
	SetGPIOData(card, tmp);
	MicroDelay(1);
   
   //tmp |= codec->clock;
   //SetGPIOData(card, tmp);
	//MicroDelay(1);
}


static void Phase88_akm4xxx_write(struct CardData *card, int chip, unsigned char addr, unsigned char data)
{
	unsigned int tmp;
	int idx;
	unsigned int addrdata;

	if (!(chip >= 0 && chip < 4))
      return;

   Phase88_ak4524_lock(card, chip);
   
	tmp = ReadCCI(card, CCI_GPIO_DATA);
	tmp |= PHASE88_RW;
   

	/* build I2C address + data byte */
	addrdata = (2 << 6) | 0x20 | (addr & 0x1f);
	addrdata = (addrdata << 8) | data;

	for (idx = 15; idx >= 0; idx--) {

		/* drop clock */
		tmp &= ~PHASE88_CLOCK;
		WriteCCI(card, CCI_GPIO_DATA, tmp);
      MicroDelay(1);

		/* set data */
		if (addrdata & (1 << idx))
			tmp |= PHASE88_DATA;
		else
			tmp &= ~PHASE88_DATA;
		WriteCCI(card, CCI_GPIO_DATA, tmp);
		MicroDelay(1);
      
		/* raise clock */
		tmp |= PHASE88_CLOCK;
		WriteCCI(card, CCI_GPIO_DATA, tmp);
		MicroDelay(1);
	}

	/* assert a cs pulse to trigger */
	WriteCCI(card, CCI_GPIO_DATA, tmp);
	MicroDelay(1);

   Phase88_ak4524_unlock(card, chip);
}


static unsigned char inits_ak4524[] = {
		0x00, 0x07, /* 0: all power up */
		0x01, 0x00, /* 1: ADC/DAC reset */
		0x02, 0x60, /* 2: 24bit I2S */
		0x03, 0x19, /* 3: deemphasis off */
		0x01, 0x03, /* 1: ADC/DAC enable */
		0x04, 0x00, /* 4: ADC left muted */
		0x05, 0x00, /* 5: ADC right muted */
		0x04, 0x80, /* 4: ADC IPGA gain 0dB */
		0x05, 0x80, /* 5: ADC IPGA gain 0dB */
		0x06, 0x7F, /* 6: DAC left full */
		0x07, 0x7F, /* 7: DAC right full */
		0xff, 0xff
	};
	static unsigned char inits_ak4528[] = {
		0x00, 0x07, /* 0: all power up */
		0x01, 0x00, /* 1: ADC/DAC reset */
		0x02, 0x60, /* 2: 24bit I2S */
		0x03, 0x01, /* 3: no highpass filters */
		0x01, 0x03, /* 1: ADC/DAC enable */
		0x04, 0x7F, /* 4: ADC left muted */
		0x05, 0x7F, /* 5: ADC right muted */
		0xff, 0xff
	};
	static unsigned char inits_ak4529[] = {
		0x09, 0x01, /* 9: ATS=0, RSTN=1 */
		0x0a, 0x3f, /* A: all power up, no zero/overflow detection */
		0x00, 0x0c, /* 0: TDM=0, 24bit I2S, SMUTE=0 */
		0x01, 0x00, /* 1: ACKS=0, ADC, loop off */
		0x02, 0xff, /* 2: LOUT1 muted */
		0x03, 0xff, /* 3: ROUT1 muted */
		0x04, 0xff, /* 4: LOUT2 muted */
		0x05, 0xff, /* 5: ROUT2 muted */
		0x06, 0xff, /* 6: LOUT3 muted */
		0x07, 0xff, /* 7: ROUT3 muted */
		0x0b, 0xff, /* B: LOUT4 muted */
		0x0c, 0xff, /* C: ROUT4 muted */
		0x08, 0x55, /* 8: deemphasis all off */
		0xff, 0xff
	};
	static unsigned char inits_ak4355[] = {
		0x01, 0x02, /* 1: reset and soft-mute */
		0x00, 0x06, /* 0: mode3(i2s), disable auto-clock detect, disable DZF, sharp roll-off, RSTN#=0 */
		0x02, 0x0e, /* 2: DA's power up, normal speed, RSTN#=0 */
		// 0x02, 0x2e, /* quad speed */
		0x03, 0x01, /* 3: de-emphasis off */
		0x04, 0x00, /* 4: LOUT1 volume muted */
		0x05, 0x00, /* 5: ROUT1 volume muted */
		0x06, 0x00, /* 6: LOUT2 volume muted */
		0x07, 0x00, /* 7: ROUT2 volume muted */
		0x08, 0x00, /* 8: LOUT3 volume muted */
		0x09, 0x00, /* 9: ROUT3 volume muted */
		0x0a, 0x00, /* a: DATT speed=0, ignore DZF */
		0x01, 0x01, /* 1: un-reset, unmute */
		0xff, 0xff
	};
	static unsigned char inits_ak4381[] = {
		0x00, 0x0c, /* 0: mode3(i2s), disable auto-clock detect */
		0x01, 0x02, /* 1: de-emphasis off, normal speed, sharp roll-off, DZF off */
		// 0x01, 0x12, /* quad speed */
		0x02, 0x00, /* 2: DZF disabled */
		0x03, 0x00, /* 3: LATT 0 */
		0x04, 0x00, /* 4: RATT 0 */
		0x00, 0x0f, /* 0: power-up, un-reset */
		0xff, 0xff
	};


/*
 * initialize all the ak4xxx chips
 */
void Init_akm4xxx(struct CardData *card, enum akm_types type, enum Model CardModel)
{
	
	int chip, num_chips;
	unsigned char *ptr, reg, data, *inits;

	switch (type) {
	case AKM4524:
		inits = inits_ak4524;
		num_chips = 8 / 2;
		break;
	case AKM4528:
		inits = inits_ak4528;
		num_chips = 1; //8 / 2;
		break;
	case AKM4529:
		inits = inits_ak4529;
		num_chips = 1;
		break;
	case AKM4355:
		inits = inits_ak4355;
		num_chips = 1;
		break;
	case AKM4381:
		inits = inits_ak4381;
		num_chips = 8 / 2;
		break;
	default:
		inits = inits_ak4524;
      num_chips = 8 / 2;
		return;
	}

   
    if (CardModel == PHASE88)
    {
    	for (chip = 0; chip < num_chips; chip++)
        {
		    ptr = inits;
    		while (*ptr != 0xff)
            {
	    		reg = *ptr++;
		    	data = *ptr++;
			    Phase88_akm4xxx_write(card, chip, reg, data);
            }
		}
	}
    else if (CardModel == MAUDIO_2496)
    {
        ptr = inits;
        while (*ptr != 0xff)
        {
	        reg = *ptr++;
		    data = *ptr++;
			akm4xxx_write(card, &(card->codec[0]), 0, reg, data);
        }
    }
    else if (CardModel == MAUDIO_DELTA44 || CardModel == MAUDIO_DELTA66)
    {
        ptr = inits;
        while (*ptr != 0xff)
        {
	        reg = *ptr++;
		    data = *ptr++;
			akm4xxx_write(card, &(card->codec[0]), 0, reg, data);
        }
    }
    else if (CardModel == MAUDIO_1010LT)
    {
        for (chip = 0; chip < num_chips; chip++)
        {
            ptr = inits;
            while (*ptr != 0xff)
            {
    	        reg = *ptr++;
    		    data = *ptr++;
    			akm4xxx_write(card, &card->codec[chip], chip, reg, data);
            }
        }
    }
}

#define AK_GET_CHIP(val)		(((val) >> 8) & 0xff)
#define AK_GET_ADDR(val)		((val) & 0xff)
#define AK_GET_SHIFT(val)		(((val) >> 16) & 0x7f)
#define AK_GET_INVERT(val)		(((val) >> 23) & 1)
#define AK_GET_MASK(val)		(((val) >> 24) & 0xff)
#define AK_COMPOSE(chip,addr,shift,mask) (((chip) << 8) | (addr) | ((shift) << 16) | ((mask) << 24))
#define AK_INVERT 			(1<<23)


