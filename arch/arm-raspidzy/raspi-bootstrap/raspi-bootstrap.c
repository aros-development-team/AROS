/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <exec/types.h>
#include <stdint.h>

#include <raspi/raspi.h>

asm("	.section .aros.startup              \n"
"		.globl bootstrap                    \n"
"		.type bootstrap,%function           \n"
"bootstrap:				                    \n"
"               mov     sp, #0x8000         \n"
"		b       coldboot                    \n"
);

void bprintf(const char *format, ...);
extern void ser_InitMINIUART(void);
extern void ser_PutCMINIUART(uint32_t c);
extern void con_InitRASPICON(void);

void coldboot(void);
void warmboot(void);
void init_bootheap(void);

/*
    We're alive...

    As of now we don't know much about the platform.
        -UBoot has stored it's ATags at 0x100 or somewhere around there and called the bootstrap
        -Bootstrap code has set us up a boot time stack at 0x8000
        -We are running on a flat physical memory layout
        -We don't know any of the system clocks
        -We don't have any memory expcept the bootstack and bootheap
        -We dont have any interrupt vectors and interrupts are disabled
*/

void coldboot(void) {

    init_bootheap();

    /*
        Aqcuire system clocks, needed for peripheral usage
        FIXME: Hunt for the Amba registers, they are there somewhere... or use the mailbox if nothing else
    */

    ser_InitMINIUART();
    con_InitRASPICON();

    /*
        Aqcuire system memory layout
    */

    warmboot();
}

void warmboot(void) {

    /*
        We have our own "kprintf" while kernel is dead
    */
    bprintf("Welcome to warmboot number %d\n", 69);
    while(1);
}

void init_bootheap(void) {
}
