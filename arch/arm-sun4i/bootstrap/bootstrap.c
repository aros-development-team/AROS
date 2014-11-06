/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hardware/sun4i/ccm.h>
#include <hardware/sun4i/pio.h>
#include <hardware/sun4i/uart.h>
#include <hardware/sun4i/dram.h>
#include <hardware/sun4i/tmr.h>

#include <stdio.h>

#define clrbits(addr, clear)           addr = (addr & ~(clear))
#define setbits(addr, set)             addr = (addr | (set))
#define clrsetbits(addr, clear, set)   addr = ((addr & ~(clear)) | (set))

#define PLL_SETTLING_DELAY 10000    /* CPU is running on 24MHz clock */

void asmdelay(uint32_t t) {
    asm volatile ("1:              \n" \
    "              subs %0, %1, #1 \n" \
    "              bne 1b            ":"=r" (t):"0"(t));
}

void kprintf(const char *format, ...);

int dramc_init(void);

asm("           .text                       \n"
"               .globl bootstrapS           \n"
"		        .type bootstrapS,%function  \n"
"                                           \n"
"bootstrapS:    ldr sp, =(0x8000-0x1000)    \n"
"               b bootstrapC                \n"
"                                           \n");

void __attribute__((noreturn)) bootstrapC(void) {

    /*
    * PLL1 output for CPU = (24MHz*N*K)/(M*P)
    * PLL1_M = (0)=1
    * PLL1_K = (0)=1
    * PLL1_N = (16)=16
    * PLL1_P = (0)=1 (1/2/4/8 or the exponent of 2 in other words)
    */
    PLL1_CFG = 0xa1005000;

    /*
    * PLL5 output for DDR = (24MHz*N*K)/M
    * PLL5 output for others = (24MHz*N*K)/P
    *
    * pcDuino has four(4) Hynix DDR3 chips, U2(d0-d7), U3(d8-d15), U10(d16-d23) and U11(d24-d31) in one rank
    *     (pcDuino 408MHz, H5TQ2G83EFR 4 x (256M x 8), https://www.skhynix.com/inc/pdfDownload.jsp?path=/datasheet/pdf/dram/Computing_DDR3_H5TQ2G4%288%293EFR%28Rev1.1%29.pdf)
    *
    * PLL5_M = (0)=1
    * PLL5_K = (0)=1
    * PLL5_N = (17)=17 (408MHz/24MHz)
    * PLL5_P = (0)=1 (1/2/4/8 or the exponent of 2 in other words)
    *
    * PLL5 bypass disabled, PLL5 enabled and DDR clk out disabled
    *
    * Writes over some unknown bits, bad bad... but seems to work :)
    *
    */
  //PLL5_CFG = 0x80001100;  // 408MHz DRAM training passes on first iteration (0x40000000 contains 0x11111111)
  //PLL5_CFG = 0x80001200;  // 432MHz DRAM training goes for a second iteration (0x40000000 contains 0x22222222)
  //PLL5_CFG = 0x80001300;  // 456MHz DRAM training goes for a second iteration (0x40000000 contains 0x22222222)
    PLL5_CFG = 0x80001400;  // 480MHz DRAM training passes on first iteration (0x40000000 contains 0x11111111)
                            // These may not mean what I think they mean...

    /*
    * PLL clocks need time to lock on (or settle on a frequency), set both PLL clocks and then delay for both of them.
    * It would be nice if there was a PLL lock bit to monitor.
    */
    asmdelay(PLL_SETTLING_DELAY);

    /*
    * Set CPU to use PLL1
    */
    CPU_AHB_APB0_CFG = (AXI_DIV_1 << 0 | AHB_DIV_2 << 4 | APB0_DIV_1 << 8 | CPU_CLK_SRC_PLL1 << 16);

    /*
    * Setup APB1 clock and open the gate for UART0 clock, clear others
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

    kprintf("PLL1_M %d\n", PLL1_M);
    kprintf("PLL1_K %d\n", PLL1_K);
    kprintf("PLL1_N %d\n", PLL1_N);
    kprintf("PLL1_P %d\n", PLL1_P);

    kprintf("Bootstrap CPU clock is %uMHz\n", ((24*PLL1_N*PLL1_K)/(PLL1_M*PLL1_P)));

    /*
    * Bits are identical to PLL1
    */
    uint32_t PLL5_P, PLL5_N, PLL5_K, PLL5_M;

    PLL5_M = ((PLL5_CFG >> 0) & 0x3) + 1;
    PLL5_K = ((PLL5_CFG >> 4) & 0x3) + 1;
    PLL5_N = (PLL5_CFG >> 8) & 0x1f;
    PLL5_P = (PLL5_CFG >> 16) & 0x3;
    PLL5_P = (1<<PLL5_P);

    kprintf("Bootstrap DDR3 clock is %uMHz (for others PLL5 clock is %uMHz)\n", ((24*PLL5_N*PLL5_K)/PLL5_M), ((24*PLL5_N*PLL5_K)/PLL5_P));

    kprintf("PLL5_M %d\n", PLL5_M);
    kprintf("PLL5_K %d\n", PLL5_K);
    kprintf("PLL5_N %d\n", PLL5_N);
    kprintf("PLL5_P %d\n", PLL5_P);

    /*
    * Enable DDR_CLK_OUT
    */
	asmdelay(0x100000);
	setbits(PLL5_CFG, 0x1<<29);



    /*
    * Check if we can write to DDR3 memory, if we can then it's all ours, every single bit and byte!
    */
    uint32_t *this_one_is_in_ddr3_memory;
    this_one_is_in_ddr3_memory = 0x40000000;

    kprintf("this_one_is_in_ddr3_memory[0] = %x\n", *this_one_is_in_ddr3_memory);
    *this_one_is_in_ddr3_memory = 0xabad1dea;
    kprintf("this_one_is_in_ddr3_memory[0] = %x\n", *this_one_is_in_ddr3_memory);

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

