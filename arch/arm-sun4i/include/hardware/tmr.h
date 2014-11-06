/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i timer registers
    Lang: english
*/

#ifndef HARDWARE_SUN4I_TIMER_H
#define HARDWARE_SUN4I_TIMER_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_TIMER_BASE        0x01c20c00

#define TIMER_IRQ_EN            (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0000))
#define TIMER_IRQ_STA           (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0004))
#define TIMER0_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0010))
#define TIMER0_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0014))
#define TIMER0_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0018))
#define TIMER1_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0020))
#define TIMER1_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0024))
#define TIMER1_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0028))
#define TIMER2_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0030))
#define TIMER2_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0034))
#define TIMER2_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0038))
#define TIMER3_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0040))
#define TIMER3_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0044))
#define TIMER3_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0048))
#define TIMER4_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0050))
#define TIMER4_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0054))
#define TIMER4_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0058))
#define TIMER5_CTRL             (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0060))
#define TIMER5_INTR_VAL         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0064))
#define TIMER5_CUR_VAL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0068))
#define TIMER_AVS_CTRL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0080))
#define TIMER_AVS0              (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0084))
#define TIMER_AVS1              (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0088))
#define TIMER_AVS_DIV           (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x008c))
#define TIMER_WDT_CTRL          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0090))
#define TIMER_WDT_MODE          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0094))
#define TIMER_CNT64_CTRL        (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x00a0))
#define TIMER_CNT64_LO          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x00a4))
#define TIMER_CNT64_HI          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x00a8))
#define TIMER_32KHZ_OSC_CTRL    (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0100))
#define TIMER_RTC_DATE          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0104))
#define TIMER_RTC_TIME          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0108))
#define TIMER_ALARM_CNT         (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x010c))
#define TIMER_ALARM_WK          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0110))
#define TIMER_ALARM_EN          (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0114))
#define TIMER_ALARM_IRQ_EN      (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0118))
#define TIMER_ALARM_IRQ_STA     (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x011c))
#define TIMER_GP0               (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0120))
#define TIMER_GP1               (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0124))
#define TIMER_GP2               (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0128))
#define TIMER_GP3               (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x012c))
#define TIMER_CPU_CFG           (*(volatile uint32_t *)(SUN4I_TIMER_BASE + 0x0140))

#endif
