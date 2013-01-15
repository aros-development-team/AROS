/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <stddef.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/kernel.h>

#include "kernel_cpu.h"
#include "kernel_intern.h"
#include "kernel_scheduler.h"

#include "kernel_syscall.h"

extern char * __text_start;
extern char * __text_end;

#ifndef _CUSTOM
#define _CUSTOM NULL
#endif

asm (".globl __intrhand_swi\n\t"
    ".type __intrhand_swi,%function\n"
    "__intrhand_swi:\n"
    "           sub     sp, sp, #4             \n" // make space to store spsr
    "           stmfd   sp!, {r0-r12, lr}      \n" // store registers to pass to c handler ..
    "           mrs     r2, spsr               \n" // store spsr above registers
    "           str     r2, [sp, #14*4]        \n"
    "           mov     r0, sp                 \n" // r0 = registers r0-r12 on the stack
    "           mov     r1, sp                 \n"
    "           add     r1, r1, #15*4          \n" // r1 = orig (callers) stack pointer
    "           bl      handle_syscall         \n"
    "           ldr     r2, [sp, #14*4]        \n" // restore spsr
    "           msr     spsr_c, r2             \n"
    "           ldmfd   sp!, {r0-r12, lr}      \n" // restore registers
    "           add     sp, sp, #4             \n"
    "           mov     pc, lr                 \n"
);

void core_Cause(unsigned char n, unsigned int mask)
{
    D(bug("[KRN] core_Cause(%d, %08x)", n, mask));

    if (SysBase)
    {
        struct IntVector *iv = &SysBase->IntVects[n];

        /* If the SoftInt vector in SysBase is set, call it. It will do the rest for us */
        if (iv->iv_Code)
            AROS_INTC3(iv->iv_Code, iv->iv_Data, mask, _CUSTOM);
    }
}

void handle_syscall(void *regs, void *tasksp)
{
    register unsigned int addr;
    register unsigned int swi_no;
    struct AROSCPUContext *ctx;
    struct Task *thisTask;

    /* We determine the SWI number by reading in the return address
       from the link register, subtract the instruction from it and
       obtain the value from there.  we also use this to check if
       we have been called from outwith the kernel's code (illegal!)
     */

    addr = ((uint32_t *)regs)[13];
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
        if ((thisTask = SysBase->ThisTask) != NULL)
        {
            int i;
            ctx = thisTask->tc_UnionETask.tc_ETask->et_RegFrame;
            for (i = 1; i < 13; i++)
            {
                ctx->r[i] = ((uint32_t *)regs)[i];
            }
            thisTask->tc_SPReg = tasksp;
        }
        
    
        switch (swi_no)
        {
            case SC_CLI:
            {
                D(bug("[KRN] ## CLI...\n"));
                break;
            }

            case SC_STI:
            {
                D(bug("[KRN] ## STI...\n"));
                break;
            }

            case SC_SUPERSTATE:
            {
                D(bug("[KRN] ## SUPERSTATE...\n"));
                break;
            }

            case SC_ISSUPERSTATE:
            {
                D(bug("[KRN] ## ISSUPERSTATE...\n"));
                break;
            }

            case SC_CAUSE:
            {
                D(bug("[KRN] ## CAUSE...\n"));
                //core_Cause(SysBase);
                break;
            }

            case SC_DISPATCH:
            {
                D(bug("[KRN] ## DISPATCH...\n"));
                core_Dispatch();
                break;
            }

            case SC_SWITCH:
            {
                D(bug("[KRN] ## SWITCH...\n"));
                core_Switch();
                break;
            }

            case SC_SCHEDULE:
            {
                D(bug("[KRN] ## SCHEDULE...\n"));
                core_Schedule();
                break;
            }

            case SC_RTAS:
            {
                break;
            }

            case SC_INVALIDATED:
            {
                break;
            }

            case SC_REBOOT:
            {
                D(bug("[KRN] ## REBOOT...\n"));
                asm volatile ("mov pc, #0\n"); // Jump to the reset vector..
                break;
            }
        }
    }
    else
    {
        D(bug("[KRN] ## SWI : ILLEGAL SWI!\n"));
        return;
    }

    D(bug("[KRN] ## SWI returning ..\n"));
}
