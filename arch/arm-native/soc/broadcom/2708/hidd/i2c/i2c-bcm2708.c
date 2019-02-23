/*
    Copyright � 2010-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <hidd/i2c.h>

static IPTR __arm_periiobase;
#define ARM_PERIIOBASE __arm_periiobase
#include <hardware/bcm2708.h>
#include <asm/io.h>

#include "i2c-bcm2708.h"

APTR KernelBase __attribute__((used)) = NULL;

void METHOD(I2CBCM2708, Hidd_I2C, PutByte)
{
    while (!(rd32le(BSC0_STATUS) & BSC_STATUS_DONE))
    {
        asm volatile ("mov r2,r2\n");
    }

    wr32le(BSC0_DATALEN, 1);
    wr32le(BSC0_FIFO, msg->data);

    wr32le(BSC0_STATUS, BSC_CLEAR);
    wr32le(BSC0_CONTROL, BSC_WRITE);
}

void METHOD(I2CBCM2708, Hidd_I2C, GetByte)
{
    while (!(rd32le(BSC0_STATUS) & BSC_STATUS_DONE))
    {
        asm volatile ("mov r2,r2\n");
    }

    wr32le(BSC0_DATALEN, 1);
    wr32le(BSC0_STATUS, BSC_CLEAR);
    wr32le(BSC0_CONTROL, BSC_READ);

    *msg->data = rd32le(BSC0_FIFO);
}

void I2CBCM2708_Init(void)
{
    ULONG tmp;
    KernelBase = OpenResource("kernel.resource");
    __arm_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    /* BSC0 is on GPIO 0 & 1 */
    tmp = rd32le(GPFSEL0);
    tmp &= ~0x3f; // Mask out bits 0-5
    tmp |= 0x24;  // Set bits 0-5 to binary '100100'
    wr32le(GPFSEL0, tmp);
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
