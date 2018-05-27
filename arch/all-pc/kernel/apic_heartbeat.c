/*
    Copyright © 2017-2018, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/types/timespec_s.h>
#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "kernel_intern.h"

#include "intservers.h"

/*
 * Unlike the VBlankServer, we might not run at a fixed 60Hz.
 */
int APICHeartbeatServer(struct ExceptionContext *regs, struct KernelBase *KernelBase, struct ExecBase *SysBase)
{
    struct PlatformData *pdata = KernelBase->kb_PlatformData;
    struct APICData *apicData = pdata->kb_APIC;
#if defined(__AROSEXEC_SMP__)
    IPTR __LAPICBase = apicData->lapicBase;
    struct X86SchedulerPrivate  *apicScheduleData;
    tls_t *apicTLS;
#endif
    UWORD current;

    if (apicData->flags & APF_TIMER)
    {
#if defined(__AROSEXEC_SMP__)
        apicid_t cpuNum = core_APIC_GetNumber(apicData);
        UQUAD now = RDTSC();
        
        // Update LAPIC tick
        apicData->cores[cpuNum].cpu_LAPICTick += APIC_REG(__LAPICBase, APIC_TIMER_ICR);

        // Relaunch LAPIC timer
        APIC_REG(__LAPICBase, APIC_TIMER_ICR) = (apicData->cores[cpuNum].cpu_TimerFreq + 500) / 1000;

        if ((now - apicData->cores[cpuNum].cpu_LastCPULoadTime) > apicData->cores[cpuNum].cpu_TSCFreq)
        {
            struct Task *t;
            UQUAD timeCur = now - apicData->cores[cpuNum].cpu_LastCPULoadTime;
            
            /* Lock all lists to make sure we catch all the tasks */
            KrnSpinLock(&PrivExecBase(SysBase)->TaskReadySpinLock, NULL, SPINLOCK_MODE_READ);
            ForeachNode(&SysBase->TaskReady, t)
            {
                if (cpuNum == IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuNumber)
                {
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage = 
                        (IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 << 32) / timeCur;
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 = 0;
                }
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskReadySpinLock);

            KrnSpinLock(&PrivExecBase(SysBase)->TaskWaitSpinLock, NULL, SPINLOCK_MODE_READ);
            ForeachNode(&SysBase->TaskWait, t)
            {
                if (cpuNum == IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuNumber)
                {
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage = 
                        (IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 << 32) / timeCur;
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 = 0;
                }
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskWaitSpinLock);

            KrnSpinLock(&PrivExecBase(SysBase)->TaskRunningSpinLock, NULL, SPINLOCK_MODE_READ);
            ForeachNode(&PrivExecBase(SysBase)->TaskRunning, t)
            {
                if (cpuNum == IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuNumber)
                {
                    /* 
                        TaskRunning list is different than others. Here the iet_private2 field is not yet updated,
                        so we have to update the CPU time in this place.
                    */
                    UQUAD time = IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 + 
                                 now - IntETask(t->tc_UnionETask.tc_ETask)->iet_private1;

                    if (time < timeCur)
                        IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage = (time << 32) / timeCur;
                    else
                        IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage = 0xffffffff;

                    IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 -= time;
                }
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskRunningSpinLock);

            KrnSpinLock(&PrivExecBase(SysBase)->TaskSpinningLock, NULL, SPINLOCK_MODE_READ);
            ForeachNode(&PrivExecBase(SysBase)->TaskSpinning, t)
            {
                if (cpuNum == IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuNumber)
                {
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuUsage = 
                        (IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 << 32) / timeCur;
                    IntETask(t->tc_UnionETask.tc_ETask)->iet_private2 = 0;
                }
            }
            KrnSpinUnLock(&PrivExecBase(SysBase)->TaskSpinningLock);

            apicData->cores[cpuNum].cpu_Load = 
                ((apicData->cores[cpuNum].cpu_TSCFreq - apicData->cores[cpuNum].cpu_SleepTime) << 32) / timeCur;

            D(bug("[Kernel:APIC.%03u] %s() cpu load %08x\n", cpuNum, __func__, (ULONG)apicData->cores[cpuNum].cpu_Load));
            apicData->cores[cpuNum].cpu_SleepTime = 0;
            apicData->cores[cpuNum].cpu_LastCPULoadTime = now;
        }

        D(bug("[Kernel:APIC.%03u] %s(), tick=%08x:%08x\n", cpuNum, __func__, (ULONG)(apicData->cores[cpuNum].cpu_LAPICTick >> 32),
            (ULONG)(apicData->cores[cpuNum].cpu_LAPICTick & 0xffffffff)));

        apicTLS = apicData->cores[cpuNum].cpu_TLS;
        if ((apicTLS) && ((apicScheduleData = apicTLS->ScheduleData) != NULL))
        {
            if ((current = apicScheduleData->Elapsed))
            {
                if ((apicScheduleData->Elapsed = (current - apicScheduleData->Granularity)) == 0)
                {
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Quantum);
                    __AROS_ATOMIC_OR_L(apicScheduleData->ScheduleFlags, TLSSF_Switch);
                }
            }
        }
#else
        D(bug("[Kernel:APIC] %s()\n", __func__));

        current = SCHEDELAPSED_GET;
        if (current)
            SCHEDELAPSED_SET(--current);

        if (current == 0)
        {
            FLAG_SCHEDQUANTUM_SET;
            FLAG_SCHEDSWITCH_SET;
        }
#endif
    }

    return 1;
}
