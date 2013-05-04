/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

/*
 * This file is intended to make generic kernel.resource compiling.
 * This code should NEVER be executed. This file MUST be overriden in
 * architecture-specific code for the scheduler to work!
 */

#include <kernel_base.h>
#include <kernel_debug.h>

void cpu_Switch(regs_t *regs)
{
    bug("[KRN] KERNEL PANIC! cpu_Switch() is not implemented!\n");
    for (;;);
}

void cpu_Dispatch(regs_t *regs)
{
    bug("[KRN] KERNEL PANIC! cpu_Dispatch() is not implemented!\n");
    for (;;);
}
