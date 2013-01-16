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

typedef struct ExceptionContext regs_t;

static inline uint32_t goSuper()
{
    asm volatile ("cps #0x13\n");	/* switch to SVC (supervisor) mode */
    return NULL;
}

static inline void goUser()
{
    asm volatile ("cps #0x1f\n");	/* switch to system (user) mode */
}

static inline void krnSysCall(uint8_t n)
{
    asm volatile ("swi %[n]\n" : : [n] "I" (n));
}

#endif /* CPU_ARM_H_ */
