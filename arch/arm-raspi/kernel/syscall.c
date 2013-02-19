/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>

#include <aros/arm/cpucontext.h>

#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"
#include "kernel_syscall.h"

extern char * __text_start;
extern char * __text_end;

#ifndef _CUSTOM
#define _CUSTOM NULL
#endif

#define DREGS(x)
#define D(x)

/*
    __vectorhand_swi:

    this code currently assumes the caller is in user mode (and will break from any other mode)

    r0 = passed to c handler, r1/r2 = temp
*/
asm (
    ".globl __vectorhand_swi                   \n"
    ".type __vectorhand_swi,%function          \n"
    "__vectorhand_swi:                         \n"
    VECTCOMMON_START
    "           add     r1, r0, #13*4          \n" // store sp^ in ctx_sp
    "           stm     r1, {sp}^              \n" 
    "           add     r1, r0, #14*4          \n" // store lr^ in ctx_lr
    "           stm     r1, {lr}^              \n"
    "           mov     fp, #0                 \n" // clear fp(??)

    "           bl      handle_syscall         \n"
    VECTCOMMON_END
);

void handle_syscall(void *regs)
{
    register unsigned int addr;
    register unsigned int swi_no;

    /* We determine the SWI number by reading in "tasks"
       program counter, subtract the instruction from it and
       obtain the value from there.  we also use this to check if
       we have been called from outwith the kernel's code (illegal!)
     */

    addr = ((uint32_t *)regs)[15];
    addr -= 4;
    swi_no = *((unsigned int *)addr) & 0x00ffffff;

    D(bug("[KRN] ## SWI %d @ 0x%p\n", swi_no, addr));

    if (((char*)addr < &__text_start) || ((char*)addr >= &__text_end))
    {
        D(bug("[KRN] ## SWI : ILLEGAL ACCESS!\n"));
        return;
    }
    if (swi_no <= 0x0a || swi_no == 0x100)
    {
        DREGS(cpu_DumpRegs(regs));
    
        switch (swi_no)
        {
            case SC_CLI:
            {
                D(bug("[KRN] ## CLI...\n"));
                ((uint32_t *)regs)[16] |= 0x80;
                break;
            }

            case SC_STI:
            {
                D(bug("[KRN] ## STI...\n"));
                ((uint32_t *)regs)[16] &= ~0x80;
                break;
            }

            case SC_SUPERSTATE:
            {
                D(bug("[KRN] ## SUPERSTATE... (0x%p ->", ((uint32_t *)regs)[16]));
                ((uint32_t *)regs)[16] &= ~CPUMODE_MASK;
                ((uint32_t *)regs)[16] |= CPUMODE_SUPERVISOR;
                D(bug(" 0x%p)\n", ((uint32_t *)regs)[16]));
                break;
            }

            case SC_ISSUPERSTATE:
            {
                D(bug("[KRN] ## ISSUPERSTATE... "));
                ((uint32_t *)regs)[0] = !(((((uint32_t *)regs)[16] & CPUMODE_MASK) == CPUMODE_USER) || ((((uint32_t *)regs)[16] & CPUMODE_MASK) == CPUMODE_SYSTEM));
                D(bug("%d\n", ((uint32_t *)regs)[0]));
                break;
            }

            case SC_REBOOT:
            {
                D(bug("[KRN] ## REBOOT...\n"));
                asm volatile ("mov pc, #0\n"); // Jump to the reset vector..
                break;
            }
            default:
                core_SysCall(swi_no, regs);
                break;
        }
    }
    else
    {
        D(bug("[KRN] ## SWI : ILLEGAL SWI!\n"));
        return;
    }

    D(bug("[KRN] ## SWI returning ..\n"));
}
