/*
 * This file is intended to make generic kernel.resource compiling.
 * This code should NEVER be executed. This file MUST be overriden in
 * architecture-specific code for the scheduler to work!
 */

#include <kernel_base.h>
#include <kernel_debug.h>

void cpu_Switch(struct ExceptionContext *regs)
{
    bug("[KRN] KERNEL PANIC! cpu_Switch() is not implemented!\n");
    for (;;);
}

void cpu_Dispatch(struct ExceptionContext *regs)
{
    bug("[KRN] KERNEL PANIC! cpu_Dispatch() is not implemented!\n");
    for (;;);
}
