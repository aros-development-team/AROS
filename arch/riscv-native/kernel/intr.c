/*
    Copyright (C) 2013-2015, The AROS Development Team. All rights reserved.
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <hardware/intbits.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"

#define BOOT_STACK_SIZE         (256 << 2)
#define BOOT_TAGS_SIZE          (128 << 3)

#define DREGS(x)
#define DIRQ(x)
#define D(x)

void ictl_enable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
}

void ictl_disable_irq(uint8_t irq, struct KernelBase *KernelBase)
{
}


/*
 * RISC-V Interrupt handler is entered in Supervisor Mode.
 */
asm (
    ".globl __int_handler\n"
    ".type __int_handler,%function \n"
    "__int_handler:               \n"
    "\taddi sp, sp, -30*4\n" // adjust stack
    "\n"
    "\tsw x1, 1*4(sp)\n" // store x1 (ra) - x0 (zero), x2 (sp), x3 (gp) and x4(tp) are ignored.
    "\tsw x5, 3*4(sp)\n" // store temp/func and saved registers
    "\tsw x6, 4*4(sp)\n"
    "\tsw x7, 5*4(sp)\n"
    "\tsw x8, 6*4(sp)\n"
    "\tsw x9, 7*4(sp)\n"
    "\tsw x10, 8*4(sp)\n"
    "\tsw x11, 9*4(sp)\n"
    "\tsw x12, 10*4(sp)\n"
    "\tsw x13, 11*4(sp)\n"
    "\tsw x14, 12*4(sp)\n"
    "\tsw x15, 13*4(sp)\n"
    "\tsw x16, 14*4(sp)\n"
    "\tsw x17, 15*4(sp)\n"
    "\tsw x18, 16*4(sp)\n"
    "\tsw x19, 17*4(sp)\n"
    "\tsw x20, 18*4(sp)\n"
    "\tsw x21, 19*4(sp)\n"
    "\tsw x22, 20*4(sp)\n"
    "\tsw x23, 21*4(sp)\n"
    "\tsw x24, 22*4(sp)\n"
    "\tsw x25, 23*4(sp)\n"
    "\tsw x26, 24*4(sp)\n"
    "\tsw x27, 25*4(sp)\n"
    "\tsw x28, 26*4(sp)\n"
    "\tsw x29, 27*4(sp)\n"
    "\tsw x30, 28*4(sp)\n"
    "\tsw x31, 29*4(sp)\n"
    "\n"
    "\tcsrr t0, mepc\n" // Save the current pc
    "\tsw t0, 30*4(sp)\n"
    "\n"
//    "\tjal ...\n"
    );
