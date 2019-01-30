/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
 */

#include <inttypes.h>
#include <asm/cpu.h>
#include <utility/tagitem.h>
#include <aros/macros.h>
#include <aros/kernel.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <hardware/bcm2708.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/videocore.h>

asm("   .section .aros.startup      \n"
"       .globl bootstrap            \n"
"       .type bootstrap,%function   \n"
"bootstrap:                         \n"
"       mrs     r0, cpsr_all        \n" /* Check if in hypervisor mode */
"       and     r0, r0, #0x1f       \n"
"       mov     r8, #0x1a           \n"
"       cmp     r0, r8              \n"
"       beq     leave_hyper         \n"
"continue_boot:                     \n"
"       cps     #0x13               \n" /* Should be in SVC (supervisor) mode already, but just incase.. */
"       setend  be                  \n" /* Switch to big endian mode */
"       ldr     sp, tmp_stack_ptr   \n"
"       b       boot                \n"
"leave_hyper:                       \n"
"       ldr     r1, continue_boot   \n"
"       msr     ELR_hyp, r1         \n"
"       mrs     r1, cpsr_all        \n"
"       and     r1, r1, #0x1f       \n"
"       orr     r1, r1, #0x13       \n"
"       msr     SPSR_hyp, r1        \n"
"       eret                        \n"
"       .section .text              \n"

".byte 0                            \n"
".string \"$VER: arosraspi.img v40.45 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup"))) = (void *)(0x1000 - 16);

struct tag;
void boot(uintptr_t dummy, uintptr_t arch, struct tag * atags)
{
    uint32_t tmp, initcr;
    int plus_board = 0;
    void (*entry)(struct TagItem *);

    (void)entry;
    (void)plus_board;

    /*
     * Disable MMU, enable caches and branch prediction. Also enabled unaligned memory
     * access. Exceptions are set to run in big-endian mode and this is the mode
     * in which page tables are written.
     */
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(initcr));
    tmp = initcr;
    tmp &= ~1;                                  /* Disable MMU */
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);    /* I and D caches, branch prediction */
    tmp = (tmp & ~2) | (1 << 22);               /* Unaligned access enable */
    tmp |= (1 << 25);                           /* EE bit for exceptions set - big endian */
                                                /* This bit sets also endianess of page tables */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));
}