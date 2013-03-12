/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/expansion.h>
#include "DriverData.h"
#include "ak_codec.h"
#include "regs.h"
#include "misc.h"


#define snd_akm4xxx_get(ak,chip,reg) (ak)->images[(chip) * 16 + (reg)]
#define snd_akm4xxx_set(ak,chip,reg,val) ((ak)->images[(chip) * 16 + (reg)] = (val))
#define snd_akm4xxx_get_ipga(ak,chip,reg) (ak)->ipga_gain[chip][(reg)-4]
#define snd_akm4xxx_set_ipga(ak,chip,reg,val) ((ak)->ipga_gain[chip][(reg)-4] = (val))


void akm4xxx_write(struct CardData *card, struct akm_codec *codec, int chip, unsigned char addr, unsigned char data)
{
   unsigned int tmp;
   int idx;
   unsigned int addrdata = 0;
   struct PCIDevice *dev = card->pci_dev;
   unsigned long base = card->iobase;

   if (codec->newflag)
   {
      akm4xxx_write_new(card, codec, chip, addr, data);
      return;
   }

   //DEBUGPRINTF("AKM: m = %x, %x, data = %x\n", INBYTE(base + 0x1F), INWORD(base + 0x16), data);
   //DEBUGPRINTF("AKM: %x, %x\n", addr, data);

   tmp = GetGPIOData(card, base);
    
   tmp |= codec->addflags;
   
   if (codec->cif)
   {
        tmp |= codec->csmask;
   }
   else
   {
        tmp &= ~codec->csmask; // cs down
        SetGPIOData(card, base, tmp); // set CS low
   }

  	MicroDelay(1);
   

	/* build I2C address + data byte */
	addrdata = (codec->caddr << 6) | 0x20 | (addr & 0x1f); // Chip Address in C1/C0 | r/w bit on (=write) | address & 5 left over bit positions
	addrdata = (addrdata << 8) | data;

	for (idx = 15; idx >= 0; idx--) {

		/* drop clock */
		tmp &= ~codec->clockmask;
		SetGPIOData(card, base, tmp);
      MicroDelay(1);

		/* set data */
		if (addrdata & (1 << idx))
			tmp |= codec->datamask;
		else
			tmp &= ~codec->datamask;

		SetGPIOData(card, base, tmp);
		MicroDelay(1);
      
		/* raise clock */
		tmp |= codec->clockmask;
		SetGPIOData(card, base, tmp);
		MicroDelay(1);
	}

	/* assert a cs pulse to trigger */
   //tmp &= ~codec->clockmask;
   
   if (codec->cif)
   {
        tmp &= ~codec->csmask;
        SetGPIOData(card, base, tmp);
	    MicroDelay(1);
   }

	tmp |= codec->csmask;
	SetGPIOData(card, base, tmp);
	MicroDelay(1);
}


// with alsa-like flags
void akm4xxx_write_new(struct CardData *card, struct akm_codec *priv, int chip, unsigned char addr, unsigned char data)
{
   unsigned int tmp;
   int idx;
   unsigned int addrdata = 0;
   struct PCIDevice *dev = card->pci_dev;
   unsigned long base = card->iobase;

   //DEBUGPRINTF("AKM: %x, %x\n", addr, data);

   tmp = GetGPIOData(card, base);
   tmp |= priv->addflags;
   tmp &= ~priv->totalmask;
   if (priv->csmask == priv->csaddr) {
		if (priv->cif) {
			tmp |= priv->csmask; /* start without chip select */
		}  else {
			tmp &= ~priv->csmask; /* chip select low */
            SetGPIOData(card, base, tmp);
            MicroDelay(1);
		}
	} else {
		/* doesn't handle cf=1 yet */
		tmp &= ~priv->csmask;
		tmp |= priv->csaddr;
        SetGPIOData(card, base, tmp);
        MicroDelay(1);
	}
  

	/* build I2C address + data byte */
	addrdata = (priv->caddr << 6) | 0x20 | (addr & 0x1f); // Chip Address in C1/C0 | r/w bit on (=write) | address & 5 left over bit positions
	addrdata = (addrdata << 8) | data;

	for (idx = 15; idx >= 0; idx--) {

		/* drop clock */
		tmp &= ~priv->clockmask;
		SetGPIOData(card, base, tmp);
        MicroDelay(1);

		/* set data */
		if (addrdata & (1 << idx))
			tmp |= priv->datamask;
		else
			tmp &= ~priv->datamask;

		SetGPIOData(card, base, tmp);
		MicroDelay(1);

		/* raise clock */
		tmp |= priv->clockmask;
		SetGPIOData(card, base, tmp);
		MicroDelay(1);
	}

   if (priv->csmask == priv->csaddr)
   {
      if (priv->cif) {
        tmp &= ~priv->csmask;
        SetGPIOData(card, base, tmp);
	    MicroDelay(1);
        }
      tmp |= priv->csmask;
   }
   else {
    tmp &= ~priv->csmask;
	tmp |= priv->csnone;
   }

	SetGPIOData(card, base, tmp);
	MicroDelay(1);
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
		0x06, 0x7E, /* 6: DAC left muted */
		0x07, 0x7E, /* 7: DAC right muted */
		0xff, 0xff
	};
	static unsigned char inits_ak4528[] = {
		0x00, 0x07, /* 0: all power up */
		0x01, 0x00, /* 1: ADC/DAC reset */
		0x02, 0x60, /* 2: 24bit I2S */
		0x03, 0x0d, /* 3: deemphasis off, turn LR highpass filters on */
		0x01, 0x03, /* 1: ADC/DAC enable */
		0x04, 0x00, /* 4: ADC left muted */
		0x05, 0x00, /* 5: ADC right muted */
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
      0x01, 0x03, // soft mute, no reset
      0x03, 0x00, // vol mute
      0x04, 0x00, // vol mute
      0x02, 0x01, // power down dacs
      
      0x01, 0x02, // soft mute + reset
      0x01, 0x03, // soft mute, no reset
      
      0x00, 0x87, // auto + I2S + no reset
      0x01, 0x01, // soft mute off
		0x02, 0x1F, /* double speed + all DAC's power up */
		0x03, 0x00, /* 3: de-emphasis 44.1 kHz */
      
		0x04, 0xFF, /* 4: LOUT1 volume */
		0x05, 0xFF, /* 5: ROUT1 volume */
		0x06, 0xFF, /* 6: LOUT2 volume */
		0x07, 0xFF, /* 7: ROUT2 volume */
		0x08, 0xFF, /* 8: LOUT3 volume */
		0x09, 0xFF, /* 9: ROUT3 volume */
		0x0a, 0x00, /* a: DATT speed=0, ignore DZF */
	    0xff, 0xff
	};

    
    static unsigned char inits_ak4358[] = {
		0x01, 0x02, /* 1: reset and soft-mute */
		0x00, 0x87, /* 0: mode3(i2s), auto-clock detect, disable DZF, sharp roll-off, RSTN#=0 */
		0x01, 0x01,
        0x02, 0x4F, /* 2: DA's power up, normal speed, RSTN#=0 */
		// 0x02, 0x2e, /* quad speed */
		0x03, 0x01, /* 3: de-emphasis off */
		0x04, 0xFF, /* 4: LOUT1 volume */
		0x05, 0xFF, /* 5: ROUT1 volume */
		0x06, 0xFF, /* 6: LOUT2 volume */
		0x07, 0xFF, /* 7: ROUT2 volume */
		0x08, 0xFF, /* 8: LOUT3 volume */
		0x09, 0xFF, /* 9: ROUT3 volume */
		0x0b, 0x00, /* b: LOUT4 volume */
		0x0c, 0x00, /* c: ROUT4 volume */
		0x0a, 0x00, /* a: DATT speed=0, ignore DZF */
        0x0d, 0xFF,
		0xff, 0xff
	};
	static unsigned char inits_ak4381[] = {
      0x00, 0x00, // power down
      0x00, 0x8F, /* 0: mode3(i2s), auto-clock detect, power up */
      0x01, 0x08, // de-emphases on 44.1 KHz, double speed
      0x03, 0xFF, /* 3: LATT 0 */
		0x04, 0xFF, /* 4: RATT 0 */
   	0xff, 0xff
	};

    static unsigned char inits_ak5365[] = {
		0x00, 0x00, /* power down */
		0x00, 0x01, /* power up */
        0x01, 0x01, /* 1 = line in */
        0x02, 0x08,
		0x03, 0x00, /* */
		0x04, 0x80, /* gain 0 dB */
		0x05, 0x80, /* gain 0 dB */
		0xff, 0xff
	};

/*
 * initialize all the ak4xxx chips
 */
void Init_akm4xxx(struct CardData *card, struct akm_codec *codec)
{
	
	int chip, num_chips;
	unsigned char *ptr, reg, data, *inits;

	switch (codec->type) {
	case AKM4524:
		inits = inits_ak4524;
		num_chips = 1;
		break;
	case AKM4528:
		inits = inits_ak4528;
		num_chips = 8 / 2;
		break;
	case AKM4529:
		inits = inits_ak4529;
		num_chips = 1;
		break;
	case AKM4355:
		inits = inits_ak4355;
		num_chips = 1;
		break;
    case AKM4358:
        inits = inits_ak4358;
        num_chips = 1;
        break;
	case AKM4381:
		inits = inits_ak4381;
		num_chips = 8 / 2;
		break;

    case AKM5365:
         inits = inits_ak5365;
        num_chips = 1;
        break;
	default:
		inits = inits_ak4524;
      num_chips = 8 / 2;
		return;
	}
   
	//for (chip = 0; chip < num_chips; chip++) {
		ptr = inits;
		while (*ptr != 0xff) {
			reg = *ptr++;
			data = *ptr++;
			akm4xxx_write(card, codec, 0, reg, data);
         MicroDelay(5);
		}
	//}
}

#define AK_GET_CHIP(val)		(((val) >> 8) & 0xff)
#define AK_GET_ADDR(val)		((val) & 0xff)
#define AK_GET_SHIFT(val)		(((val) >> 16) & 0x7f)
#define AK_GET_INVERT(val)		(((val) >> 23) & 1)
#define AK_GET_MASK(val)		(((val) >> 24) & 0xff)
#define AK_COMPOSE(chip,addr,shift,mask) (((chip) << 8) | (addr) | ((shift) << 16) | ((mask) << 24))
#define AK_INVERT 			(1<<23)


#if 0
static int akm4xxx_volume_info(kcontrol_t *kcontrol, ctl_elem_info_t * uinfo)
{
	unsigned int mask = AK_GET_MASK(kcontrol->private_value);

	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = mask;
	return 0;
}

static int akm4xxx_volume_get(kcontrol_t *kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	int invert = AK_GET_INVERT(kcontrol->private_value);
	unsigned int mask = AK_GET_MASK(kcontrol->private_value);
	unsigned char val = akm4xxx_get(ak, chip, addr);
	
	ucontrol->value.integer.value[0] = invert ? mask - val : val;
	return 0;
}

static int akm4xxx_volume_put(kcontrol_t *kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	int invert = AK_GET_INVERT(kcontrol->private_value);
	unsigned int mask = AK_GET_MASK(kcontrol->private_value);
	unsigned char nval = ucontrol->value.integer.value[0] % (mask+1);
	int change;

	if (invert)
		nval = mask - nval;
	change = akm4xxx_get(ak, chip, addr) != nval;
	if (change)
		akm4xxx_write(ak, chip, addr, nval);
	return change;
}

static int akm4xxx_ipga_gain_info(kcontrol_t *kcontrol, ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 36;
	return 0;
}

static int akm4xxx_ipga_gain_get(kcontrol_t *kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	ucontrol->value.integer.value[0] = akm4xxx_get_ipga(ak, chip, addr) & 0x7f;
	return 0;
}

static int akm4xxx_ipga_gain_put(kcontrol_t *kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	unsigned char nval = (ucontrol->value.integer.value[0] % 37) | 0x80;
	int change = akm4xxx_get_ipga(ak, chip, addr) != nval;
	if (change)
		akm4xxx_write(ak, chip, addr, nval);
	return change;
}

static int akm4xxx_deemphasis_info(kcontrol_t *kcontrol, ctl_elem_info_t *uinfo)
{
	static char *texts[4] = {
		"44.1kHz", "Off", "48kHz", "32kHz",
	};
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = 4;
	if (uinfo->value.enumerated.item >= 4)
		uinfo->value.enumerated.item = 3;
	strcpy(uinfo->value.enumerated.name, texts[uinfo->value.enumerated.item]);
	return 0;
}

static int akm4xxx_deemphasis_get(kcontrol_t * kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	int shift = AK_GET_SHIFT(kcontrol->private_value);
	ucontrol->value.enumerated.item[0] = (akm4xxx_get(ak, chip, addr) >> shift) & 3;
	return 0;
}

static int akm4xxx_deemphasis_put(kcontrol_t *kcontrol, ctl_elem_value_t *ucontrol)
{
	akm4xxx_t *ak = _kcontrol_chip(kcontrol);
	int chip = AK_GET_CHIP(kcontrol->private_value);
	int addr = AK_GET_ADDR(kcontrol->private_value);
	int shift = AK_GET_SHIFT(kcontrol->private_value);
	unsigned char nval = ucontrol->value.enumerated.item[0] & 3;
	int change;
	
	nval = (nval << shift) | (akm4xxx_get(ak, chip, addr) & ~(3 << shift));
	change = akm4xxx_get(ak, chip, addr) != nval;
	if (change)
		akm4xxx_write(ak, chip, addr, nval);
	return change;
}

/*
 * build AK4xxx controls
 */

int akm4xxx_build_controls(akm4xxx_t *ak)
{
	unsigned int idx, num_emphs;
	int err;

	for (idx = 0; idx < ak->num_dacs; ++idx) {
		kcontrol_t ctl;
		memset(&ctl, 0, sizeof(ctl));
		strcpy(ctl.id.name, "DAC Volume");
		ctl.id.index = idx + ak->idx_offset * 2;
		ctl.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		ctl.count = 1;
		ctl.info = akm4xxx_volume_info;
		ctl.get = akm4xxx_volume_get;
		ctl.put = akm4xxx_volume_put;
		switch (ak->type) {
		case AK4524:
			ctl.private_value = AK_COMPOSE(idx/2, (idx%2) + 6, 0, 127); /* register 6 & 7 */
			break;
		case AK4528:
			ctl.private_value = AK_COMPOSE(idx/2, (idx%2) + 4, 0, 127); /* register 4 & 5 */
			break;
		case AK4529: {
			int val = idx < 6 ? idx + 2 : (idx - 6) + 0xb; /* registers 2-7 and b,c */
			ctl.private_value = AK_COMPOSE(0, val, 0, 255) | AK_INVERT;
			break;
		}
		case AK4355:
			ctl.private_value = AK_COMPOSE(0, idx + 4, 0, 255); /* register 4-9, chip #0 only */
			break;
		case AK4381:
			ctl.private_value = AK_COMPOSE(idx/2, (idx%2) + 3, 0, 255); /* register 3 & 4 */
			break;
		default:
			return -EINVAL;
			}
		ctl.private_data = ak;
		if ((err = ctl_add(ak->card, ctl_new(&ctl, SNDRV_CTL_ELEM_ACCESS_READ|SNDRV_CTL_ELEM_ACCESS_WRITE))) < 0)
			return err;
	}
	for (idx = 0; idx < ak->num_adcs && ak->type == AK4524; ++idx) {
		kcontrol_t ctl;
		memset(&ctl, 0, sizeof(ctl));
		strcpy(ctl.id.name, "ADC Volume");
		ctl.id.index = idx + ak->idx_offset * 2;
		ctl.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		ctl.count = 1;
		ctl.info = akm4xxx_volume_info;
		ctl.get = akm4xxx_volume_get;
		ctl.put = akm4xxx_volume_put;
		ctl.private_value = AK_COMPOSE(idx/2, (idx%2) + 4, 0, 127); /* register 4 & 5 */
		ctl.private_data = ak;
		if ((err = ctl_add(ak->card, ctl_new(&ctl, SNDRV_CTL_ELEM_ACCESS_READ|SNDRV_CTL_ELEM_ACCESS_WRITE))) < 0)
			return err;
		memset(&ctl, 0, sizeof(ctl));
		strcpy(ctl.id.name, "IPGA Analog Capture Volume");
		ctl.id.index = idx + ak->idx_offset * 2;
		ctl.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		ctl.count = 1;
		ctl.info = akm4xxx_ipga_gain_info;
		ctl.get = akm4xxx_ipga_gain_get;
		ctl.put = akm4xxx_ipga_gain_put;
		ctl.private_value = AK_COMPOSE(idx/2, (idx%2) + 4, 0, 0); /* register 4 & 5 */
		ctl.private_data = ak;
		if ((err = ctl_add(ak->card, ctl_new(&ctl, SNDRV_CTL_ELEM_ACCESS_READ|SNDRV_CTL_ELEM_ACCESS_WRITE))) < 0)
			return err;
	}
	if (ak->type == AK4355)
		num_emphs = 1;
	else
		num_emphs = ak->num_dacs / 2;
	for (idx = 0; idx < num_emphs; idx++) {
		kcontrol_t ctl;
		memset(&ctl, 0, sizeof(ctl));
		strcpy(ctl.id.name, "Deemphasis");
		ctl.id.index = idx + ak->idx_offset;
		ctl.id.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
		ctl.count = 1;
		ctl.info = akm4xxx_deemphasis_info;
		ctl.get = akm4xxx_deemphasis_get;
		ctl.put = akm4xxx_deemphasis_put;
		switch (ak->type) {
		case AK4524:
		case AK4528:
			ctl.private_value = AK_COMPOSE(idx, 3, 0, 0); /* register 3 */
			break;
		case AK4529: {
			int shift = idx == 3 ? 6 : (2 - idx) * 2;
			ctl.private_value = AK_COMPOSE(0, 8, shift, 0); /* register 8 with shift */
			break;
		}
		case AK4355:
			ctl.private_value = AK_COMPOSE(idx, 3, 0, 0);
			break;
		case AK4381:
			ctl.private_value = AK_COMPOSE(idx, 1, 1, 0);
			break;
		}
		ctl.private_data = ak;
		if ((err = ctl_add(ak->card, ctl_new(&ctl, SNDRV_CTL_ELEM_ACCESS_READ|SNDRV_CTL_ELEM_ACCESS_WRITE))) < 0)
			return err;
	}
	return 0;
}

static int __init alsa_akm4xxx_module_init(void)
{
	return 0;
}
        
static void __exit alsa_akm4xxx_module_exit(void)
{
}
        
module_init(alsa_akm4xxx_module_init)
module_exit(alsa_akm4xxx_module_exit)

EXPORT_SYMBOL(akm4xxx_write);
EXPORT_SYMBOL(akm4xxx_reset);
EXPORT_SYMBOL(akm4xxx_init);
EXPORT_SYMBOL(akm4xxx_build_controls);
#endif

