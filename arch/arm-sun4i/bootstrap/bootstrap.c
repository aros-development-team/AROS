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
"bootstrapS:    ldr sp, =(0x8000-0x1000)    \n"
"               b bootstrapC                \n"
"                                           \n");

void __attribute__((noreturn)) bootstrapC(void) {

    /*
    * PLL1 output=(24MHz*N*K)/(M*P)
    * PLL1_M = (0)=1
    * PLL1_K = (0)=1
    * PLL1_N = (16)=16
    * PLL1_P = (0)=1
    */
    PLL1_CFG = 0xa1005000;

    /*
    * Set CPU to use PLL1
    */
    CPU_AHB_APB0_CFG = (AXI_DIV_1 << 0 | AHB_DIV_2 << 4 | APB0_DIV_1 << 8 | CPU_CLK_SRC_PLL1 << 16);

    /*
    * Setup APB1 clock and open the gate for UART0 clock
    */
    APB1_CLK_DIV_CFG = (APB1_CLK_SRC_OSC24M << 24 | APB1_FACTOR_N_1 << 16 | APB1_FACTOR_M_1 << 0);
    APB1_GATE = (0x1<<16);

    /*
    * Setup IO pins for UART0
    */
    PIO_CFG2_REG(PB) = (PIO_CFG2_REG(PB) & ~(0b01110111000000000000000000000000)) | 0b00100010000000000000000000000000;

    /*
    * Setup UART0 (115200, 8bits)
    */
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

/*
    Plan of attack:

        - Move register definitions to header files

        - Boot0 header that MKSUNXIBOOT creates can be truncated a bit (or can it?)
        - Boot0 header will contain UARTDEBUG register address (and used GPIO pins same as on other operating systems)
        - Boot0 header should be modifiable from Aros installer (Poseidon Sunxi class in case the destination is NAND)

        - Our bootstrap code should enable DRAM and check it's size, maximum of 2Gb DRAM available on sun4i
        - Boot0 header defines also the used DRAM type and its timing values etc. (DDR2 or DDR3), can it be automated to "safe" defaults?
            - DDR3 memory chips use ODT (on die termination)
            - Allwinner A10 has two(2) such pins, ODT0 and ODT1
        - Unsure if PMU (power management unit) is needed before DRAM can be initialized

        - Bootstrap code should enable NAND (in case the code is started from NAND it might be already setup by the BROM code)
        - Bootstrap code should enable MMC access (in case the code is started from MMC it might be already setup by the BROM code)
        - Bootstrap code will need a read only filesystem (FAT or SFS)

        - Bootstrap code setups MMU
        - Bootstrap code reads Aros module file list and loads and relocates ELF modules to DRAM
        - Modules are loaded from NAND or MMC, depending on boot priority and presense of MMC card

        - Bootstrap code jumps to Aros kernel and passes on information
*/

        uint32_t PLL1_P, PLL1_N, PLL1_K, PLL1_M;

        PLL1_M = ((PLL1_CFG >> 0) & 0x3) + 1;
        PLL1_K = ((PLL1_CFG >> 4) & 0x3) + 1;
        PLL1_N = (PLL1_CFG >> 8) & 0x1f;
        PLL1_P = (PLL1_CFG >> 16) & 0x3;
        PLL1_P = (1<<PLL1_P);

/*
        kprintf("PLL1_M %d\n", PLL1_M);
        kprintf("PLL1_K %d\n", PLL1_K);
        kprintf("PLL1_N %d\n", PLL1_N);
        kprintf("PLL1_P %d\n", PLL1_P);
*/
        kprintf("Bootstrap CPU speed is %uMHz\n", ((24*PLL1_N*PLL1_K)/(PLL1_M*PLL1_P)));

    /*
    * pcDuino uses CARD0 interface in SD card mode (PF io pins) and PH1 as card detect switch input with pull up resistor
    * For generic bootstrap we will need information stored for used DEBUGUART and SD-card interface
    */
    PIO_CFG0_REG(PH) = (PIO_CFG2_REG(PB) & ~(0b00000000000000000000000001110000)) | 0b00000000000000000000000000000000;

    BOOL cardinserted = FALSE;

    if(!(PIO_DATA_REG(PH) & 0b10)) {
        kprintf("SD card presence detected\n");
        cardinserted = TRUE;
    };

    while(1){
        if((PIO_DATA_REG(PH) & 0b10) && (cardinserted)) {
            kprintf("SD card removed\n");
            cardinserted = FALSE;
        };

        if(!(PIO_DATA_REG(PH) & 0b10) && !(cardinserted)) {
            kprintf("SD card inserted\n");
            cardinserted = TRUE;
        };
    };
}

