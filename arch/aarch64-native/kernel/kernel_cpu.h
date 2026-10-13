/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#ifndef CPU_AARCH64_H_
#define CPU_AARCH64_H_

#include <inttypes.h>
#include "kernel_arm.h"
#include "tls.h"

extern uint32_t __arm_affinitymask;

#define EXCEPTIONS_COUNT	1

#define ARM_FPU_TYPE	        FPU_VFP
#define ARM_FPU_SIZE	        32*128

/* We use native context format, no conversion needed */
#define regs_t struct ExceptionContext
/* There are no private add-ons */
#define AROSCPUContext ExceptionContext

/* AArch64 PSTATE/SPSR mode field values */
#define CPUMODE_EL0t    0x00
#define CPUMODE_EL1t    0x04
#define CPUMODE_EL1h    0x05
/*
 * "USER" is the level AROS tasks run at. AROS runs the OS privileged: tasks
 * execute at EL1t (SPSel=0, SP_EL0), supervisor (SuperState) at EL1h. EL0 is
 * unusable on AArch64 (no ARMv7-style domains; EL0-writable pages are forced
 * PXN). The scheduler-syscall gate and SC_ISSUPERSTATE compare the saved mode
 * against this, so it must be the task level (EL1t), not EL0t.
 */
#define CPUMODE_USER    CPUMODE_EL1t
#define CPUMODE_SYSTEM  CPUMODE_EL1h
#define CPUMODE_MASK    0x0f

/* SPSR/PSTATE bit positions */
#define PSTATE_I_BIT    (1 << 7)    /* IRQ mask */
#define PSTATE_F_BIT    (1 << 6)    /* FIQ mask */
#define PSTATE_A_BIT    (1 << 8)    /* SError mask */
#define PSTATE_D_BIT    (1 << 9)    /* Debug mask */

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

typedef uint8_t cpumode_t;

#define goSuper() 0
#define goBack(mode) do { (void)(mode); } while(0)
#define goUser()

#undef krnSysCall
#define krnSysCall(n) asm volatile ("svc %[svc_no]\n\t" : : [svc_no] "I" (n) : "x30");

void cpu_DumpRegs(regs_t *regs);

/* Logical id from TLS, set by the boot CPU: MPIDR_EL1 is unreliable on
 * Pi 3, some armstubs leave a core's EL1 view at 0. */
static inline int GetCPUNumber() {
    tls_t *__tls;
    asm volatile ("mrs %0, tpidr_el1" : "=r" (__tls));
    return (int)__tls->CPUNumber;
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

/* Refreshes iet_CpuUsage for TaskTag_CPUUsage, from the timer IRQ. */
#define TASKUSAGE_WINDOW        1000000 /* microseconds */

void core_TaskCPUUsage(void);

#if defined(__AROSEXEC_SMP__)
/*
 * Per-core load for KrnGetSystemAttr(KATTR_CPULoad + cpu): the inverse of
 * the idle task's share of the window. Its busy time is already in
 * iet_private2, so only the previous sample is kept here.
 */
#define AARCH64_MAXCPUS     4

struct aarch64_CPULoadData
{
    UQUAD       cpl_LastIdle;       /* idle busy time at the last sample */
    UQUAD       cpl_LastStamp;      /* when that sample was taken (us)   */
    ULONG       cpl_LastLoad;       /* the load that sample produced     */
};

/* Shortest window worth measuring - callers asking again within it are
 * served the previous result. */
#define CPULOAD_MINWINDOW       100000  /* microseconds */

extern struct Task *aarch64_IdleTask[AARCH64_MAXCPUS];
extern struct aarch64_CPULoadData aarch64_CPULoad[AARCH64_MAXCPUS];
#endif

#endif /* CPU_AARCH64_H_ */
