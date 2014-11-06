/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i clock control module
    Lang: english
*/

#ifndef HARDWARE_SUN4I_CCM_H
#define HARDWARE_SUN4I_CCM_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_CCM_BASE      0x01c20000

#define PLL1_CFG            (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0000))
#define PLL5_CFG            (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0020))
#define CPU_AHB_APB0_CFG    (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0054))
#define APB1_CLK_DIV_CFG    (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0058))
#define AHB_GATE0           (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0060))
#define APB1_GATE           (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x006C))
#define GPS_CLK_CFG         (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x00d0))
#define DRAM_CLK_CFG        (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x0100))
#define MBUS_CLK_CFG        (*(volatile uint32_t *)(SUN4I_CCM_BASE + 0x015c))

#define APB1_CLK_SRC_OSC24M 0
#define APB1_FACTOR_M_1     0
#define APB1_FACTOR_N_1     0

#define CPU_CLK_SRC_OSC24M  1
#define CPU_CLK_SRC_PLL1    2

#define AXI_DIV_1           0
#define AXI_DIV_2           1
#define AXI_DIV_3           2
#define AXI_DIV_4           3

#define AHB_DIV_1           0
#define AHB_DIV_2           1
#define AHB_DIV_4           2
#define AHB_DIV_8           3

#define APB0_DIV_1          0
#define APB0_DIV_2          1
#define APB0_DIV_4          2
#define APB0_DIV_8          3

#endif
