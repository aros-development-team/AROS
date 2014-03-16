/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i timer registers
    Lang: english
*/

#ifndef
#define

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_TMR_BASE			0x01c20c00

struct TMR {
	uint32_t IRQ_EN;
	uint32_t IRQ_STA;
	uint32_t 0_CTRL;
	uint32_t 0_INTR_VAL;
	uint32_t 0_CUR_VAL;
	uint32_t 1_CTRL;
	uint32_t 1_INTR_VAL;
	uint32_t 1_CUR_VAL;
	uint32_t 2_CTRL;
	uint32_t 2_INTR_VAL;
	uint32_t 2_CUR_VAL;
	uint32_t 3_CTRL;
	uint32_t 3_INTR_VAL;
	uint32_t 3_CUR_VAL;
	uint32_t 4_CTRL;
	uint32_t 4_INTR_VAL;
	uint32_t 4_CUR_VAL;
	uint32_t 5_CTRL;
	uint32_t 5_INTR_VAL;
	uint32_t 5_CUR_VAL;
	uint32_t AVS_CTRL;
	uint32_t AVS0;
	uint32_t AVS1;
	uint32_t AVS_DIV;
	uint32_t WDT_CTRL;
	uint32_t WDT_MODE;
	uint32_t CNT64_CTRL;
	uint32_t CNT64_LO;
	uint32_t CNT64_HI;
	uint32_t 32KHZ_OSC_CTRL;
	uint32_t RTC_DATE;
	uint32_t RTC_TIME;
	uint32_t ALARM_CNT;
	uint32_t ALARM_WK;
	uint32_t ALARM_EN;
	uint32_t ALARM_IRQ_EN;
	uint32_t ALARM_IRQ_STA;
	uint32_t GP0;
	uint32_t GP1;
	uint32_t GP2;
	uint32_t GP3;
	uint32_t CPU_CFG;
}__attribute__((__packed__));

#endif
