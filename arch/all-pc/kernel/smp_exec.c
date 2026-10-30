/*
    Copyright (C) 2017-2023, The AROS Development Team. All rights reserved.
*/

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
extern void Exec_CleanupETask(struct Task *, struct ExecBase *);


/* Pin a task that predates CPU affinity to the boot CPU */
static BOOL cpu_AssignBootMask(struct Task *task)
{
    struct IntETask *iet;
    void *aff;

    if (!task || !(task->tc_Flags & TF_ETASK) || !task->tc_UnionETask.tc_ETask)
        return TRUE;

    iet = IntETask(task->tc_UnionETask.tc_ETask);
    if (iet->iet_CpuAffinity)
        return TRUE;

    if (!(aff = KrnAllocCPUMask()))
    {
        bug("[Kernel:SMP] no memory for task affinity masks - staying on one CPU\n");
        return FALSE;
    }
    KrnGetCPUMask(0, aff);
    iet->iet_CpuAffinity = aff;

    return TRUE;
}

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
     * make sure all current tasks have affinity masks. A NULL mask is
     * invalid, so affinity is only switched on once every one has one.
     */
    if (!cpu_AssignBootMask(FindTask(NULL)))
        return;

    ForeachNode(&SysBase->TaskReady, currentTask)
    {
        if (!cpu_AssignBootMask(currentTask))
            return;
    }
    ForeachNode(&SysBase->TaskWait, currentTask)
    {
        if (!cpu_AssignBootMask(currentTask))
            return;
    }

    PrivExecBase(SysBase)->IntFlags |= EXECF_CPUAffinity;
}

/* Runs on the core itself: the control registers are per core */
static void cpu_SMPInitAVX(cpuid_t cpuNo)
{
    if (KernelBase->kb_ContextSize > CPUSSEContxtSize)
    {
        D(
            bug("[Kernel:%03u] %s: Enabling AVX ...\n", cpuNo, __func__);
            bug("[Kernel:%03u] %s: Adjusting CR0 flags\n", cpuNo, __func__);
        )
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
            : : : "%rax",  "%rcx",  "%rdx"
            );
        D(bug("[Kernel:%03u] %s: AVX Ready!\n", cpuNo, __func__);)
    }
}

/*
 * Build the bootstrap task for core cpuNo. Runs on the boot CPU before the
 * core is woken (smp_Wake), so the core never allocates before it has a
 * task: if the allocator's semaphore were contended then, it would wait
 * on behalf of no task at all.
 */

struct Task *cpu_InitBootStrap(cpuid_t cpuNo)
{
    struct ExceptionContext *bsctx;
    struct MemList *ml;
    struct Task *bstask;
#define bstaskmlsize    (sizeof(struct MemList) + sizeof(struct MemEntry))
    IPTR bstNameArg[1];

    /* Build bootstraps memory list */
    if ((ml = AllocMem(bstaskmlsize, MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate memory for bootstrap task\n", cpuNo);
        return NULL;
    }

    D(bug("[Kernel:%03u] %s: Allocated memory list for bootstrap task\n", cpuNo, __func__);)

    ml->ml_ME[0].me_Length = sizeof(struct Task);
    if ((ml->ml_ME[0].me_Addr = AllocMem(sizeof(struct Task), MEMF_PUBLIC|MEMF_CLEAR)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate task for bootstrap\n", cpuNo);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    bstask = (struct Task *)ml->ml_ME[0].me_Addr;
    D(bug("[Kernel:%03u] %s: Bootstrap task @ 0x%p\n", cpuNo, __func__, bstask);)

    NEWLIST(&bstask->tc_MemEntry);

    D(bug("[Kernel:%03u] %s: Allocating CPU context...\n", cpuNo, __func__);)
    if ((bsctx = KrnCreateContext()) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to create the bootstrap Task context\n", cpuNo);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }
    D(bug("[Kernel:%03u] %s: CPU Ctx @ 0x%p\n", cpuNo, __func__, bsctx);)

    ml->ml_ME[1].me_Length = 20;
    if ((ml->ml_ME[1].me_Addr = AllocMem(20, MEMF_PUBLIC)) == NULL)
    {
        bug("[Kernel:%03u] FATAL : Failed to allocate the bootstrap Task name\n", cpuNo);
        KrnDeleteContext(bsctx);
        FreeMem(ml->ml_ME[0].me_Addr, ml->ml_ME[0].me_Length);
        FreeMem(ml, bstaskmlsize);
        return NULL;
    }

    bstask->tc_Node.ln_Name = ml->ml_ME[1].me_Addr;
    bstNameArg[0] = cpuNo;
    RawDoFmt("CPU #%03u Bootstrap", (RAWARG)bstNameArg, RAWFMTFUNC_STRING, bstask->tc_Node.ln_Name);

    ml->ml_NumEntries      = 2;
    AddTail(&bstask->tc_MemEntry, &ml->ml_Node);

    D(bug("[Kernel:%03u] %s: Task Memory Allocated\n", cpuNo, __func__);)

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

    /* the bootstrap can only run on its core. Exec_InitETask gave it the
     * creating CPU's mask, so rewrite that one in place. */
    IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpuNo;
    KrnClearCPUMask(IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);
    KrnGetCPUMask(cpuNo, IntETask(bstask->tc_UnionETask.tc_ETask)->iet_CpuAffinity);

    D(bug("[Kernel:%03u] %s: Initialization Complete\n", cpuNo, __func__);)

    bsctx->Flags = 0;

    return bstask;
}

/* Free a bootstrap task whose core was never started */
void cpu_DisposeBootStrap(struct Task *bstask)
{
    struct MemList *mb, *mbnext;

    KrnDeleteContext(bstask->tc_UnionETask.tc_ETask->et_RegFrame);
    Exec_CleanupETask(bstask, SysBase);

    /* The task itself is in one of these MemLists */
    bstask->tc_MemEntry.lh_TailPred->ln_Succ = NULL;
    for (mb = (struct MemList *)bstask->tc_MemEntry.lh_Head; mb; mb = mbnext)
    {
        mbnext = (struct MemList *)mb->ml_Node.ln_Succ;
        FreeEntry(mb);
    }
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

    cpu_SMPInitAVX(cpuNo);

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
