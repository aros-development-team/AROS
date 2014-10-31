/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hardware/sun4i/ccm.h>
#include <hardware/sun4i/pio.h>
#include <hardware/sun4i/uart.h>

#include <stdio.h>

#define CPU_AHB_APB0_CFG (*(volatile uint32_t *)0x01c20054)
#define PLL1_CFG         (*(volatile uint32_t *)0x01c20000)
#define APB1_CLK_DIV_CFG (*(volatile uint32_t *)0x01c20058)
#define APB1_GATE        (*(volatile uint32_t *)0x01c2006C)

#define UART0_RBR (*(volatile uint32_t *)0x01c28000)
#define UART0_THR (*(volatile uint32_t *)0x01c28000)
#define UART0_DLL (*(volatile uint32_t *)0x01c28000)
#define UART0_DLH (*(volatile uint32_t *)0x01c28004)
#define UART0_IER (*(volatile uint32_t *)0x01c28004)
#define UART0_IIR (*(volatile uint32_t *)0x01c28008)
#define UART0_FCR (*(volatile uint32_t *)0x01c28008)
#define UART0_LCR (*(volatile uint32_t *)0x01c2800c)
#define UART0_MCR (*(volatile uint32_t *)0x01c28010)
#define UART0_LSR (*(volatile uint32_t *)0x01c28014)
#define UART0_MSR (*(volatile uint32_t *)0x01c28018)
#define UART0_SCH (*(volatile uint32_t *)0x01c2801c)
#define UART0_USR (*(volatile uint32_t *)0x01c2807c)
#define UART0_TFL (*(volatile uint32_t *)0x01c28080)
#define UART0_RFL (*(volatile uint32_t *)0x01c28084)
#define UART0_HLT (*(volatile uint32_t *)0x01c280a4)

#define APB1_CLK_SRC_OSC24M		0
#define APB1_FACTOR_M_1			0
#define APB1_FACTOR_N_1			0

#define CPU_CLK_SRC_OSC24M		1
#define CPU_CLK_SRC_PLL1		2

#define AXI_DIV_1			0
#define AXI_DIV_2			1
#define AXI_DIV_3			2
#define AXI_DIV_4			3

#define AHB_DIV_1			0
#define AHB_DIV_2			1
#define AHB_DIV_4			2
#define AHB_DIV_8			3

#define APB0_DIV_1			0
#define APB0_DIV_2			1
#define APB0_DIV_4			2
#define APB0_DIV_8			3

#define asmdelay(t) asm volatile("mov r0, %[value]\n1: sub r0, #1\nbne 1b\n"::[value] "i" (t) : "r0", "cc");

void kprintf(const char *format, ...);

asm("           .text                       \n"
"               .globl bootstrapS           \n"
"		        .type bootstrapS,%function  \n"
"                                           \n"
"bootstrapS:    ldr sp, =0x8000             \n"
"               b bootstrapC                \n"
"                                           \n");

void __attribute__((noreturn)) bootstrapC(void) {
    PLL1_CFG = 0xa1005000;

    CPU_AHB_APB0_CFG = (AXI_DIV_1 << 0 | AHB_DIV_2 << 4 | APB0_DIV_1 << 8 | CPU_CLK_SRC_PLL1 << 16);

    APB1_CLK_DIV_CFG = (APB1_CLK_SRC_OSC24M << 24 | APB1_FACTOR_N_1 << 16 | APB1_FACTOR_M_1 << 0);

    APB1_GATE = (0x1<<16);

    PIO_CFG2_REG(PB) = (PIO_CFG2_REG(PB) & ~(0b01110111000000000000000000000000)) | 0b00100010000000000000000000000000;

    while(UART0_USR & 1);
    UART0_LCR = (UART0_LCR | (1<<7));
    UART0_DLL = (13>>0) & 0xff;
    UART0_DLH = (13>>8) & 0xff;
    UART0_LCR = (UART0_LCR & ~(1<<7));
    UART0_HLT = 0;
    UART0_LCR = 3;
    UART0_FCR = 6;

    /*
    * Our CPU clock has not yeat stabilized, don't do any time critical things until it settles.
    */
    asmdelay(200);

    kprintf("Copyright (c)2014, The AROS Development Team. All rights reserved.\n\n");
    kprintf("0xabad1dea = %x\n", 0xabad1dea);
    kprintf("1234567890 = %d\n", 1234567890);

    while(1);
}
