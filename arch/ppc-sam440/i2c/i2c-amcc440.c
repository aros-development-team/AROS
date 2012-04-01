/*
 * i2c-amcc440.c
 *
 *  Created on: Feb 1, 2010
 *      Author: misc
 */


#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>

#include <hidd/i2c.h>

#include <asm/amcc440.h>
#include <asm/io.h>

#include "i2c-amcc440.h"

void METHOD(I2C440, Hidd_I2C, PutBits)
{
	/* Put i2c bus into reset state. Mandatory in order to control it manually */
	outb(IIC_XTCNTLSS_SRST, (UBYTE *)IIC0_XTCNTLSS);

	UBYTE val=0;

	val |= (msg->scl ? IIC_DIRECTCNTL_SCLC : 0);
	val |= (msg->sda ? IIC_DIRECTCNTL_SDAC : 0);

	outb(val, (UBYTE *)IIC0_DIRECTCNTL);
}

void METHOD(I2C440, Hidd_I2C, GetBits)
{
	UBYTE val = inb((UBYTE *)IIC0_DIRECTCNTL);

    *msg->sda = (val & IIC_DIRECTCNTL_MSDA) != 0;
    *msg->scl = (val & IIC_DIRECTCNTL_MSCL) != 0;
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
