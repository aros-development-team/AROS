/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: sun4i uart module (NS16550)
    Lang: english
*/

#ifndef HARDWARE_SUN4I_UART_H
#define HARDWARE_SUN4I_UART_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef _INTTYPES_H
#include <inttypes.h>
#endif

#define SUN4I_UART0_BASE    0x01c28000
#define SUN4I_UART1_BASE    0x01c28400
#define SUN4I_UART2_BASE    0x01c28800
#define SUN4I_UART3_BASE    0x01c28c00
#define SUN4I_UART4_BASE    0x01c29000
#define SUN4I_UART5_BASE    0x01c29400
#define SUN4I_UART6_BASE    0x01c29800
#define SUN4I_UART7_BASE    0x01c29c00

#define UART0_RBR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0000))
#define UART0_THR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0000))
#define UART0_DLL           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0000))
#define UART0_DLH           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0004))
#define UART0_IER           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0004))
#define UART0_IIR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0008))
#define UART0_FCR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0008))
#define UART0_LCR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x000c))
#define UART0_MCR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0010))
#define UART0_LSR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0014))
#define UART0_MSR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0018))
#define UART0_SCH           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x001c))
#define UART0_USR           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x007c))
#define UART0_TFL           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0080))
#define UART0_RFL           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x0084))
#define UART0_HLT           (*(volatile uint32_t *)(SUN4I_UART0_BASE + 0x00a4))

#endif /* HARDWARE_SUN4I_UART_H */
