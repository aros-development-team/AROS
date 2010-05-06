/*
 * intelG45_regs.h
 *
 *  Created on: Apr 17, 2010
 *      Author: misc
 */

#include <stdint.h>

#ifndef INTELG45_REGS_H_
#define INTELG45_REGS_H_

#define G45_MGCC			0x52	/* Mirror of Dev0 GMCH Graphics Control */

#define G45_MGCC_IVD		0x0002	/* IGP VGA Disable */

#define G45_MGCC_GMS_MASK	0x00f0	/* Graphics Mode Select */
#define G45_MGCC_GMS_1M		0x0010
#define G45_MGCC_GMS_4M		0x0020
#define G45_MGCC_GMS_8M		0x0030
#define G45_MGCC_GMS_16M	0x0040
#define G45_MGCC_GMS_32M	0x0050
#define G45_MGCC_GMS_48M	0x0060
#define G45_MGCC_GMS_64M	0x0070

#define G45_BSM			0x5c
#define G45_MSAC			0x62



#define G45_GPIOA			0x5010
#define G45_GPIOB			0x5014
#define G45_GPIOC			0x5018
#define G45_GPIOD			0x501c
#define G45_GPIOE			0x5020
#define G45_GPIOF			0x5024

#define G45_GPIO_CLOCK_DIR_MASK	0x0001
#define G45_GPIO_CLOCK_DIR_VAL		0x0002
#define G45_GPIO_CLOCK_DATA_MASK	0x0004
#define G45_GPIO_CLOCK_DATA_VAL	0x0008
#define G45_GPIO_CLOCK_DATA_IN		0x0010
#define G45_GPIO_DATA_DIR_MASK		0x0100
#define G45_GPIO_DATA_DIR_VAL		0x0200
#define G45_GPIO_DATA_MASK			0x0400
#define G45_GPIO_DATA_VAL			0x0800
#define G45_GPIO_DATA_IN			0x1000

#define G45_GMBUS			0x5100

#define G45_VGACNTRL		0x71400

#define G45_DPLLA_CTRL		0x6014

#define G45_DPLL_VCO_ENABLE		0x80000000
#define G45_DPLL_DVO_HIGH_SPEED	0x40000000
#define G45_DPLL_VGA_MODE_DISABLE	0x10000000
#define G45_DPLL_MODE_LVDS			0x08000000
#define G45_DPLL_MODE_DAC_SERIAL	0x04000000
#define G45_DPLL_DAC_SERIAL_P2_DIV_5	0x01000000
#define G45_DPLL_DAC_SERIAL_P2_DIV_10	0x00000000
#define G45_DPLL_LVDS_P2_DIV_7		0x01000000
#define G45_DPLL_LVDS_P2_DIV_14	0x00000000
#define G45_DPLL_P1_MASK			0x00ff0000
#define G45_DPLL_P1_SHIFT			16
#define G45_DPLL_PHASE_MASK		0x00001e00
#define G45_DPLL_PHASE_SHIFT		9

#define readl(addr) ( *(volatile uint32_t *) (addr) )
#define readw(addr) ( *(volatile uint16_t *) (addr) )
#define readb(addr) ( *(volatile uint8_t *)  (addr) )

#define writeb(b,addr) ( (*(volatile uint8_t *)  (addr)) = (b) )
#define writew(b,addr) ( (*(volatile uint16_t *) (addr)) = (b) )
#define writel(b,addr) ( (*(volatile uint32_t *) (addr)) = (b) )

#endif /* INTELG45_REGS_H_ */
