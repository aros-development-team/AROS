/*
 * This file is intended to make generic kernel.resource compiling.
 * This code should NEVER be executed. This file MUST be overriden in
 * architecture-specific code for the scheduler to work!
 */

#include <kernel_base.h>
#include <kernel_debug.h>

void cpu_Switch(regs_t *regs)
{
}

void cpu_Dispatch(regs_t *regs)
{
}
