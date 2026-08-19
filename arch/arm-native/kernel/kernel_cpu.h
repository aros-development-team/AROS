/*
    Copyright (C) 2013, The AROS Development Team. All rights reserved.
*/

#ifndef CPU_ARM_H_
#define CPU_ARM_H_

#include <inttypes.h>
#include "kernel_arm.h"
#include "tls.h"

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

/*
 * Logical CPU id from per-CPU TLS (TPIDRURO), populated by the boot CPU
 * when it brings each core up. MPIDR/VMPIDR is not a reliable source on
 * Pi 3 - some armstub revisions leave a core's EL1 MPIDR view at 0 - so
 * we do not read c0,c0,5 here.
 */
static inline int GetCPUNumber() {
    tls_t *__tls;
    asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r" (__tls));
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

#if defined(__AROSEXEC_SMP__)
/*
 * Per-core load accounting for KrnGetSystemAttr(KATTR_CPULoad + cpu),
 * which processor.resource exposes as GCIT_ProcessorLoad (SysMon's CPU
 * graphs). A core's load is the inverse of its idle task's share of the
 * elapsed window; that task's cumulative busy time is already kept in
 * iet_private2 by cpu_Switch, so all we store here is the previous
 * sample. exec registers each idle task as it creates them, because only
 * exec knows which task is which core's idler.
 */
#define ARM_MAXCPUS     4

struct arm_CPULoadData
{
    UQUAD       cpl_LastIdle;       /* idle busy time at the last sample */
    UQUAD       cpl_LastStamp;      /* when that sample was taken (us)   */
    ULONG       cpl_LastLoad;       /* the load that sample produced     */
};

/*
 * Shortest window we will measure. SysMon asks for each core's load two
 * or three times per refresh (graph, label, gauge); measuring a window of
 * a few microseconds would swing the answer between 0 and 100%, so those
 * extra reads are served the previous result instead. Well below any UI
 * refresh interval, so each refresh still gets a fresh sample.
 */
#define CPULOAD_MINWINDOW       100000  /* microseconds */

extern struct Task *arm_IdleTask[ARM_MAXCPUS];
extern struct arm_CPULoadData arm_CPULoad[ARM_MAXCPUS];
#endif

#endif /* CPU_ARM_H_ */
