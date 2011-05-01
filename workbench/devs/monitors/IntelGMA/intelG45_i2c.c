/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <hidd/i2c.h>

#include <stdint.h>

 
#include "intelG45_intern.h"
#include "intelG45_regs.h"

void METHOD(INTELI2C, Hidd_I2C, PutBits)
{
	struct g45staticdata *sd = SD(cl);
	uint32_t val = 0;

#if 0
	/* SCL as output */
	val = G45_GPIO_CLOCK_DIR_MASK | G45_GPIO_CLOCK_DIR_VAL;
	/* update SCL value */
	val |= G45_GPIO_CLOCK_DATA_MASK;
	/* SDA as output */
	val |= G45_GPIO_DATA_DIR_MASK | G45_GPIO_DATA_DIR_VAL;
	/* update SDA value */
	val |= G45_GPIO_DATA_MASK;

	/* set SCL */
	if (msg->scl)
		val |= G45_GPIO_CLOCK_DATA_VAL;
	if (msg->sda)
		val |= G45_GPIO_DATA_VAL;
#endif

	if (msg->scl)
		val |= G45_GPIO_CLOCK_DIR_MASK;
	else
		val |= G45_GPIO_CLOCK_DIR_MASK | G45_GPIO_CLOCK_DIR_VAL | G45_GPIO_CLOCK_DATA_MASK;

	if (msg->sda)
		val |= G45_GPIO_DATA_DIR_MASK;
	else
		val |= G45_GPIO_DATA_DIR_MASK | G45_GPIO_DATA_DIR_VAL | G45_GPIO_DATA_MASK;

	//D(bug("[GMA_I2C] Put: %08x -> %08x\n", val, sd->Card.MMIO + G45_GPIOA));

	writel(val, sd->Card.MMIO + sd->DDCPort);
	val = readl(sd->Card.MMIO + sd->DDCPort);
}

void METHOD(INTELI2C, Hidd_I2C, GetBits)
{
	struct g45staticdata *sd = SD(cl);
	uint32_t val;

	/* release SCL and SDA lines */
//	writel(/*G45_GPIO_CLOCK_DIR_MASK |*/ G45_GPIO_DATA_DIR_MASK, sd->Card.MMIO + G45_GPIOA);
//	writel(0, sd->Card.MMIO + G45_GPIOA);

	val = readl(sd->Card.MMIO + sd->DDCPort);

	//D(bug("[GMA_I2C] Get: %08x <- %08x\n", val, sd->Card.MMIO + G45_GPIOA));
	*msg->scl = (val & G45_GPIO_CLOCK_DATA_IN) != 0;
	*msg->sda = (val & G45_GPIO_DATA_IN) != 0;
}


static const struct OOP_MethodDescr INTELI2C_Hidd_I2C_descr[] =
{
    {(OOP_MethodFunc)INTELI2C__Hidd_I2C__PutBits, moHidd_I2C_PutBits},
    {(OOP_MethodFunc)INTELI2C__Hidd_I2C__GetBits, moHidd_I2C_GetBits},
    {NULL, 0}
};
#define NUM_INTELI2C_Hidd_I2C_METHODS 2

const struct OOP_InterfaceDescr INTELI2C_ifdescr[] =
{
    {INTELI2C_Hidd_I2C_descr, IID_Hidd_I2C, NUM_INTELI2C_Hidd_I2C_METHODS},
    {NULL, NULL}
};

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
