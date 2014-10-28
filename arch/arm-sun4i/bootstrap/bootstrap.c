/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#define asmdelay(t) asm volatile("mov r0, %[value]\n1: sub r0, #1\nbne 1b\n"::[value] "i" (t) : "r0", "cc");

void __attribute__((noreturn, section(".bootstrap"))) bootstrap(void) {

    asmdelay(200);
    asmdelay(200);

    while(1);
}

