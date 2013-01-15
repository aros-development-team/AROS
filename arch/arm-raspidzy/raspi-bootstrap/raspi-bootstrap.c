/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <raspi/raspi.h>

asm("	.section .aros.startup              \n"
"		.globl bootstrap                    \n"
"		.type bootstrap,%function           \n"
"bootstrap:				                    \n"
"               mov     sp, #0x8000         \n"
"		b       coldboot                    \n"
);

extern void con_InitRASPI(void);

/*
    We're alive...

    As of now we don't know much about the platform.
        -UBoot has stored it's ATags at 0x100 or somewhere around there and called the bootstrap
        -Bootstrap code has set us up a boot time stack at 0x8000
        -We are running on a flat physical memory layout
        -We don't know any of the system clocks
        -We don't have any memory expcept the bootstack
        -We dont have any interrupt vectors and interrupts are disabled
*/

void coldboot(void) {

    /*
        Hackish access to miniuart as we REALLY don't know any of the system clocks
    */
    volatile struct raspiaux *raspiaux= RASPIAUX_PHYSBASE;

    raspiaux->enables = 1;
    raspiaux->mu_ier_reg = 0;
    raspiaux->mu_iir_reg = 0xc6;
    raspiaux->mu_lcr_reg = 0;
    raspiaux->mu_mcr_reg = 0;
    raspiaux->mu_cntl_reg = 0;
    raspiaux->mu_baud_reg = 270;

    volatile struct raspigpio *raspigpio= RASPIGPIO_PHYSBASE;

    /*
        Aqcuire system clocks, needed for peripheral usage
    */

    /*
        Aqcuire system memory layout
    */

    con_InitRASPI();
    while(1);
}

void warmboot() {
    while(1);
}
