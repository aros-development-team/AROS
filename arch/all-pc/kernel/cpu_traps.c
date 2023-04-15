/*
    Copyright (C) 2011-2023, The AROS Development Team. All rights reserved.

    Desc: Common trap handling routines for x86 CPU
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
 * AmigaTraps is used to map intel x86 exception numbers
 * to AmigaOS traps, so that they can be passed to exec
 * exception handlers.
 * 2	    Bus error	                            access to nonexistent memory
 * 3	    Address error	                        unaligned long/word access
 * 4	    Illegal instruction	                    illegal opcode (bar Line 1010 and 1111 emualted instructions)
 * 5	    Zero divide	                            division by zero
 * 6	    CHK instruction	                        register bounds error
 * 7	    TRAPV instruction	                    overflow error
 * 8	    Privilege violation
 * 9	    Trace	status register
 * 10	    Line 1010 emulator	                    executing opcodes beginning with $A
 * 11	    Line 1111 emulator                      executing opcodes beginning with $F
 *
 * TODO: Map x86 debug traps to amiga traps
 * 32-47	Trap instructions                       TRAP<x> instruction, where x ranges from 0 to 15
 *
 */
#define AMIGATRAP_COUNT 19
static const char AmigaTraps[AMIGATRAP_COUNT] =
{
     5,  9, -1,  4, 11, 2,
     4,  0,  8, 11,  3, 3,
     2,  8,  3, -1, 11, 3,
    -1
};

extern BOOL IsKernelBaseReady(struct ExecBase *SysBase);

void cpu_Trap(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number)
{
    D(bug("[Kernel] %s(%u)\n", __func__, irq_number));

    if (!krnRunExceptionHandlers(KernelBase, irq_number, regs))
    {
        if (IsKernelBaseReady(SysBase) &&
            (irq_number < AMIGATRAP_COUNT) && (AmigaTraps[irq_number] != -1))
        {
            D(bug("[Kernel] %s(%u): Forwarding to exec <Amiga trap #%d, exception error %08x>\n", __func__, irq_number, AmigaTraps[irq_number], error_code);)

            if (core_Trap(AmigaTraps[irq_number], regs))
            {
                /* If the trap handler returned, we can continue */
                D(bug("[Kernel] %s(%u): Trap handler(s) returned\n", __func__, irq_number));
                goto trapDone;
            }
        }

        /* Halt for all unhandled exceptions except spurious interrupts */
        if (irq_number != APIC_EXCEPT_SPURIOUS)
        {
            bug("[Kernel] %s(%u) UNHANDLED EXCEPTION\n", __func__, irq_number);
            PrintContext(regs, error_code);
            X86_HandleSysHaltSC(regs);
        }
    }

    /*
     * If its an APIC exception, but not Syscall, send EOI
     */
    if ((irq_number >= X86_CPU_EXCEPT_COUNT) && (irq_number < APIC_EXCEPT_SYSCALL))
    {
        D(bug("[Kernel] %s(%u): Sending EOI to LAPIC on CPU%03x\n", __func__, irq_number, KrnGetCPUNumber());)
        IPTR __APICBase = core_APIC_GetBase();
        APIC_REG(__APICBase, APIC_EOI) = 0;
    }

trapDone:
    return;
}
