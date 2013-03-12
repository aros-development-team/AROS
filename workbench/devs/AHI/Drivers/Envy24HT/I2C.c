/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include "I2C.h"
#include "DriverData.h"
#include "pci_wrapper.h"

static int I2C_bit_sendbytes(struct CardData *card, struct I2C *device, unsigned char *bytes, int count);
static int I2C_bit_readbytes(struct CardData *card, struct I2C *device, unsigned char *bytes, int count);
static int I2C_bit_probeaddr(struct CardData *card, unsigned short addr);

#ifdef __MORPHOS__
extern struct DriverBase* AHIsubBase;
#endif

struct I2C *AllocI2C(unsigned char addr)
{
	struct I2C *device;

	device = (struct I2C *) ALLOCVEC(sizeof(struct I2C), MEMF_CLEAR);
	if (device == NULL)
		return NULL;
      
	device->addr = addr;
    device->flags = 0;

	return device;
}


void FreeI2C(struct I2C *device)
{
	FREEVEC(device);
}


int WriteBytesI2C(struct CardData *card, struct I2C *device, unsigned char *bytes, int count)
{
	return I2C_bit_sendbytes(card, device, bytes, count);
}


int ReadBytesI2C(struct CardData *card, struct I2C *device, unsigned char *bytes, int count)
{
	return I2C_bit_readbytes(card, device, bytes, count);
}


int ProbeAddressI2C(struct CardData *card, unsigned short addr)
{
	return I2C_bit_probeaddr(card, addr);
}




// Let the specific chip set up GPIO dir and mask for example
// before we start feeding bits and bytes...
static inline void I2C_bit_hw_start(struct CardData *card)
{
	if (card->bit_ops->Start)
		card->bit_ops->Start(card);
}


static inline void I2C_bit_hw_stop(struct CardData *card)
{
	if (card->bit_ops->Stop)
		card->bit_ops->Stop(card);
}


static void I2C_bit_direction(struct CardData *card, int clock, int data)
{
	if (card->bit_ops->SetDir_CLK_SDA)
		card->bit_ops->SetDir_CLK_SDA(card, clock, data);
}


static void I2C_bit_set(struct CardData *card, int clock, int data)
{
	card->bit_ops->Write_CLK_SDA(card, clock, data);
}



static int I2C_bit_data(struct CardData *card, int ack)
{
	return card->bit_ops->GetData(card, ack);
}



// See page 17 and 18 of the I2C spec, version 2.1 January 2000,
// especially, figure 18 (START byte procedure)
static void I2C_bit_start(struct CardData *card)
{
   // chip-specific init
	I2C_bit_hw_start(card); 
	
   // we're gonna write to both
   I2C_bit_direction(card, 1, 1);
	I2C_bit_set(card, 1, 1);
	I2C_bit_set(card, 1, 0);
	I2C_bit_set(card, 0, 0);
}


// see figure 10 and 19
static void I2C_bit_stop(struct CardData *card)
{
	I2C_bit_set(card, 0, 0);
	I2C_bit_set(card, 1, 0);
	I2C_bit_set(card, 1, 1);
   
   // chip-specific stop
	I2C_bit_hw_stop(card);
}


// data gets send on high clock
static void I2C_bit_send(struct CardData *card, int data)
{
	I2C_bit_set(card, 0, data);
	I2C_bit_set(card, 1, data);
	I2C_bit_set(card, 0, data);
}


static int I2C_bit_ack(struct CardData *card)
{
	int ack;

   // figure 7
	I2C_bit_set(card, 0, 1);
	I2C_bit_set(card, 1, 1);
   
   // then read the ack bit
	I2C_bit_direction(card, 1, 0);
	ack = I2C_bit_data(card, 1);
   
   
	I2C_bit_direction(card, 1, 1);
	I2C_bit_set(card, 0, 1);

	return ack;
}


static int I2C_bit_sendbyte(struct CardData *card, unsigned char data)
{
	int i, err;

   // send the byte bit for bit, MSB first
	for (i = 7; i >= 0; i--)
		I2C_bit_send(card, (data & (1 << i)));
   
	if ((err = I2C_bit_ack(card)) < 0)
		return err;

	return 0;
}


static int I2C_bit_readbyte(struct CardData *card, int last)
{
	int i;
	unsigned char data = 0;

	I2C_bit_set(card, 0, 1);
	I2C_bit_direction(card, 1, 0);

	for (i = 7; i >= 0; i--) {
		I2C_bit_set(card, 1, 1);
      
		if (I2C_bit_data(card, 0))
			data |= (1 << i);
		
      I2C_bit_set(card, 0, 1);
   	}

	I2C_bit_direction(card, 1, 1);
	I2C_bit_send(card, last);

	return data;
}


static int I2C_bit_sendbytes(struct CardData *card, struct I2C *device, unsigned char *bytes, int count)
{
	int err, res = 0;

	I2C_bit_start(card);

   // first send address of the I2C chip we want to reach
	if ((err = I2C_bit_sendbyte(card, device->addr << 1)) < 0) {
		I2C_bit_hw_stop(card);
		return err;
   	}
	
   // then send all consecutive bytes with an 'ack' in between
   while (count-- > 0) {
		if ((err = I2C_bit_sendbyte(card, *bytes++)) < 0) {
			I2C_bit_hw_stop(card);
			return err;
	   	}
	
   	res++;
	   }
      
	I2C_bit_stop(card);
   
	return res;
}


static int I2C_bit_readbytes(struct CardData *card, struct I2C *device, unsigned char *bytes, int count)
{
	int err, res = 0;

	I2C_bit_start(card);
	
   if ((err = I2C_bit_sendbyte(card, (device->addr << 1) | 1)) < 0) {
		I2C_bit_hw_stop(card);
		return err;
	   }
      
	while (count-- > 0) {
		if ((err = I2C_bit_readbyte(card, count == 0)) < 0) {
			I2C_bit_hw_stop(card);
			return err;
		   }
	
   	*bytes++ = (unsigned char)err;
		res++;
	   }
   
	I2C_bit_stop(card);
   
	return res;
}


static int I2C_bit_probeaddr(struct CardData *card, unsigned short addr)
{
	int err;

	I2C_bit_start(card);
	err = I2C_bit_sendbyte(card, addr << 1);
	I2C_bit_stop(card);

	return err;
}

