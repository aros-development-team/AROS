/*
 * intelG45_i2c.c
 *
 *  Created on: Apr 25, 2010
 *      Author: misc
 */

#include "intelG45_intern.h"
#include "intelG45_regs.h"

#define DEBUG 0
#include <aros/debug.h>

#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <proto/exec.h>

#include <hidd/i2c.h>

#include <stdint.h>

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

	writel(val, sd->Card.MMIO + G45_GPIOA);
	val = readl(sd->Card.MMIO + G45_GPIOA);
}

void METHOD(INTELI2C, Hidd_I2C, GetBits)
{
	struct g45staticdata *sd = SD(cl);
	uint32_t val;

	/* release SCL and SDA lines */
//	writel(/*G45_GPIO_CLOCK_DIR_MASK |*/ G45_GPIO_DATA_DIR_MASK, sd->Card.MMIO + G45_GPIOA);
//	writel(0, sd->Card.MMIO + G45_GPIOA);

	val = readl(sd->Card.MMIO + G45_GPIOA);

	//D(bug("[GMA_I2C] Get: %08x <- %08x\n", val, sd->Card.MMIO + G45_GPIOA));
	*msg->scl = (val & G45_GPIO_CLOCK_DATA_IN) != 0;
	*msg->sda = (val & G45_GPIO_DATA_IN) != 0;
}

ADD2LIBS((STRPTR)"i2c.hidd", 0, static struct Library *, I2CBase);
