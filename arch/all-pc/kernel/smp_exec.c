/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)
#include <proto/exec.h>
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>

#include <exec/rawfmt.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "apic.h"

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "etask.h"

extern BOOL Exec_InitETask(struct Task *, struct Task *, struct ExecBase *);


void cpu_PrepareExec(struct ExecBase *SysBase)
{
    struct Task *currentTask;
    void *SystemCPUMask;
    
    /*
     * Setup the available CPU Mask in ExecBase,
     * and mark the boot CPU as available
    */
    SystemCPUMask = KrnAllocCPUMask();
    KrnGetCPUMask(0, SystemCPUMask);

    PrivExecBase(SysBase)->CPUMask = SystemCPUMask;

    /*
     * make sure all current tasks have affinity masks
     */
    currentTask = FindTask(NULL);
    IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = KrnAllocCPUMask();
    KrnGetCPUMask(0, IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);

    ForeachNode(&SysBase->TaskReady, currentTask)
    {
        IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = KrnAllocCPUMask();
        KrnGetCPUMask(0, IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
    }
    ForeachNode(&SysBase->TaskWait, currentTask)
    {
        IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = KrnAllocCPUMask();
        KrnGetCPUMask(0, IntETask(currentTask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
    }

    PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;
}

struct Task *cpu_InitBootStrap(cpuid_t cpuNo)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
#define bstask          ((struct Task *)(ml->ml_ME[0].me_Addr))
#define bstaskmlsize    (sizeof(struct MemList) + sizeof(struct MemEntry))
    IPTR bstNameArg[1];

    /* Build bootstraps memory list */
    if ((ml = AllocMem(bstaskmlsize, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate memory for bootstrap task\n", cpuNo);
        return NULL;
    }

    D(bug("[Kernel:%03u] Allocated memory list for bootstrap task\n", cpuNo);)

    ml->ml_ME[0].me_Length = sizeof(struct Task);
    if ((ml->ml_ME[0].me_Addr = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate task for bootstrap\n", cpuNo);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    D(bug("[Kernel:%03u] %s: Bootstrap task @ 0x%p\n", cpuNo, __func__, bstask));

    NEWLIST(&bstask->tc_MemEntry);
    AddHead(&bstask->tc_MemEntry, &ml->ml_Node);

    if (KernelBase->kb_ContextSize > AROS_ROUNDUP2(sizeof(struct AROSCPUContext), 16) + sizeof(struct FPFXSContext))
    {
        D(bug("[Kernel:%03u] %s: Enabling AVX ...\n", cpuNo, __func__));
        D(bug("[Kernel:%03u] %s: Adjusting CR0 flags\n", cpuNo, __func__);)
        /* Enable monitoring media instruction to generate #NM when CR0.TS is set.
         * Disable coprocessor emulation.
         */
        wrcr(cr0, (rdcr(cr0) & ~_CR0_EM) | _CR0_MP);
        
        D(bug("[Kernel:%03u] %s: Enable #XF & XGETVB\n", cpuNo, __func__);)
        /* Enable #XF instead of #UD when a SIMD exception occurs
         * Enable xgetvb/xsetvb
         */ 
        wrcr(cr4, rdcr(cr4) | _CR4_OSXMMEXCPT | _CR4_OSXSBV);
        D(bug("[Kernel:%03u] %s: Enable Saving AVX context\n", cpuNo, __func__);)
        asm volatile (
            "    xor        %%rcx, %%rcx\n\t"
            "    xgetbv\n\t"                                                            /* Load XCR0 register                           */
            "    or         $0b111, %%eax\n\t"                                          /* Set FPU, XMM, YMM bits                       */
            "    xsetbv\n\t"                                                            /* Save back to XCR0                            */
            : : : "%eax",  "%rcx"
            );
        D(bug("[Kernel:%03u] %s: AVX Ready!\n", cpuNo, __func__);)
    }
    D(bug("[Kernel:%03u] %s: Allocating CPU context...\n", cpuNo, __func__));

    if ((bsctx = KrnCreateContext()) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to create the bootstrap Task context\n", cpuNo);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    D(bug("[Kernel:%03u] %s: CPU Ctx @ 0x%p\n", cpuNo, __func__, bsctx));


    ml->ml_ME[1].me_Length = 20;
    if ((ml->ml_ME[1].me_Addr = AllocMem(20, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate the bootstrap Task name\n", cpuNo);
        KrnDeleteContext(bsctx);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    ml->ml_NumEntries      = 2;
    D(bug("[Kernel:%03u] %s: Task Memory Allocated\n", cpuNo, __func__);)

    bstask->tc_Node.ln_Name = ml->ml_ME[1].me_Addr;
    bstNameArg[0] = cpuNo;
    RawDoFmt("CPU #%03u Bootstrap", (RAWARG)bstNameArg, RAWFMTFUNC_STRING, bstask->tc_Node.ln_Name);

    bstask->tc_Node.ln_Type = NT_TASK;
    bstask->tc_Node.ln_Pri  = 0;
    bstask->tc_State        = TS_READY;
    bstask->tc_SigAlloc     = 0xFFFF;

    /* Create an ETask structure and attach CPU context */
    if (!Exec_InitETask(bstask, NULL, SysBase))
    {
        bug("[Kernel:%03u] FATAL : Failed to initialize bootstrap ETask\n", cpuNo);
        KrnDeleteContext(bsctx);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    bstask->tc_UnionETask.tc_ETask->et_RegFrame = bsctx;

    D(bug("[Kernel:%03u] %s: ETask initialized\n", cpuNo, __func__);)

    /* the bootstrap can only run on this CPU */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpuNo;
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity = KrnAllocCPUMask();
    if (!IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity)
    {
        bug("[Kernel:%03u] FATAL : Failed to initialize bootstrap CPU Affinity\n", cpuNo);
        KrnDeleteContext(bsctx);
        FreeMem(ml->ml_ME[1].me_Addr, ml->ml_ME[1].me_Length);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    KrnGetCPUMask(cpuNo, IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);

    D(bug("[Kernel:%03u] %s: Initialization Complete\n", cpuNo, __func__);)

    bsctx->Flags = 0;

    return bstask;
#undef  bstask
}

void cpu_BootStrap(struct Task *bstask)
{
    struct APICData *apicData  = KernelBase->kb_PlatformData->kb_APIC;
    cpuid_t cpuNo = KrnGetCPUNumber();

    D(
        bug("[Kernel:%03u] %s()\n", cpuNo, __func__);
        
        if (IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber != cpuNo)
            bug("[Kernel:%03u] %s: bstask running on wrong CPU? (task cpu = %03u)\n", cpuNo, __func__, IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber);
    )

    bstask->tc_State = TS_RUN;
    SET_THIS_TASK(bstask);

    D(bug("[Kernel:%03u] %s: Leaving supervisor mode\n", cpuNo, __func__));

    krnLeaveSupervisorRing(FLAGS_INTENABLED);

    D(bug("[Kernel:%03u] %s: Initialising APIC...\n", cpuNo, __func__));
    core_APIC_Init(apicData, cpuNo);

    D(
        bug("[Kernel:%03u] %s: Initialising Scheduler...\n", cpuNo, __func__);
        bug("[Kernel:%03u] %s:        Enabling Exec Interrupts...\n", cpuNo, __func__);
    )

    /* Let the system know this CPU is available .. */
    KrnGetCPUMask(cpuNo, PrivExecBase(SysBase)->CPUMask);

    /* We now start up the interrupts */
    Permit();
    Enable();

    D(bug("[Kernel:%03u] %s:        Creating Idle Task ...\n", cpuNo, __func__));

    Exec_X86CreateIdleTask(SysBase);

    D(bug("[Kernel:%03u] %s: Done\n", cpuNo, __func__));
}
#endif
