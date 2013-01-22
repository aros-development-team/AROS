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

static inline uint32_t goSuper()
{
    asm volatile ("cps #0x13\n");	/* switch to SVC (supervisor) mode */
    return (uint32_t)NULL;
}

static inline void goUser()
{
    asm volatile ("cps #0x1f\n");	/* switch to system (user) mode */
}

#define krnSysCall(n) asm volatile ("swi %[swi_no]\n\t" : : [swi_no] "I" (n));

#endif /* CPU_ARM_H_ */
