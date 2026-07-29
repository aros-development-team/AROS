/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM27xx SoC-common platform support for AArch64, shared between the
    legacy interrupt controller Pis (bcm2708/2836/2837) and the GIC based
    Pi 4 (bcm2711). Implemented in platform_bcm27xx.c.
*/

#ifndef KERNEL_BCM27XX_H
#define KERNEL_BCM27XX_H

#include <exec/types.h>

#include <inttypes.h>

void bcm27xx_init(APTR, APTR);
void bcm27xx_init_cpu(APTR, APTR);
void bcm27xx_fiq_process(void);
void bcm27xx_send_ipi(uint32_t, uint32_t, uint32_t);
unsigned int bcm27xx_get_time(void);
void bcm27xx_toggle_led(int, int);
void bcm27xx_ser_putc(uint8_t);
int bcm27xx_ser_getc(void);

#endif /* KERNEL_BCM27XX_H */
