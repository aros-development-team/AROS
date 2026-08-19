/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
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
"               mrs     r4, cpsr                \n" /* Check if in hypervisor mode */
"               and     r4, r4, #0x1f           \n" /* In that case try to leave it */
"               mov     r8, #0x1a               \n"
"               cmp     r4, r8                  \n"
"               beq     leave_hyper             \n"
"mpcore_continue_boot:                          \n"
"               cps     #0x13                   \n"
"               mrc     p15,0,r4,c1,c0,2        \n" /* Enable signle and double VFP coprocessors */
"               orr     r4, r4, #0x00f00000     \n" /* This is necessary since gcc might want to use vfp registers  */
"               mcr     p15,0,r4,c1,c0,2        \n" /* Either as cache for general purpose regs or e.g. for division. This is the case with gcc9 */
"               mov     r4,#0x40000000          \n"
"               fmxr    fpexc,r4                \n" /* Enable VFP now */
#if AROS_BIG_ENDIAN
"               setend  be                      \n" /* If AROS is big endian set the endianess of cpu here */
#endif
"               ldr     r3, mpcore_pde          \n" /* MMU table */
"               orr     r3, r3, #0x4a           \n" /* SMP: S | RGN_OC_WBWA | IRGN_WBWA */
"               mcr     p15, 0, r3, c2, c0, 0   \n"
"               mov     r3, #0                  \n"
"               mcr     p15, 0, r3, c2, c0, 2   \n"
"               mov     r3, #1                  \n"
"               mcr     p15, 0, r3, c3, c0, 0   \n"
/*
 * SMP enable bit lives in different registers per part:
 *   Cortex-A7  (Pi 2):  ACTLR.SMP    (bit 6, EL1)
 *   Cortex-A53 (Pi 3):  CPUECTLR.SMPEN (bit 6, EL3 only - set in leave_hyper)
 * Writing ACTLR bit 6 on A53 hits an unrelated implementation-defined
 * bit (not SMPEN), so gate this on the part number.
 */
"               mrc     p15, 0, r5, c0, c0, 0   \n" /* MIDR */
"               movw    r6, #0xfff0             \n"
"               and     r5, r5, r6              \n"
"               movw    r6, #0xc070             \n" /* Cortex-A7 part */
"               cmp     r5, r6                  \n"
"               bne     mpcore_smp_done         \n"
"               mrc     p15, 0, r3, c1, c0, 1   \n" /* ACTLR */
"               orr     r3, r3, #0x40           \n" /* SMPEN (bit 6) - enable cache snoop */
"               mcr     p15, 0, r3, c1, c0, 1   \n" /* must be set before MMU enable */
"mpcore_smp_done:                               \n"
"               mrc     p15, 0, r4, c1, c0, 0   \n"
"               mov     r3, #0                  \n"
"               mcr     p15, 0, r3, c7, c10, 4  \n" /* dsb */
"               mcr     p15, 0, r3, c7, c5, 4   \n" /* isb - serialize SMP enable before MMU enable */
"               orr     r4, r4, #0x800000       \n" /* v6 page tables */
"               orr     r4, r4, #1              \n" /* Enable MMU */
#if AROS_BIG_ENDIAN
"               orr     r4, r4, #0x2000000      \n" /* EE bit - BigEndian exceptions and BigEndian page tables */
#endif
"               mcr     p15, 0, r4, c1, c0, 0   \n"
"               mcr     p15, 0, r3, c7, c5, 4   \n"
"               cps     #0x11                   \n"
#if AROS_BIG_ENDIAN
"               setend  be                      \n" /* If AROS is big endian set the endianess of cpu here */
#endif
"               ldr     sp, mpcore_fstack       \n"
"               cps     #0x13                   \n"
"               ldr     sp, mpcore_stack        \n"
"               ldr     r3, mpcore_tls          \n"
"               mcr     p15, 0, r3, c13, c0, 3  \n"
"               ldr     pc, mpcore_code         \n"

"leave_hyper:                                   \n" /* Escape hypervisor mode forever */
/*
 * Cortex-A53 (Pi 3): set CPUECTLR.SMPEN (bit 6) while still in HYP/EL2.
 * This is the actual SMP enable on A53 - the Pi armstub sets it for the
 * boot CPU but secondaries come up here in HYP and need it set before
 * we drop to EL1 (where CPUECTLR is inaccessible). Cortex-A7 (Pi 2)
 * uses ACTLR.SMP instead and is handled in mpcore_continue_boot.
 */
"               mrc     p15, 0, r5, c0, c0, 0   \n" /* MIDR */
"               movw    r6, #0xfff0             \n"
"               and     r5, r5, r6              \n"
"               movw    r6, #0xd030             \n" /* Cortex-A53 part */
"               cmp     r5, r6                  \n"
"               bne     mpcore_hyp_eret         \n"
"               mrrc    p15, 1, r4, r5, c15     \n" /* CPUECTLR (64-bit) */
"               orr     r4, r4, #0x40           \n" /* SMPEN (bit 6) */
"               mcrr    p15, 1, r4, r5, c15     \n"
"               mcr     p15, 0, r3, c7, c5, 4   \n" /* isb */
"mpcore_hyp_eret:                               \n"
"               adr     r4, mpcore_continue_boot\n"
"               .byte   0x04,0xf3,0x2e,0xe1     \n" /* msr     ELR_hyp, r4             */
"               mrs     r4, cpsr                \n"
"               and     r4, r4, #0x1f           \n"
"               orr     r4, r4, #0x13           \n"
"               .byte   0x04,0xf3,0x6e,0xe1     \n" /* msr     SPSR_hyp, r4            */
"               .byte   0x6e,0x00,0x60,0xe1     \n" /* eret                            */ /* Exit hypervisor */

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

#if defined(__AROSEXEC_SMP__)
/* See kernel_cpu.h - filled in by exec as it creates the idle tasks. */
struct Task *arm_IdleTask[ARM_MAXCPUS];
struct arm_CPULoadData arm_CPULoad[ARM_MAXCPUS];
#endif

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

    /*
     * Arm this core's per-core scheduler heartbeat (CNTP). Must run in
     * privileged mode - CNTP_* are PL1-banked - and after the bootstrap
     * task exists, so the first FIQ tick (~20ms out) lands on a core that
     * is already schedulable. core 0 keeps the GPU system timer instead.
     */
    if (__arm_arosintern.ARMI_InitTimerCore)
        __arm_arosintern.ARMI_InitTimerCore();
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
#if AROS_BIG_ENDIAN
        "setend be\n"
#endif
        "mov sp, %[bs_stack]\n"
        : : [bs_stack] "r" (bs_stack), [mode_user] "I" (CPUMODE_USER)
        );

    /* We now start up the interrupts */
    Permit();
    Enable();

    /*
     * Become this core's idle context - do NOT return. cpu_Register was
     * reached by a jump from the trampoline, so the link register still
     * holds the firmware mailbox spin-table return address; returning here
     * drops the core back into that loop, out of the scheduler's reach.
     *
     * Mirror IdleTask: switch to a privileged mode (so WFI is permitted)
     * and enable IRQs, then sleep. A cross-CPU IPI_SCHEDULE arrives as an
     * FIQ whose exit path (handle_fiq -> core_ExitInterrupt) dispatches
     * whatever task has become ready for this core.
     */
    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_SUPERSTATE) : "lr");
    asm volatile ("swi %[swi_no]" : : [swi_no] "I" (SC_STI) : "lr");
    for (;;)
        asm volatile ("wfi");
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
"           vmrs    r3, fpscr               \n"
"           str     r3, [r0, #256]          \n"
"           vstmia  r0, {d0-d15}            \n"
"           bx      lr                      \n"

/*
 * ARMv7 VSTMIA/VLDMIA only encode up to 16 doubles per instruction
 * (imm8 <= 32). d16-d31 must be saved/restored with a second
 * VSTMIA/VLDMIA targeting the high half.
 */
"cpu_Save_VFP32_State:                      \n"
"           vmrs    r3, fpscr               \n"
"           str     r3, [r0, #256]          \n"
"           vstmia  r0, {d0-d15}            \n"
"           add     r1, r0, #128            \n"
"           vstmia  r1, {d16-d31}           \n"
"           bx      lr                      \n"

"cpu_Restore_VFP16_State:                   \n"
"           ldr     r3, [r0, #256]          \n"
"           vmsr    fpscr, r3               \n"
"           vldmia  r0, {d0-d15}            \n"
"           bx      lr                      \n"

"cpu_Restore_VFP32_State:                   \n"
"           ldr     r3, [r0, #256]          \n"
"           vmsr    fpscr, r3               \n"
"           vldmia  r0, {d0-d15}            \n"
"           add     r1, r0, #128            \n"
"           vldmia  r1, {d16-d31}           \n"
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
        uint32_t mvfr0;

        krnARMImpl->ARMI_Family = 7;

        /*
         * VFPv3+ (ARMv7+) reports register file size in MVFR0[3:0]:
         *   1 = 16 double registers (d0-d15)
         *   2 = 32 double registers (d0-d31)
         */
        asm volatile ("vmrs %0, mvfr0" : "=r" (mvfr0));
        if ((mvfr0 & 0xf) == 2)
        {
            krnARMImpl->ARMI_Save_VFP_State = &cpu_Save_VFP32_State;
            krnARMImpl->ARMI_Restore_VFP_State = &cpu_Restore_VFP32_State;
        }
        else
        {
            krnARMImpl->ARMI_Save_VFP_State = &cpu_Save_VFP16_State;
            krnARMImpl->ARMI_Restore_VFP_State = &cpu_Restore_VFP16_State;
        }

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
        /* ARMv6 / VFPv2 — d0-d15 only, no MVFR0. */
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

#if defined(__AROSEXEC_SMP__)
    /*
     * Wait() carries tc_SpinLock across KrnSwitch so no other CPU can
     * observe (TS_WAIT, on-TaskWait) and dispatch this task before its
     * context is saved. The context is now stored - release the lock
     * (see rom/exec/wait.c). Only the Wait path reaches here in TS_WAIT:
     * interrupts are masked for its whole carry window, so an IRQ/FIQ
     * entry can never find the current task in TS_WAIT.
     */
    if (task->tc_State == TS_WAIT)
        KrnSpinUnLock(&task->tc_SpinLock);
#endif

    /*
     * iet_private1 == 0 means the task never went through cpu_Dispatch:
     * the boot task and the per-core bootstrap tasks are installed with
     * SET_THIS_TASK, so their first switch has no launch time and a delta
     * would charge the whole uptime to them. Skip the slice - the next
     * dispatch stamps iet_private1 and accounting starts clean. (A task
     * legitimately dispatched at counter value 0 loses one slice per
     * ~71 minute wrap - noise.)
     */
    if (__arm_arosintern.ARMI_GetTime &&
        IntETask(task->tc_UnionETask.tc_ETask)->iet_private1)
    {
        /*
         * Update the task's CPU time. ARMI_GetTime is the platform's
         * free-running MICROsecond counter (BCM system timer, 1MHz), so
         * scale the delta to nanoseconds - without this every task's
         * accumulated CPU time came out 1000x too low. The delta is taken
         * in 32 bits so it stays correct across the counter's ~71 minute
         * wrap (a scheduling interval is milliseconds at most).
         */
        uint32_t deltaUS = (uint32_t)__arm_arosintern.ARMI_GetTime() -
                           (uint32_t)IntETask(task->tc_UnionETask.tc_ETask)->iet_private1;

        /*
         * Cumulative busy time in microseconds. task.resource derives the
         * task's CPU usage percentage from how fast this grows.
         */
        IntETask(task->tc_UnionETask.tc_ETask)->iet_private2 += deltaUS;

        timeCur = (UQUAD)deltaUS * 1000;
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
        /*
         * WFI wakes on pending interrupts even when they are masked (and
         * they ARE masked here - we are in exception context, and Wait()
         * paths arrive with FIQ off too). But waking is not enough: the
         * pending IPI/timer FIQ must be HANDLED to make a task ready, or
         * this loop spins on a latched mailbox bit forever. Open a brief
         * IRQ+FIQ window after each wake so the handler runs (the FIQ/IRQ
         * exit path never dispatches over this SVC-mode context - see
         * intr.c - it just delivers and returns), then re-mask and rescan.
         */
        asm volatile("wfi");
        asm volatile("cpsie if\n\tisb\n\tcpsid if" ::: "memory");
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
