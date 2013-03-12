/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/expansion.h>
#include "misc.h"
#include "DriverData.h"
#include "regs.h"
#include "ak_codec.h"
#include "Revo51.h"


static void Revo51_Start(struct CardData *card)
{
	unsigned char mask;

	SaveGPIO(card->pci_dev, card);
}


static void Revo51_Stop(struct CardData *card)
{
	RestoreGPIO(card->pci_dev, card);
}



// Set the direction of the CLK and SDA lines:
// For sending, use 1, 1
static void  Revo51_SetDir_CLK_SDA(struct CardData *card, int clock, int data)
{
	unsigned int mask = 0, val;

    val = 0;
	if (clock)
		val = REVO51_I2C_CLOCK; /* write SCL */
	if (data)
		val |= REVO51_I2C_DATA; /* write SDA */

   mask = REVO51_I2C_CLOCK | REVO51_I2C_DATA;
   card->gpio_dir &= ~mask;
   card->gpio_dir |= val;
   SetGPIODir(card->pci_dev, card, card->gpio_dir);
   SetGPIOMask(card, card->iobase, ~mask);

}



// Write data to the SDA line and clock to the SCL
static void Revo51_Write_CLK_SDA(struct CardData *card, int clk, int data)
{
	unsigned int val = 0, mask = REVO51_I2C_CLOCK | REVO51_I2C_DATA;;

	if (clk)
		val |= REVO51_I2C_CLOCK;
	if (data)
		val |= REVO51_I2C_DATA;
	
    card->gpio_dir |= mask;
	SetGPIODir(card->pci_dev, card, card->gpio_dir);
    SetGPIOMask(card, card->iobase, ~mask);
	SetGPIOData(card, card->iobase, mask & val);
	
   MicroDelay(5);
}


static int Revo51_GetDataBit(struct CardData *card, int ack)
{
	int bit;

	if (ack)
		MicroDelay(5);

    // get data bit from the GPIO pines
    card->gpio_dir &= ~REVO51_I2C_DATA;
    SetGPIODir(card->pci_dev, card, card->gpio_dir);
    bit = GetGPIOData(card, card->iobase) & REVO51_I2C_DATA ? 1 : 0;

	return bit;
}


static struct I2C_bit_ops Revo51_bit_ops = {
	Revo51_Start,
	Revo51_Stop,
	Revo51_SetDir_CLK_SDA,
	Revo51_Write_CLK_SDA,
	Revo51_GetDataBit,
};


void Revo51_Init(struct CardData *card)
{
   unsigned char bytes[4];

   /* create i2c devices */
   card->i2c = AllocI2C(0x40);
   card->i2c->bit = &Revo51_bit_ops;
   
   card->bit_ops = &Revo51_bit_ops;

   //DEBUGPRINTF("Resetting PT2258\n");

   // step 1: clear reg 0xC0
   bytes[0] = 0xC0;
   WriteBytesI2C(card, card->i2c, bytes, 1);

   // unmute all channels
   bytes[0] = REVO51_2258_UNMUTE;
   WriteBytesI2C(card, card->i2c, bytes, 1);

   // set all 6 channels to no attenuation (0dB)
   bytes[0] = 0xD0;
   bytes[1] = 0xE0;
   WriteBytesI2C(card, card->i2c, bytes, 2);
}

