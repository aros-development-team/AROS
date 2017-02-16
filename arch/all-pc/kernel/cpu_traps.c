/*
    Copyright © 2011-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Common trap handling routines for x86 CPU
    Lang: English
*/

#include <exec/execbase.h>
#include <exec/tasks.h>
#include <proto/exec.h>

#include "kernel_base.h"
#include "kernel_debug.h"
#include "kernel_interrupts.h"
#include "kernel_intr.h"
#include "cpu_traps.h"

#define D(x)
#define DUMP_CONTEXT

#ifdef DUMP_CONTEXT

static void PrintContext(struct ExceptionContext *regs, unsigned long error_code)
{
    int i;
    unsigned long *ptr;
    struct Task *t = GET_THIS_TASK;

    if (t)
    {
        bug("[Kernel]  %s %p '%s'\n", t->tc_Node.ln_Type == NT_TASK?"task":"process", t, t->tc_Node.ln_Name);
        bug("[Kernel] SPLower=%p SPUpper=%p\n", t->tc_SPLower, t->tc_SPUpper);

        if (((void *)SP(regs) < t->tc_SPLower) || ((void *)SP(regs) > t->tc_SPUpper))
            bug("[Kernel] Stack out of Bounds!\n");
    }

    bug("[Kernel] Error 0x%p\n", error_code);
    PRINT_CPUCONTEXT(regs);

    bug("[Kernel] Stack:\n");
    ptr = (unsigned long *)SP(regs);
    for (i=0; i < 10; i++)
        bug("[Kernel] %02x: %p\n", i * sizeof(unsigned long), ptr[i]);
}

#else

#define PrintContext(regs, err)

#endif

/*
 * This table is used to translate x86 trap number
 * to AmigaOS trap number to be passed to exec exception handler.
 */
static const char AmigaTraps[] =
{
     5,  9, -1,  4, 11, 2,
     4,  0,  8, 11,  3, 3,
     2,  8,  3, -1, 11, 3,
    -1
};

void cpu_Trap(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number)
{
    D(bug("[Kernel] Trap exception %u\n", irq_number));

#if defined(EMULATE_SYSBASE) && (__WORDSIZE==64)
    if (irq_number == 0x0e)
    {
        uint64_t ptr = rdcr(cr2);
	unsigned char *ip = (unsigned char *)regs->rip;

	D(bug("[Kernel] Page fault exception\n"));

        if (ptr == EMULATE_SYSBASE)
        {
            D(bug("[Kernel] ** Code at 0x%p is trying to access the SysBase at 0x%p.\n", ip, ptr));

            if ((ip[0] & 0xfb) == 0x48 &&
                 ip[1]         == 0x8b && 
                (ip[2] & 0xc7) == 0x04 &&
                 ip[3]         == 0x25)
            {
                int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

                switch(reg)
                {
                    case 0:
                        regs->rax = (UQUAD)SysBase;
                        break;
                    case 1:
                        regs->rcx = (UQUAD)SysBase;
                        break;
                    case 2:
                        regs->rdx = (UQUAD)SysBase;
                        break;
                    case 3:
                        regs->rbx = (UQUAD)SysBase;
                        break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                    case 5:
                        regs->rbp = (UQUAD)SysBase;
                        break;
                    case 6:
                        regs->rsi = (UQUAD)SysBase;
                        break;
                    case 7:
                        regs->rdi = (UQUAD)SysBase;
                        break;
                    case 8:
                        regs->r8 = (UQUAD)SysBase;
                        break;
                    case 9:
                        regs->r9 = (UQUAD)SysBase;
                        break;
                    case 10:
                        regs->r10 = (UQUAD)SysBase;
                        break;
                    case 11:
                        regs->r11 = (UQUAD)SysBase;
                        break;
                    case 12:
                        regs->r12 = (UQUAD)SysBase;
                        break;
                    case 13:
                        regs->r13 = (UQUAD)SysBase;
                        break;
                    case 14:
                        regs->r14 = (UQUAD)SysBase;
                        break;
                    case 15:
                        regs->r15 = (UQUAD)SysBase;
                        break;
                }

                regs->rip += 8;
                
                core_LeaveInterrupt(regs);
            }
            else if ((ip[0] & 0xfb) == 0x48 &&
                      ip[1]         == 0x8b && 
                     (ip[2] & 0xc7) == 0x05)
            {
                int reg = ((ip[2] >> 3) & 0x07) | ((ip[0] & 0x04) << 1);

                switch(reg)
                {
                    case 0:
                        regs->rax = (UQUAD)SysBase;
                        break;
                    case 1:
                        regs->rcx = (UQUAD)SysBase;
                        break;
                    case 2:
                        regs->rdx = (UQUAD)SysBase;
                        break;
                    case 3:
                        regs->rbx = (UQUAD)SysBase;
                        break;
//                    case 4:   /* Cannot put SysBase into rSP register */
//                        regs->rsp = (UQUAD)SysBase;
//                        break;
                    case 5:
                        regs->rbp = (UQUAD)SysBase;
                        break;
                    case 6:
                        regs->rsi = (UQUAD)SysBase;
                        break;
                    case 7:
                        regs->rdi = (UQUAD)SysBase;
                        break;
                    case 8:
                        regs->r8 = (UQUAD)SysBase;
                        break;
                    case 9:
                        regs->r9 = (UQUAD)SysBase;
                        break;
                    case 10:
                        regs->r10 = (UQUAD)SysBase;
                        break;
                    case 11:
                        regs->r11 = (UQUAD)SysBase;
                        break;
                    case 12:
                        regs->r12 = (UQUAD)SysBase;
                        break;
                    case 13:
                        regs->r13 = (UQUAD)SysBase;
                        break;
                    case 14:
                        regs->r14 = (UQUAD)SysBase;
                        break;
                    case 15:
                        regs->r15 = (UQUAD)SysBase;
                        break;
                }
                
                regs->rip += 7;
                
                core_LeaveInterrupt(regs);
            }
                D(else bug("[Kernel] Instruction not recognized\n"));
        }

#ifdef DUMP_CONTEXT
	unsigned int i;

        bug("[Kernel] PAGE FAULT accessing 0x%p\n", ptr);
        bug("[Kernel] Insn: ");
        for (i = 0; i < 16; i++)
            bug("%02x ", ip[i]);
        bug("\n");
#endif

	/* The exception will now be passed on to handling code below */
    }
#endif

    if (krnRunExceptionHandlers(KernelBase, irq_number, regs))
	return;

    D(bug("[Kernel] Passing on to exec, Amiga trap %d\n", AmigaTraps[irq_number]));

    if (AmigaTraps[irq_number] != -1)
    {
	if (core_Trap(AmigaTraps[irq_number], regs))
	{
	    /* If the trap handler returned, we can continue */
	    D(bug("[Kernel] Trap handler returned\n"));
	    return;
	}
    }

    bug("[Kernel] UNHANDLED EXCEPTION %lu\n", irq_number);
    PrintContext(regs, error_code);

    while (1) asm volatile ("hlt");
}
