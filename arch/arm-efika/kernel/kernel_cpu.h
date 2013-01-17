/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: cpu_arm.h
    Lang: english
*/

#ifndef CPU_ARM_H_
#define CPU_ARM_H_

#include <inttypes.h>

#define EXCEPTIONS_COUNT	1

#define AROSCPUContext ExceptionContext

typedef struct {
	uint32_t r[16];
} regs_t;

static inline uint32_t goSuper()
{
	return 0;
}

static inline void goUser()
{

}

static inline void goBack(uint32_t mode)
{

}

static inline void ictl_enable_irq(uint8_t irq)
{

}

static inline void ictl_disable_irq(uint8_t irq)
{

}

static inline void krnSysCall(uint8_t n)
{
}

typedef uint8_t cpumode_t;


#define ARM_FPU_TYPE	FPU_VFP
#define ARM_FPU_SIZE	32*64

#endif /* CPU_ARM_H_ */
