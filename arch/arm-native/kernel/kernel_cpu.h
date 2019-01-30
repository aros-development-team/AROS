/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef CPU_ARM_H_
#define CPU_ARM_H_

#include <inttypes.h>
#include "kernel_arm.h"

extern uint32_t __arm_affinitymask;

#define EXCEPTIONS_COUNT	1

#define ARM_FPU_TYPE	        FPU_VFP
#define ARM_FPU_SIZE	        32*64

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

#define goSuper() 0
#define goUser()

#undef krnSysCall
#define krnSysCall(n) asm volatile ("swi %[swi_no]\n\t" : : [swi_no] "I" (n) : "lr");

void cpu_DumpRegs(regs_t *regs);

static inline int GetCPUNumber() {
    int tmp;
    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));
    return tmp & 3;
}

static inline void SendIPISelf(uint32_t ipi, uint32_t ipi_param)
{
    int cpu = GetCPUNumber();
    __arm_arosintern.ARMI_SendIPI((ipi & 0x0fffffff) | (cpu << 28), ipi_param, (1 << cpu));
}

static inline void SendIPIOthers(uint32_t ipi, uint32_t ipi_param)
{
    int cpu = GetCPUNumber();
    __arm_arosintern.ARMI_SendIPI((ipi & 0x0fffffff) | (cpu << 28), ipi_param, 0xf & ~(1 << cpu));
}

static inline void SendIPIAll(uint32_t ipi, uint32_t ipi_param)
{
    int cpu = GetCPUNumber();
    __arm_arosintern.ARMI_SendIPI((ipi & 0x0fffffff) | (cpu << 28), ipi_param, 0xf);
}

#endif /* CPU_ARM_H_ */
