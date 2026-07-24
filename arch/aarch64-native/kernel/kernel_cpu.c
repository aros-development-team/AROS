/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    AArch64 CPU support: probe, init, context switch, VFP/NEON, SMP trampoline.
*/

#include <aros/types/timespec_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <aros/aarch64/cpucontext.h>
#include <strings.h>

#include <aros/types/spinlock_s.h>

#include "kernel_base.h"

#include <proto/kernel.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include <kernel_objects.h>
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#include "tls.h"

#define D(x)
#define DSCHED(x)
#define DREGS(x)

#if defined(__AROSEXEC_SMP__)
extern struct Task *cpu_InitBootStrap(struct ExecBase *);
extern void cpu_BootStrap(struct Task *, struct ExecBase *);
#endif

extern void aarch64_flush_cache(uintptr_t, uint32_t);

/*
 * AArch64 SMP trampoline.
 * Secondary cores start here after being woken via mailbox.
 */
asm(
"       .globl mpcore_trampoline                \n"
"       .type mpcore_trampoline,%function       \n"
"mpcore_trampoline:                             \n"
"               mrs     x4, currentel           \n"
"               lsr     x4, x4, #2             \n"
"               cmp     x4, #2                 \n"
"               bne     mpcore_el1_entry        \n"
"               /* Drop from EL2 to EL1 */      \n"
"               mov     x4, #(1 << 31)         \n" /* HCR_EL2.RW = 1 (AArch64 at EL1) */
"               msr     hcr_el2, x4            \n"
"               mov     x4, #0x3c5             \n" /* SPSR_EL2: EL1h, DAIF masked */
"               msr     spsr_el2, x4           \n"
"               adr     x4, mpcore_el1_entry   \n"
"               msr     elr_el2, x4            \n"
"               eret                            \n"
"mpcore_el1_entry:                              \n"
"               /* Enable FP/NEON */            \n"
"               mov     x4, #(3 << 20)         \n"
"               msr     cpacr_el1, x4          \n"
"               isb                             \n"
"               /* Load page table base */      \n"
"               ldr     x3, mpcore_pde         \n"
"               msr     ttbr0_el1, x3          \n"
"               isb                             \n"
"               /* Load TCR_EL1 */              \n"
"               ldr     x3, mpcore_tcr         \n"
"               msr     tcr_el1, x3            \n"
"               /* Load MAIR_EL1 */             \n"
"               ldr     x3, mpcore_mair        \n"
"               msr     mair_el1, x3           \n"
"               /* Invalidate TLB */            \n"
"               tlbi    vmalle1                 \n"
"               dsb     sy                      \n"
"               isb                             \n"
"               /* Enable MMU */                \n"
"               mrs     x4, sctlr_el1          \n"
"               orr     x4, x4, #1             \n" /* MMU enable */
"               orr     x4, x4, #(1 << 2)     \n" /* D-cache */
"               orr     x4, x4, #(1 << 12)    \n" /* I-cache */
"               msr     sctlr_el1, x4          \n"
"               isb                             \n"
"               /* Set up stacks */             \n"
"               msr     spsel, #1              \n"
"               ldr     x3, mpcore_stack       \n"
"               mov     sp, x3                 \n"
"               /* Set TLS */                   \n"
"               ldr     x3, mpcore_tls         \n"
"               msr     tpidr_el1, x3          \n"
"               /* Jump to C entry */           \n"
"               ldr     x3, mpcore_code        \n"
"               br      x3                     \n"

"       .globl mpcore_pde                       \n"
"mpcore_pde:    .quad   0                       \n"
"mpcore_code:   .quad   0                       \n"
"mpcore_stack:  .quad   0                       \n"
"mpcore_tls:    .quad   0                       \n"
"       .globl mpcore_tcr                       \n"
"mpcore_tcr:    .quad   0                       \n"
"       .globl mpcore_mair                      \n"
"mpcore_mair:   .quad   0                       \n"
"       .globl mpcore_end                       \n"
"mpcore_end:  "
);

spinlock_t startup_lock;

void cpu_Register()
{
    uint64_t tmp;
#if defined(__AROSEXEC_SMP__)
    tls_t *__tls;
    struct ExecBase *SysBase;
#endif
    struct KernelBase *KernelBase;
    cpuid_t cpunum = GetCPUNumber();

    /* Enable I-cache, D-cache, branch prediction */
    asm volatile("mrs %0, sctlr_el1" : "=r"(tmp));
    tmp |= (1 << 2) | (1 << 12);     /* D-cache, I-cache */
    asm volatile("msr sctlr_el1, %0" : : "r"(tmp));
    asm volatile("isb");

    cpu_Init(&__arm_arosintern, NULL);

#if defined(__AROSEXEC_SMP__)
    __tls = TLS_PTR_GET();

    bug("[Kernel:%02d] Bootstrapping...\n", cpunum);

    bug("[Kernel:%02d] TLS @ 0x%p\n", cpunum, (__tls));
    KernelBase = (struct KernelBase *)__tls->KernelBase;
    SysBase = (struct ExecBase *)__tls->SysBase;
    bug("[Kernel:%02d] KernelBase @ 0x%p\n", cpunum, KernelBase);
    bug("[Kernel:%02d] SysBase @ 0x%p\n", cpunum, SysBase);

    if ((__tls->ThisTask = cpu_InitBootStrap(SysBase)) == NULL)
        goto cpu_registerfatal;

    if (__arm_arosintern.ARMI_InitCore)
        __arm_arosintern.ARMI_InitCore(KernelBase, SysBase);

    cpu_BootStrap(__tls->ThisTask, SysBase);
#else
    KernelBase = (struct KernelBase *)TLS_GET(KernelBase);
#endif

    bug("[Kernel:%02d] Operational\n", cpunum);

#if defined(__AROSEXEC_SMP__)
cpu_registerfatal:
#endif
    bug("[Kernel:%02d] Waiting for interrupts\n", cpunum);

    KrnSpinUnLock(&startup_lock);

#if !defined(__AROSEXEC_SMP__)
    do {
#endif
    asm volatile("wfi");
#if !defined(__AROSEXEC_SMP__)
    } while (1);
#else
    /* Switch to the task level (EL1t) and load the bootstrap task stack.
     * AROS runs privileged at EL1; tasks run at EL1t (SP_EL0), not EL0. */
    bug("[Kernel:%02d] Continuing at EL1t on the task stack ... \n", cpunum);

    uint64_t bs_stack = (uint64_t)__tls->ThisTask->tc_SPUpper;
    asm volatile(
        "msr    sp_el0, %[bs_stack]  \n"
        "adr    x0, 1f               \n"
        "msr    elr_el1, x0          \n"
        "mov    x0, #0x4             \n"  /* SPSR: EL1t (SPSel=0, SP_EL0) */
        "msr    spsr_el1, x0         \n"
        "eret                        \n"
        "1:                          \n"
        : : [bs_stack] "r" (bs_stack)
        : "x0", "memory"
    );

    /* We now start up the interrupts */
    Permit();
    Enable();
#endif
}

void cpu_Delay(int usecs)
{
    unsigned int delay;
    for (delay = 0; delay < usecs; delay++) asm volatile ("nop\n");
}

/*
 * AArch64 FP/NEON register save/restore.
 * V0-V31 are 128-bit, stored as pairs of 64-bit values.
 */
asm(
"       .globl cpu_Save_FP_State                \n"
"       .type cpu_Save_FP_State,%function       \n"
"cpu_Save_FP_State:                             \n"
"           mrs     x1, fpcr                    \n"
"           mrs     x2, fpsr                    \n"
"           str     x1, [x0, #512]             \n"
"           str     x2, [x0, #520]             \n"
"           stp     q0, q1, [x0, #0]           \n"
"           stp     q2, q3, [x0, #32]          \n"
"           stp     q4, q5, [x0, #64]          \n"
"           stp     q6, q7, [x0, #96]          \n"
"           stp     q8, q9, [x0, #128]         \n"
"           stp     q10, q11, [x0, #160]       \n"
"           stp     q12, q13, [x0, #192]       \n"
"           stp     q14, q15, [x0, #224]       \n"
"           stp     q16, q17, [x0, #256]       \n"
"           stp     q18, q19, [x0, #288]       \n"
"           stp     q20, q21, [x0, #320]       \n"
"           stp     q22, q23, [x0, #352]       \n"
"           stp     q24, q25, [x0, #384]       \n"
"           stp     q26, q27, [x0, #416]       \n"
"           stp     q28, q29, [x0, #448]       \n"
"           stp     q30, q31, [x0, #480]       \n"
"           ret                                 \n"

"       .globl cpu_Restore_FP_State             \n"
"       .type cpu_Restore_FP_State,%function    \n"
"cpu_Restore_FP_State:                          \n"
"           ldr     x1, [x0, #512]             \n"
"           ldr     x2, [x0, #520]             \n"
"           msr     fpcr, x1                    \n"
"           msr     fpsr, x2                    \n"
"           ldp     q0, q1, [x0, #0]           \n"
"           ldp     q2, q3, [x0, #32]          \n"
"           ldp     q4, q5, [x0, #64]          \n"
"           ldp     q6, q7, [x0, #96]          \n"
"           ldp     q8, q9, [x0, #128]         \n"
"           ldp     q10, q11, [x0, #160]       \n"
"           ldp     q12, q13, [x0, #192]       \n"
"           ldp     q14, q15, [x0, #224]       \n"
"           ldp     q16, q17, [x0, #256]       \n"
"           ldp     q18, q19, [x0, #288]       \n"
"           ldp     q20, q21, [x0, #320]       \n"
"           ldp     q22, q23, [x0, #352]       \n"
"           ldp     q24, q25, [x0, #384]       \n"
"           ldp     q26, q27, [x0, #416]       \n"
"           ldp     q28, q29, [x0, #448]       \n"
"           ldp     q30, q31, [x0, #480]       \n"
"           ret                                 \n"
);

extern void cpu_Save_FP_State(void *buffer);
extern void cpu_Restore_FP_State(void *buffer);

void cpu_Init_VFP_State(void *buffer)
{
    bzero(buffer, sizeof(struct VFPContext));
}

void cpu_Probe(struct ARM_Implementation *krnARMImpl)
{
    uint64_t midr;

    asm volatile("mrs %0, midr_el1" : "=r"(midr));

    /* AArch64 is always ARMv8+ (family 8) */
    krnARMImpl->ARMI_Family = 8;

    krnARMImpl->ARMI_Save_VFP_State = &cpu_Save_FP_State;
    krnARMImpl->ARMI_Restore_VFP_State = &cpu_Restore_FP_State;

#if defined(__AROSEXEC_SMP__)
    {
        uint64_t mpidr;
        asm volatile("mrs %0, mpidr_el1" : "=r"(mpidr));
        /* Check for multiprocessor system */
    }
#endif

    krnARMImpl->ARMI_Init_VFP_State = &cpu_Init_VFP_State;
    krnARMImpl->ARMI_Delay = &cpu_Delay;
}

void cpu_Init(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    cpuid_t cpunum = GetCPUNumber();

    core_SetupMMU(msg);

    __arm_arosintern.ARMI_AffinityMask |= (1 << cpunum);

    /* Enable FP/NEON access via CPACR_EL1 */
    {
        uint64_t cpacr;
        asm volatile("mrs %0, cpacr_el1" : "=r"(cpacr));
        cpacr |= (3 << 20);  /* FPEN = 0b11 (no trapping of FP/NEON) */
        asm volatile("msr cpacr_el1, %0" : : "r"(cpacr));
        asm volatile("isb");
    }
}

void cpu_Switch(regs_t *regs)
{
    struct Task *task;
    UQUAD timeCur;
    struct timespec timeSpec;

    DSCHED(
        cpuid_t cpunum = GetCPUNumber();
        bug("[Kernel:%02d] cpu_Switch()\n", cpunum);
    )

    task = GET_THIS_TASK;

    /* Cache running task's context */
    STORE_TASKSTATE(task, regs)

    if (__arm_arosintern.ARMI_GetTime)
    {
        /* Update the task's CPU time */
        timeCur = __arm_arosintern.ARMI_GetTime() - IntETask(task->tc_UnionETask.tc_ETask)->iet_private1;
        timeSpec.tv_sec = timeCur / 1000000000;
        timeSpec.tv_nsec = timeCur % 1000000000;

        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec += timeSpec.tv_nsec;
        IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec  += timeSpec.tv_sec;
        while (IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec >= 1000000000)
        {
            IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec -= 1000000000;
            IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_sec++;
        }
    }

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;
#if defined(__AROSEXEC_SMP__)
    cpuid_t cpunum = GetCPUNumber();
    DSCHED(
        bug("[Kernel:%02d] cpu_Dispatch()\n", cpunum);
    )
#else
    DSCHED(
        cpuid_t cpunum = GetCPUNumber();
        bug("[Kernel:%02d] cpu_Dispatch()\n", cpunum);
    )
#endif

    while (!(task = core_Dispatch()))
    {
        DSCHED(bug("[Kernel:%02d] cpu_Dispatch: Nothing to run - idling\n", cpunum));
        asm volatile("wfi");
    }

    DSCHED(bug("[Kernel:%02d] cpu_Dispatch: 0x%p [R  ] '%s'\n", cpunum, task, task->tc_Node.ln_Name));

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    DREGS(cpu_DumpRegs(regs));

    /* Handle task's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

#if defined(__AROSEXEC_SMP__)
    IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuNumber = cpunum;
#endif

    if (__arm_arosintern.ARMI_GetTime)
    {
        /* Store the launch time */
        IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 = __arm_arosintern.ARMI_GetTime();
        if (!IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_sec && !IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_nsec)
        {
            IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_sec = IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 / 1000000;
            IntETask(task->tc_UnionETask.tc_ETask)->iet_StartTime.tv_nsec = (IntETask(task->tc_UnionETask.tc_ETask)->iet_private1 % 1000000) * 1000;
        }
    }

    if (task->tc_Flags & TF_LAUNCH)
    {
        AROS_UFC1(void, task->tc_Launch,
                  AROS_UFCA(struct ExecBase *, SysBase, A6));
    }
}

void cpu_DumpRegs(regs_t *regs)
{
    cpuid_t cpunum = GetCPUNumber();
    int i;

    bug("[Kernel:%02d] AArch64 CPU Register Dump:\n", cpunum);
    for (i = 0; i < 29; i++)
    {
        bug("[Kernel:%02d]      x%02d: 0x%016lx\n", cpunum, i, regs->x[i]);
    }
    bug("[Kernel:%02d] (fp) x29: 0x%016lx\n", cpunum, regs->fp);
    bug("[Kernel:%02d] (lr) x30: 0x%016lx\n", cpunum, regs->lr);
    bug("[Kernel:%02d] (sp)    : 0x%016lx\n", cpunum, regs->sp);
    bug("[Kernel:%02d] (pc)    : 0x%016lx\n", cpunum, regs->pc);
    bug("[Kernel:%02d]  spsr   : 0x%08x\n", cpunum, regs->cpsr);
}
