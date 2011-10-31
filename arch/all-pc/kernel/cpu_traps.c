/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id:$

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
    struct Task *t = SysBase->ThisTask;

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
