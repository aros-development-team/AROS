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

#define DREGS(x)

/* r0 = passed to function, r1/r2 = temp */
asm (".globl __intrhand_swi\n\t"
    ".type __intrhand_swi,%function\n"
    "__intrhand_swi:\n"
    "           sub     sp, sp, #3*4           \n" // make space to store spsr, stack pointer and link register
    "           stmfd   sp!, {r0-r11}          \n" // store registers to pass to c handler ..
    "           str     lr, [sp, #14*4]        \n"
    "           mrs     r2, spsr               \n" // store spsr above registers
    "           str     r2, [sp, #15*4]        \n"
    "           mov     r0, sp                 \n" // r0 = registers r0-r12 on the stack
    "           str     ip, [sp, #13*4]        \n" // store passed in "callers" stack pointer ..
    "           ldr     ip, [ip, #-4]           \n" // restore r12 from the "callers" stack
    "           str     ip, [sp, #12*4]        \n" // and store it 
    "           ldr     r1, [sp, #1*4]         \n" // restore r1 ..
    "           ldr     r2, [sp, #2*4]         \n" // .. and r2 ..
    "           mov     fp, #0                 \n" // clear fp(??)
    "           bl      handle_syscall         \n"
    "           ldr     lr, [sp, #14*4]        \n" // restore lr
    "           ldr     r2, [sp, #15*4]        \n" // restore spsr
    "           msr     spsr_c, r2             \n"
    "           ldmfd   sp!, {r0-r11}          \n" // restore registers
    "           add     sp, sp, #12            \n" // correct the stack pointer .. 
    "           movs    pc, lr                 \n" // ..and return
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

void handle_syscall(void *regs)
{
    register unsigned int addr;
    register unsigned int swi_no;
    struct ExceptionContext *ctx;
    struct Task *thisTask;

    /* We determine the SWI number by reading in the return address
       from the link register, subtract the instruction from it and
       obtain the value from there.  we also use this to check if
       we have been called from outwith the kernel's code (illegal!)
     */

    addr = ((uint32_t *)regs)[14];
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
            D(bug("[KRN] SWI invoked in '%s'", thisTask->tc_Node.ln_Name));
            if ((ctx = thisTask->tc_UnionETask.tc_ETask->et_RegFrame) != NULL)
            {
                int i;
                
                D(bug(", ExceptionContext @ 0x%p", ctx));
                DREGS(bug("\n"));
                for (i = 0; i < 12; i++)
                {
                    ctx->r[i] = ((uint32_t *)regs)[i];
                    DREGS(bug("[KRN]      r%02d: 0x%08x\n", i, ctx->r[i]));
                }
                ctx->ip = ((uint32_t *)regs)[12];
                DREGS(bug("[KRN] (ip) r12: 0x%08x\n", ctx->ip));
                ctx->sp = ((uint32_t *)regs)[13];
                DREGS(bug("[KRN] (sp) r13: 0x%08x\n", ctx->sp));
                ctx->lr = ((uint32_t *)regs)[14];
                DREGS(bug("[KRN] (lr) r14: 0x%08x\n", ctx->lr));
                ctx->cpsr = ((uint32_t *)regs)[15];;
                DREGS(bug("[KRN]     cpsr: 0x%08x", ctx->cpsr));
                thisTask->tc_SPReg = ctx->sp;
            }
            D(bug("\n"));
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
