/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef CPU_ARM_H_
#define CPU_ARM_H_

#include <inttypes.h>

#define EXCEPTIONS_COUNT	1

#define ARM_FPU_TYPE	        FPU_VFP
#define ARM_FPU_SIZE	        32*64

/* our "user" mode is SYSTEM_MODE */
#define CTX_MODE 0x1f

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

#define goSuper() 0
#define goUser()

#define krnSysCall(n) asm volatile ("swi %[swi_no]\n\t" : : [swi_no] "I" (n));

void cpu_DumpRegs(regs_t *regs);

#endif /* CPU_ARM_H_ */
