/*
    Copyright � 2013-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/types/timespec_s.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <aros/arm/cpucontext.h>
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

asm(
"       .globl mpcore_trampoline                \n"
"       .type mpcore_trampoline,%function       \n"
"mpcore_trampoline:                             \n"
"               ldr     r3, mpcore_pde          \n"
"               mcr     p15, 0, r3, c2, c0, 0   \n"
"               mov     r3, #0                  \n"
"               mcr     p15, 0, r3, c2, c0, 2   \n"
"               mov     r3, #1                  \n"
"               mcr     p15, 0, r3, c3, c0, 0   \n"
"               mrc     p15, 0, r4, c1, c0, 0   \n"
"               mov     r3, #0                  \n"
"               mcr     p15, 0, r3, c7, c10, 4  \n"
"               orr     r4, r4, #0x800000       \n"
"               orr     r4, r4, #1              \n"
"               mcr     p15, 0, r4, c1, c0, 0   \n"
"               mcr     p15, 0, r3, c7, c5, 4   \n"
"               cps     #0x11                   \n"
"               ldr     sp, mpcore_fstack       \n"
"               cps     #0x13                   \n"
"               ldr     sp, mpcore_stack        \n"
"               ldr     r3, mpcore_tls          \n"
"               mcr     p15, 0, r3, c13, c0, 3  \n"
"               ldr     pc, mpcore_code         \n"

"       .globl mpcore_pde                       \n"
"mpcore_pde:    .word   0                       \n"
"mpcore_code:   .word   0                       \n"
"mpcore_stack:  .word   0                       \n"
"mpcore_tls:    .word   0                       \n"
"mpcore_fstack: .word   0                       \n"
"       .globl mpcore_end                       \n"
"mpcore_end:  "
);

spinlock_t startup_lock;

void cpu_Register()
{
    uint32_t tmp;
#if defined(__AROSEXEC_SMP__)
    tls_t *__tls;
    struct ExecBase *SysBase;
#endif
    struct KernelBase *KernelBase;
    cpuid_t cpunum = GetCPUNumber();

    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);                    // I and D caches, branch prediction
    tmp = (tmp & ~2) | (1 << 22);                               // Unaligned access enable
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));

    cpu_Init(&__arm_arosintern, NULL);

#if defined(__AROSEXEC_SMP__)
    __tls = TLS_PTR_GET();

    /* Now we are ready to bootstrap and launch the scheduler */
    bug("[Kernel:%02d] Bootstrapping...\n", cpunum);

    asm volatile ("mrs %0, cpsr" :"=r"(tmp));
    bug("[Kernel:%02d] CPSR=%08x\n", cpunum, tmp);
    tmp &= ~(1 << 6);
    asm volatile ("msr cpsr_cxsf, %0" ::"r"(tmp));
    bug("[Kernel:%02d] CPSR=%08x\n", cpunum, tmp);

    bug("[Kernel:%02d] TLS @ 0x%p\n", cpunum, (__tls));
    KernelBase = (struct KernelBase *)__tls->KernelBase;        // TLS_GET(KernelBase)
    SysBase = (struct ExecBase *)__tls->SysBase;                // TLS_GET(SysBase)
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

    /* switch to user mode, and load the bs task stack */
    bug("[Kernel:%02d] Dropping into USER mode ... \n", cpunum);

    uint32_t bs_stack = __tls->ThisTask->tc_SPUpper;
    asm volatile(
        "cps %[mode_user]\n"
        "mov sp, %[bs_stack]\n"
        : : [bs_stack] "r" (bs_stack), [mode_user] "I" (CPUMODE_USER)
        );

    /* We now start up the interrupts */
    Permit();
    Enable();
#endif
}

void cpu_Delay(int usecs)
{
    unsigned int delay;
    for (delay = 0; delay < usecs; delay++) asm volatile ("mov r0, r0\n");
}

void cpu_Save_VFP16_State(void *buffer);
void cpu_Save_VFP32_State(void *buffer);
void cpu_Restore_VFP16_State(void *buffer);
void cpu_Restore_VFP32_State(void *buffer);

asm(
"cpu_Save_VFP16_State:                      \n"
"           vmsr    fpscr, r3               \n"
"           str     r3, [r0, #256]          \n"
"           vstmia  r0, {d0-d15}            \n"
"           bx      lr                      \n"

"cpu_Save_VFP32_State:                      \n"
"           vmsr    fpscr, r3               \n"
"           str     r3, [r0, #256]          \n"
"           .word   0xec800b40              \n"         // vstmia  r0, {d0-d31}
"           bx      lr                      \n"

"cpu_Restore_VFP16_State:                   \n"
"           ldr     r3, [r0, #256]          \n"
"           vmrs    r3, fpscr               \n"
"           vldmia  r0, {d0-d15}            \n"
"           bx      lr                      \n"

"cpu_Restore_VFP32_State:                   \n"
"           ldr     r3, [r0, #256]          \n"
"           vmrs    r3, fpscr               \n"
"           .word   0xec900b20              \n"         // vldmia  r0, {d0-d31}
"           bx      lr                      \n"
);

void cpu_Init_VFP_State(void *buffer)
{
    bzero(buffer, sizeof(struct VFPContext));
}

void cpu_Probe(struct ARM_Implementation *krnARMImpl)
{
    uint32_t tmp;

    asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (tmp));
    if ((tmp & 0xfff0) == 0xc070 || (tmp & 0xfff0) == 0xd030)
    {
        krnARMImpl->ARMI_Family = 7;

        krnARMImpl->ARMI_Save_VFP_State = &cpu_Save_VFP16_State;
        krnARMImpl->ARMI_Restore_VFP_State = &cpu_Restore_VFP16_State;

#if defined(__AROSEXEC_SMP__)
        // Read the Multiprocessor Affinity Register (MPIDR)
        asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (tmp));

        if (tmp & (2 << 30))
        {
            //Multicore system
        }
#endif
    }
    else
    {
        krnARMImpl->ARMI_Family = 6;
        krnARMImpl->ARMI_Save_VFP_State = &cpu_Save_VFP16_State;
        krnARMImpl->ARMI_Restore_VFP_State = &cpu_Restore_VFP16_State;
    }

    krnARMImpl->ARMI_Init_VFP_State = &cpu_Init_VFP_State;
    krnARMImpl->ARMI_Delay = &cpu_Delay;
}

void cpu_Init(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    register unsigned int fpuflags;
    cpuid_t cpunum = GetCPUNumber();

    core_SetupMMU(msg);

     __arm_arosintern.ARMI_AffinityMask |= (1 << cpunum);

    /* Enable Vector Floating Point Calculations */
    asm volatile("mrc p15,0,%[fpuflags],c1,c0,2\n" : [fpuflags] "=r" (fpuflags));   // Read Access Control Register
    fpuflags |= (VFPSingle | VFPDouble);                                            // Enable Single & Double Precision
    asm volatile("mcr p15,0,%[fpuflags],c1,c0,2\n" : : [fpuflags] "r" (fpuflags)); // Set Access Control Register
    asm volatile(
        "       mov %[fpuflags],%[vfpenable]    \n"                                 // Enable VFP
        "       fmxr fpexc,%[fpuflags]          \n"
         : [fpuflags] "=r" (fpuflags) : [vfpenable] "I" (VFPEnable));
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
        while(IntETask(task->tc_UnionETask.tc_ETask)->iet_CpuTime.tv_nsec >= 1000000000)
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

#if 0
    /* Break Disable() if needed */
    if (IDNESTCOUNT_GET >= 0) {
        IDNESTCOUNT_SET(-1);
        ((uint32_t *)regs)[16] &= ~0x80;
    }
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

    /* Handle tasks's flags */
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
    /* Leave interrupt and jump to the new task */
}

void cpu_DumpRegs(regs_t *regs)
{
    cpuid_t cpunum = GetCPUNumber();
    int i;

    bug("[Kernel:%02d] CPU Register Dump:\n", cpunum);
    for (i = 0; i < 12; i++)
    {
        bug("[Kernel:%02d]      r%02d: 0x%08x\n", cpunum, i, ((uint32_t *)regs)[i]);
    }
    bug("[Kernel:%02d] (ip) r12: 0x%08x\n", cpunum, ((uint32_t *)regs)[12]);
    bug("[Kernel:%02d] (sp) r13: 0x%08x\n", cpunum, ((uint32_t *)regs)[13]);
    bug("[Kernel:%02d] (lr) r14: 0x%08x\n", cpunum, ((uint32_t *)regs)[14]);
    bug("[Kernel:%02d] (pc) r15: 0x%08x\n", cpunum, ((uint32_t *)regs)[15]);
    bug("[Kernel:%02d]     cpsr: 0x%08x\n", cpunum, ((uint32_t *)regs)[16]);
}
