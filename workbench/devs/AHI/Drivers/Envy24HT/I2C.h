/*
    Copyright © 2005-2013, Davy Wentzler. All rights reserved.
    $Id$
*/

#ifndef I2C_H
#define I2C_H


struct I2C;
struct CardData;

struct I2C_bit_ops {
	void (*Start)(struct CardData *card);
	void (*Stop)(struct CardData *card);
	void (*SetDir_CLK_SDA)(struct CardData *card, int clock, int data);  /* set line direction (0 = write, 1 = read) */
	void (*Write_CLK_SDA)(struct CardData *card, int clock, int data);
	int (*GetData)(struct CardData *card, int ack);
};

struct I2C {
	unsigned short flags; // some private data
	unsigned short addr;
   
   struct I2C_bit_ops *bit; // with a pointer, the specific card information can be put in a struct and assigned to this attribute
};


struct I2C *AllocI2C(unsigned char addr);
void FreeI2C(struct I2C *device);

int WriteBytesI2C(struct CardData *card, struct I2C *device, unsigned char *bytes, int count);
int ReadBytesI2C(struct CardData *card, struct I2C *device, unsigned char *bytes, int count);
int ProbeAddressI2C(struct CardData *card, unsigned short addr);

#endif /* I2C_H */
