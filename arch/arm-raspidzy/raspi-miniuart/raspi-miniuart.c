/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <stdint.h>

#include <raspi/raspi.h>

void ser_InitMINIUART(void) {

    /*
        Hackish access to miniuart as we REALLY don't know any of the system clocks
    */
    volatile struct raspiaux *raspiaux= RASPIAUX_PHYSBASE;

    raspiaux->enables = 1;
    raspiaux->mu_ier_reg = 0;
    raspiaux->mu_cntl_reg = 0;
    raspiaux->mu_lcr_reg = 3;
    raspiaux->mu_mcr_reg = 0;
    raspiaux->mu_ier_reg = 0;
    raspiaux->mu_iir_reg = 0xc6;
    raspiaux->mu_baud_reg = 270;

    /*
        Circuitry controlling the gpio pullup/down behaviour is running at least 150 times slower than the cpu.
        We need to wait for the circuitry to have at least one clock cycle to fetch the gppud register content
        and then another for it to fetch the gppudclk content and once more when clearing gppudclk to be safe
        as we have no way of knowing who and when will call it next time.
    */

    volatile struct raspigpio *raspigpio= RASPIGPIO_PHYSBASE;

    raspigpio->gpfsel1 &= ~(GPIOfsel(14, GPIOmask) | GPIOfsel(15, GPIOmask));
    raspigpio->gpfsel1 |= (GPIOfsel(14, GPIOalt5) | GPIOfsel(15, GPIOalt5));

    ULONG temp;

    raspigpio->gppud = GPIOpullnone;
        for(temp=0;temp<150;temp++);

    raspigpio->gppudclk0 = (1<<14)|(1<<15);
        for(temp=0;temp<150;temp++);

    raspigpio->gppudclk0 = 0;
        for(temp=0;temp<150;temp++);

    raspiaux->mu_cntl_reg = 3;

}

void ser_PutCMINIUART(uint32_t c) {

    volatile struct raspiaux *raspiaux= RASPIAUX_PHYSBASE;

    while(!(raspiaux->mu_lsr_reg & 0x20));
    raspiaux->mu_io_reg = c;

}

