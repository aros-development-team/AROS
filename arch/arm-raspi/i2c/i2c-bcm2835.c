/*
    Copyright © 2010-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>

#include <hidd/i2c.h>

#include <asm/bcm2835.h>
#include <asm/io.h>

#include "i2c-bcm2835.h"

void METHOD(I2CBCM2835, Hidd_I2C, PutByte)
{
    while (!((*(volatile UBYTE *)BSC0_STATUS) & BSC_STATUS_DONE))
    {
        asm volatile ("mov r2,r2\n");
    }

    *(volatile UBYTE *)BSC0_DATALEN = 1;
    *(volatile UBYTE *)BSC0_FIFO = msg->data;

    *(volatile UBYTE *)BSC0_STATUS = BSC_CLEAR;
    *(volatile UBYTE *)BSC0_CONTROL = BSC_WRITE;
}

void METHOD(I2CBCM2835, Hidd_I2C, GetByte)
{
    while (!((*(volatile UBYTE *)BSC0_STATUS) & BSC_STATUS_DONE))
    {
        asm volatile ("mov r2,r2\n");
    }

    *(volatile UBYTE *)BSC0_DATALEN = 1;
    *(volatile UBYTE *)BSC0_STATUS = BSC_CLEAR;
    *(volatile UBYTE *)BSC0_CONTROL = BSC_READ;

    *msg->data = *(volatile UBYTE *)BSC0_FIFO;
}

void I2CBCM2835_Init(void)
{
    /* BSC0 is on GPIO 0 & 1 */
    *(volatile UBYTE *)GPFSEL0 &= ~0x3f; // Mask out bits 0-5
    *(volatile UBYTE *)GPFSEL0 |= 0x24;  // Set bits 0-5 to binary '100100'
}


ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
