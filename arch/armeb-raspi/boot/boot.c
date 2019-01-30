/*
    Copyright � 2013-2015, The AROS Development Team. All rights reserved.
    $Id$

    Desc: boot.c
    Lang: english
 */

#include <exec/types.h>
#include <aros/macros.h>
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

#include "boot.h"
#include "serialdebug.h"
#include "mmu.h"

#undef ARM_PERIIOBASE
#define ARM_PERIIOBASE (__arm_periiobase)

uint32_t __arm_periiobase;

extern void mem_init(void);
extern unsigned int uartclock;
extern unsigned int uartdivint;
extern unsigned int uartdivfrac;
extern unsigned int uartbaud;

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
"       setend  be                  \n"
"       ldr     r1, =continue_boot  \n"
"       msr     ELR_hyp, r1         \n"
"       mrs     r1, cpsr_all        \n"
"       and     r1, r1, #0x1f       \n"
"       orr     r1, r1, #0x13       \n"
"       msr     SPSR_hyp, r1        \n"
"       eret                        \n"
"       .section .text              \n"
".byte 0                            \n"
".string \"$VER: arosraspi.img v40.46 (" __DATE__ ")\"\n"
".byte 0                            \n"
"\n\t\n\t"
);

// The bootstrap tmp stack is re-used by the reset handler so we store it at this fixed location
static __used void * tmp_stack_ptr __attribute__((used, section(".aros.startup"))) = (void *)(0x1000 - 16);
static struct TagItem *boottag;
//static unsigned long *mem_upper;

struct tag;

static const char bootstrapName[] = "Bootstrap/ARM BCM2708";

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

    mmu_init();

    /*
        Check processor type - armv6 is old raspberry pi with SOC IO base at 0x20000000.
        armv7 will be raspberry pi 2 with SOC IO base at 0x3f000000
     */
    asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (tmp));

    tmp = (tmp >> 4) & 0xfff;

    /* tmp == 7 means armv6 architecture. */
    if (tmp == 0xc07) /* armv7, also RaspberryPi 2 */
    {
        __arm_periiobase = BCM2836_PERIPHYSBASE;
        plus_board = 1;

        /* prepare map for core boot vector(s) */
        mmu_map_section(0x40000000, 0x40000000, 0x100000, 0, 0, 3, 0);
    }
    else
    {
        __arm_periiobase = BCM2835_PERIPHYSBASE;
        /* Need to detect the plus board here in order to control LEDs properly */
    }

    /* Prepare map for MMIO registers */
    mmu_map_section(__arm_periiobase, __arm_periiobase, ARM_PERIIOSIZE, 1, 0, 3, 0);

    mem_init();

    boottag = tmp_stack_ptr - BOOT_STACK_SIZE - BOOT_TAGS_SIZE;

    /* first of all, store the arch for the kernel to use .. */
    boottag->ti_Tag = KRN_Platform;
    boottag->ti_Data = (IPTR)arch;
    boottag++;

    /* Init LED */
    {
        if (plus_board)
        {
            /*
             * Either B+ or rpi2 board. Uses two leds (power and activity) on GPIOs
             * 47 and 35. Enable both leds as output and turn both of them off.
             *
             * The power led will be brought back up once AROS boots.
             */

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL4);
            tmp &= ~(7 << 21); // GPIO 47 = 001 - output
            tmp |= (1 << 21);
            *(volatile unsigned int *)GPFSEL4 = AROS_LONG2LE(tmp);

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL3);
            tmp &= ~(7 << 15); // GPIO 35 = 001 - output
            tmp |= (1 << 15);
            *(volatile unsigned int *)GPFSEL3 = AROS_LONG2LE(tmp);

            /* LEDS off */
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE((1 << (47-32)));
            *(volatile unsigned int *)GPCLR1 = AROS_LONG2LE((1 << (35-32)));
        }
        else
        {
            /*
             * Classic rpi board has only one controlable LED - activity on GPIO 16. Turn it
             * off now, kernel.resource will bring it back later.
             */

            tmp = AROS_LE2LONG(*(volatile unsigned int *)GPFSEL1);
            tmp &= ~(7 << 18); // GPIO 16 = 001 - output
            tmp |= (1 << 18);
            *(volatile unsigned int *)GPFSEL1 = AROS_LONG2LE(tmp);

            *(volatile unsigned int *)GPSET0 = AROS_LONG2LE((1 << 16));
        }
    }

    serInit();

    boottag->ti_Tag = KRN_BootLoader;
    boottag->ti_Data = (IPTR)bootstrapName;
    boottag++;

#if 0
    if (vcfb_init())
    {
        boottag->ti_Tag = KRN_FuncPutC;
        boottag->ti_Data = (IPTR)fb_Putc;
        boottag++;
    }
#endif

    kprintf("[BOOT] AROS %s\n", bootstrapName);
while(1);
}