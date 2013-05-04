/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
*/

/*
 * CPU-specific definitions.
 *
 * Architectures with the same CPU will likely share single kernel_cpu.h
 * in arch/$(CPU)-all/kernel/kernel_cpu.h
 *
 * As you can see, this file is just a sample.
 */

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_
 
/* Number of exceptions supported by the CPU. Needed by kernel_base.h */
#define EXCEPTIONS_COUNT 1

/* CPU context stored in task's et_RegFrame. Just a dummy sample definition. */
struct AROSCPUContext
{
    IPTR pc;
};

typedef struct AROSCPUContext regs_t;

/* User/supervisor mode switching */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

/* A command to issue a syscall */
#define krnSysCall(num)

#endif
