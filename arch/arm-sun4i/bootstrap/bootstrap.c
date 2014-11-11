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

#include <asm/cpu.h>
#include <asm/arm/mmu.h>
#include <asm/arm/cp15.h>

#include <stdio.h>

#define clrbits(addr, clear)           addr = (addr & ~(clear))
#define setbits(addr, set)             addr = (addr | (set))
#define clrsetbits(addr, clear, set)   addr = ((addr & ~(clear)) | (set))

#define PLL_SETTLING_DELAY 100000    /* CPU is running on 24MHz clock when this gets used */

/*
* FIXME: Use timer0, it's so simple
*/
void asmdelay(uint32_t t) {
    asm volatile ("1:              \n" \
    "              subs %0, %1, #1 \n" \
    "              bne 1b            ":"=r" (t):"0"(t));
}

void kprintf(const char *format, ...);

void bootstrapS(void);
asm("           .text                       \n"
"               .globl bootstrapS           \n"
"		        .type bootstrapS,%function  \n"
"                                           \n"
"bootstrapS:    ldr sp, =(0x8000-0x1000)    \n"
"               b bootstrapC                \n"
"                                           \n");

void __attribute__((noreturn)) bootstrapC(void) {

    uint32_t CPU_CFG_CHIP_REV;

    /* Disable MMU, level one data cache and strict alignment fault checking */
    CP15_C1CR_Clear(C1CRF_C|C1CRF_A|C1CRF_M);
    /* Enable level one data cache */
    CP15_C1CR_Set(C1CRF_C);
	/* Set cp10 and cp11 for Privileged and User mode access */
	CP15_C1CACR_All(C1CACRV_CPAP(10)|C1CACRV_CPAP(11));
	/* Enable VFP (and NEON in our case) */
    fmxr(cr8, fmrx(cr8) | 1 << 30);

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
    *     (pcDuino 408MHz, H5TQ2G83EFR-PBC 250K 4 x (256M x 8), https://www.skhynix.com/inc/pdfDownload.jsp?path=/datasheet/pdf/dram/Computing_DDR3_H5TQ2G4%288%293EFR%28Rev1.1%29.pdf)
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

  //PLL5_CFG = 0x91059191;  // 408MHz FIXME: document bits that are set and make it a macro
  //PLL5_CFG = 0x91058091|(17<<8);  // 408MHz DRAM N=17
  //PLL5_CFG = 0x91058091|(18<<8);  // 432MHz DRAM N=18
  //PLL5_CFG = 0x91058091|(19<<8);  // 456MHz DRAM N=19
    PLL5_CFG = 0x91058091|(20<<8);  // 480MHz DRAM N=20

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
    UART0_MCR = 0;
    UART0_IER = 0;
    UART0_FCR = 6;

    /*
    * PLL clocks need time to lock on (or settle on a frequency), set both PLL clocks and then delay for both of them.
    * It would be nice if there was a PLL lock bit to monitor.
    * "Also, once the DLH is set, at least 8 clock cycles of the slowest UART clock should be allowed to pass before transmitting or receiving data."
    */
    asmdelay(PLL_SETTLING_DELAY);

    /*
    * Set CPU to use PLL1
    */
    CPU_AHB_APB0_CFG = (AXI_DIV_1 << 0 | AHB_DIV_2 << 4 | APB0_DIV_1 << 8 | CPU_CLK_SRC_PLL1 << 16);

    /*
    * Get chip revision. Clear the register first and see what pops up again or are read only.
    * Apparently revision A has some bits inverted.
    */
    TIMER_CPU_CFG = 0;
    CPU_CFG_CHIP_REV = ((TIMER_CPU_CFG>>6) & 0b11);

    /*
    * Allwinner user manual says "If the clock source is changed, at most to wait for 8 present running clock cycles"
    */
    asmdelay(1000);

    kprintf("Copyright (c)2014, The AROS Development Team. All rights reserved.\n\n");

    kprintf("Allwinner A10 revision ");
    switch(CPU_CFG_CHIP_REV) {
        case TIMER_CPU_CFG_CHIP_REV_A:
            kprintf("A\n\n");
            break;
        case TIMER_CPU_CFG_CHIP_REV_C1:
            kprintf("C1\n\n");
            break;
        case TIMER_CPU_CFG_CHIP_REV_C2:
            kprintf("C2\n\n");
            break;
        case TIMER_CPU_CFG_CHIP_REV_B:
            kprintf("B\n\n");
            break;
        default:
            kprintf("unknown\n\n");
            break;
    }

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

    uint32_t PLL1_P, PLL1_N, PLL1_K, PLL1_M, CPU_CLK;

    PLL1_M = ((PLL1_CFG >> 0) & 0x3) + 1;
    PLL1_K = ((PLL1_CFG >> 4) & 0x3) + 1;
    PLL1_N = (PLL1_CFG >> 8) & 0x1f;
    PLL1_P = (PLL1_CFG >> 16) & 0x3;
    PLL1_P = (1<<PLL1_P);

    CPU_CLK = ((24*PLL1_N*PLL1_K)/(PLL1_M*PLL1_P));

    kprintf("Bootstrap CPU clock is %uMHz\n", CPU_CLK);

    /*
    * Bits are identical to PLL1
    */
    uint32_t PLL5_P, PLL5_N, PLL5_K, PLL5_M, DRAM_CLK, PLL5_CLK;

    PLL5_M = ((PLL5_CFG >> 0) & 0x3) + 1;
    PLL5_K = ((PLL5_CFG >> 4) & 0x3) + 1;
    PLL5_N = (PLL5_CFG >> 8) & 0x1f;
    PLL5_P = (PLL5_CFG >> 16) & 0x3;
    PLL5_P = (1<<PLL5_P);

    DRAM_CLK = ((24*PLL5_N*PLL5_K)/PLL5_M);
    PLL5_CLK = ((24*PLL5_N*PLL5_K)/PLL5_P);

    kprintf("Bootstrap DDR3 clock is %uMHz (for others PLL5 clock is %uMHz)\n\n", DRAM_CLK, PLL5_CLK);

/* DDR3 setup [start] - minus PLL5 clock for SDRAM */

    /*
    * Enable DDR_CLK_OUT
    */
	setbits(PLL5_CFG, 0x1<<29);

    //MBUS_CLK_CFG = 0x82000001;

    /*
    * Open DRAM gate
    */
	clrbits(AHB_GATE0, 0x1<<14);
	asmdelay(0x1000);
	setbits(AHB_GATE0, 0x1<<14);
	asmdelay(0x1000);

	if (CPU_CFG_CHIP_REV != 0) {
		setbits(DRAM_MCR, 0x1<<12);
		asmdelay(0x100);
		clrbits(DRAM_MCR, 0x1<<12);
	} else {
		clrbits(DRAM_MCR, 0x1<<12);
		asmdelay(0x100);
		setbits(DRAM_MCR, 0x1<<12);
	}

	clrsetbits(DRAM_MCR, 0x3, (0x6<<12) | 0xffc);

    /*
    * DRAM clock off
    */
    clrbits(DRAM_CLK_CFG, 0x1<<15);

    /*
    * DRAM controller needs to be kicked with a magic word
    */
	DRAM_CSEL = SUN4I_DRAM_MAGIC1;

	setbits(DRAM_CCR, 0x1<<28);

    /*
    * Enable DLL0
    */
	clrsetbits(DRAM_DLLCR0, 0x1<<30, 0x1<<31);
	asmdelay(0x100);
	clrbits(DRAM_DLLCR0, 0x3<<30);
	asmdelay(0x1000);
	clrsetbits(DRAM_DLLCR0, 0x1<<31, 0x1<<30);
	asmdelay(0x1000);

    /*
    * Configure DRAM (specific for pcDuino)
    *
    * Type: DDR3
    * Bus data width: 32
    * Chip data width: 8
    * Chip density: 2048 mebibits
    * Ranks: 1
    */
    DRAM_DCR = 0x000030db;

    /*
    * DRAM clock on
    */
    setbits(DRAM_CLK_CFG, 0x1<<15);
	asmdelay(0x10);

	while (DRAM_CCR & (0x1 << 31));

    /*
    * Enable DLL1-DLL2 for 16-bit bus and DLL1-DLL4 for 32-bit bus others not supported and may hang
    */
	clrsetbits(DRAM_DLLCR1, 0x1<<30, 0x1<<31);
	clrsetbits(DRAM_DLLCR2, 0x1<<30, 0x1<<31);
    if(((DRAM_DCR>>6) & 0x3) == 0x3) {
	    clrsetbits(DRAM_DLLCR3, 0x1<<30, 0x1<<31);
	    clrsetbits(DRAM_DLLCR4, 0x1<<30, 0x1<<31);
    }
	asmdelay(0x100);

	clrbits(DRAM_DLLCR1, 0x3<<30);
	clrbits(DRAM_DLLCR2, 0x3<<30);
    if(((DRAM_DCR>>6) & 0x3) == 0x3) {
	    clrbits(DRAM_DLLCR3, 0x3<<30);
	    clrbits(DRAM_DLLCR4, 0x3<<30);
    }
	asmdelay(0x1000);

	clrsetbits(DRAM_DLLCR1, 0x1<<31, 0x1<<30);
	clrsetbits(DRAM_DLLCR2, 0x1<<31, 0x1<<30);
    if(((DRAM_DCR>>6) & 0x3) == 0x3) {
	    clrsetbits(DRAM_DLLCR3, 0x1<<31, 0x1<<30);
	    clrsetbits(DRAM_DLLCR4, 0x1<<31, 0x1<<30);
    }
	asmdelay(0x1000);

    /*
    * Set ODT impedance divide ratio
    */
    DRAM_ZQCR0 = 0x07b00000;

    /*
    * Set IO configuration register
    */
    DRAM_IOCR = 0x00cc0000;

/*
* Compute refresh interval
* tREFI is 7.8us from Hynix datasheet (normal temperature range, 3.9us for extended range)
* -> We need to convert it to our DDR3 clock ticks (nREFI)
* -> 7.8us = 7800ns
* -> 7800ns/(DDR3_clk_period) = 7800nS*DDR3_clk
* -> DDR3_clk is already known so we use that
* -> nREFI = (7800nS*480MHz)/1000 = (7.8uS*480MHz) = 3744 ticks
*
* tRFC is given as clock ticks in Hynix datasheet (nRFC)
* nRFC for 2Gb DDR3-1066 is 86 ticks 
* -> 1/(1066MHz/2) = 0.00187617 or so seconds for the clock period
* -> With 86 ticks this gives tRFC a value of 0.16135 seconds or so (161.35ms)
* -> If we multiply this with the DDR3_clk frequency of the original 533MHz clock we should arrive at the same tick count of 86
* -> (161.35ms*533MHz)/1000 = 86
*
* -> Instead we need to calculate the correct tic count for our DDR3 clock (480MHz for now)
* -> (161.35ms*480MHz)/1000 = 77 ticks = nRFC
* -> Or more simply ((86/533MHz) = (nRFC/480MHz)) or nRFC = ((86/533MHz)*480MHz)
*
* DDR3_numr = Number of posted refreshes 0-8 (0=1) set it to 8 for now
*/
#define DDR3_tREFI  7800
#define DDR3_tRFC   162
#define DDR3_numr   8

    uint32_t temp, DDR3_nREFI, DDR3_nRFC, DDR3_nRFPRD;

    DDR3_nREFI = ((DDR3_tREFI*DRAM_CLK)/1000);
    DDR3_nRFC = ((DDR3_tRFC*DRAM_CLK)/1000);
    DDR3_nRFPRD = ((DDR3_nREFI*(DDR3_numr+1))-200);

    temp = DRAM_DRR;
    kprintf("DRAM_DRR %x\n", temp);

    kprintf("DDR3_nREFI %x\n", DDR3_nREFI);
    kprintf("DDR3_nRFC %x\n", DDR3_nRFC);
    kprintf("DDR3_nRFPRD %x\n", DDR3_nRFPRD);

    DRAM_DRR = ((1<<31) | (DDR3_numr<<24) | ((DDR3_nRFPRD)<<8) | (DDR3_nRFC));

    temp = DRAM_DRR;
    kprintf("DRAM_DRR %x\n", temp);

    /*
    * Computed DRAM_DRR = 0x886f1642, CHECKME:
    */

    /*
    * Set timing parameters
    */
	DRAM_TPR0 = 0x30926692;
	DRAM_TPR1 = 0x00001090;
	DRAM_TPR2 = 0x0001a0c8;

    /*
    * DDR3 and CAS = 6
    */

/*
* Compute CL
*
* Hynix gives CL as a tick count referencing used frequency
*
* DDR3-667  CL = 5  tCK = 3ns (or so) (5*3ns) = 15ns
* DDR3-800  CL = 6  tCK = 2.5ns (6*2.5ns) = 15ns
* DDR3-1066 CL = 7  tCK = 1.875ns (7*1.875ns) = 13.125ns
* DDR3-1333 CL = 9  tCK = 1.5ns (9*1.5ns) = 13.5ns
* DDR3-1600 CL = 11 tCK = 1.25ns (11*1.25ns) = 13.75ns
*
* tCK is from Hynix datasheet (CL7-CL11) and is approximate value, but we can observe that is has a linear relationship to CL
* -> set tCL to 15.0ns which will give us close enough CL values as in Hynix datasheet for our parts speed grade
*
* DDR3-667  (333.5MHz)  CL = 5
* DDR3-800  (400MHz)    CL = 6
* DDR3-1066 (533MHz)    CL = 7
* DDR3-1333 (666.5MHz)  CL = 9
* DDR3-1600 (800MHz)    CL = 12
*
*/

#define tCL 15

    uint32_t nCL;

    nCL = ((tCL*DRAM_CLK)/1000);
    kprintf("nCL = %d\n", nCL);

    /*
    * Controller adds 4 extra cycles(or does it?), adjust the value
    */
    DRAM_MR = (((nCL-4)<<4) | (0x5<<9));

    DRAM_EMR =  0x00000004;
    DRAM_EMR2 = 0x00000000;
    DRAM_EMR3 = 0x00000000;

    /*
    * Set DQS window mode
    */
    clrsetbits(DRAM_CCR, 0x1<<17, 0x1<<14);

    /* reset external DRAM */
    setbits(DRAM_CCR, 0x1<<31);
    while (DRAM_CCR & (0x1<<31));

    clrbits(DRAM_CCR, 0x1<<28);

    /*
    * Trigger the data training and wait it to finish
    */
    setbits(DRAM_CCR, 0x1<<30);
    while (DRAM_CCR & (0x1<<30));

    /*
    * Check the result
    */
    if(DRAM_CSR & (0x1<<20)) {
        kprintf("DDR3 data training failed!\n");
        bootstrapS();
    } else {
        kprintf("DDR3 data training succesful!\n");
    }

    /*
    * Host port access and priority (USB, CPU, GPU etc.)
    */
    DRAM_HPCR0 = 0x0301;
    DRAM_HPCR1 = 0x0301;
    DRAM_HPCR2 = 0x0301;
    DRAM_HPCR3 = 0x0301;
    DRAM_HPCR4 = 0x0301;
    DRAM_HPCR5 = 0x0301;
    DRAM_HPCR6 = 0;
    DRAM_HPCR7 = 0;
    DRAM_HPCR8 = 0;
    DRAM_HPCR9 = 0;
    DRAM_HPCR10 = 0;
    DRAM_HPCR11 = 0;
    DRAM_HPCR12 = 0;
    DRAM_HPCR13 = 0;
    DRAM_HPCR14 = 0;
    DRAM_HPCR15 = 0;
    DRAM_HPCR16 = 0x1031;
    DRAM_HPCR17 = 0x1031;
    DRAM_HPCR18 = 0x0735;
    DRAM_HPCR19 = 0x1035;
    DRAM_HPCR20 = 0x1035;
    DRAM_HPCR21 = 0x0731;
    DRAM_HPCR22 = 0x1031;
    DRAM_HPCR23 = 0x0735;
    DRAM_HPCR24 = 0x1035;
    DRAM_HPCR25 = 0x1031;
    DRAM_HPCR26 = 0x0731;
    DRAM_HPCR27 = 0x1035;
    DRAM_HPCR28 = 0x1031;
    DRAM_HPCR29 = 0x0301;
    DRAM_HPCR30 = 0x0301;
    DRAM_HPCR31 = 0x0731;

/* DDR3 setup [end] */

    uint32_t *a, b, i;

    uint32_t t, x, y, z, w;
 
    x = "AaAa";
    y = "RrRr";
    z = "OoOo";
    w = "SsSs";

    a = 0x40000000;

    for(i=0; i<1024*1024; i++) {
        t = x ^ (x << 11);
        x = y; y = z; z = w;
        w = w ^ (w >> 19) ^ t ^ (t >> 8);
        a[i] = w;
    }


    x = "AaAa";
    y = "RrRr";
    z = "OoOo";
    w = "SsSs";

    a = 0x40000000;

    for(i=0; i<1024*1024; i++) {
        t = x ^ (x << 11);
        x = y; y = z; z = w;
        w = w ^ (w >> 19) ^ t ^ (t >> 8);
        if(w!=a[i]) {
            kprintf("%x = %x (should be %x)\n", &a[1], a[i], w);
        }
    }

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

