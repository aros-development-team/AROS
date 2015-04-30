/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec_platform.h>

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <aros/arm/cpucontext.h>
#include <strings.h>

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

#define D(x)
#define DREGS(x)

extern struct Task *sysIdleTask;
uint32_t __arm_affinitymask __attribute__((section(".data"))) = 1;

extern BOOL Exec_InitETask(struct Task *, struct ExecBase *);

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
"       .globl mpcore_end                       \n"
"mpcore_end:  "
);

void cpu_Register()
{
    uint32_t tmp;
#if defined(__AROSEXEC_SMP__)
    tls_t *__tls;
    struct ExecBase *SysBase;
    struct KernelBase *KernelBase;
    struct Task *t;
    struct MemList *ml;
    struct ExceptionContext *ctx;
#endif

    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);    /* I and D caches, branch prediction */
    tmp = (tmp & ~2) | (1 << 22);               /* Unaligned access enable */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));

    cpu_Init(&__arm_arosintern, NULL);

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));
    
#if defined(__AROSEXEC_SMP__)
    asm volatile (" mrc p15, 0, %0, c13, c0, 3 " : "=r" (__tls));

    /* Now we are ready to boostrap and launch the schedular */
    bug("[KRN] Core %d Boostrapping..\n", (tmp & 0x3));
    bug("[KRN] Core %d TLS @ 0x%p\n", (tmp & 0x3), (__tls));
    KernelBase = __tls->KernelBase; // TLS_GET(KernelBase)
    SysBase = __tls->SysBase; // TLS_GET(SysBase)
    bug("[KRN] Core %d KernelBase @ 0x%p\n", (tmp & 0x3), KernelBase);
    bug("[KRN] Core %d SysBase @ 0x%p\n", (tmp & 0x3), SysBase);

    t   = AllocMem(sizeof(struct Task),    MEMF_PUBLIC|MEMF_CLEAR);
    ml  = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR);

    if (!t || !ml)
    {
        bug("[KRN] Core %d FATAL : Failed to allocate memory for bootstrap task!", (tmp & 0x3));
        goto cpu_registerfatal;
    }

    bug("[KRN] Core %d Bootstrap task @ 0x%p\n", (tmp & 0x3), t);
    bug("[KRN] Core %d cpu context size %d\n", (tmp & 0x3), KernelBase->kb_ContextSize);

    ctx = KrnCreateContext();
    if (!ctx)
    {
        bug("[KRN] Core %d FATAL : Failed to create the boostrap task context!\n", (tmp & 0x3));
        goto cpu_registerfatal;
    }

    bug("[KRN] Core %d cpu ctx @ 0x%p\n", (tmp & 0x3), ctx);

    NEWLIST(&t->tc_MemEntry);

    t->tc_Node.ln_Name = AllocVec(20, MEMF_CLEAR);
    sprintf( t->tc_Node.ln_Name, "Core(%d) Bootstrap", (tmp & 0x3));
    t->tc_Node.ln_Type = NT_TASK;
    t->tc_Node.ln_Pri  = 0;
    t->tc_State        = TS_RUN;
    t->tc_SigAlloc     = 0xFFFF;

    /* Build bootstraps memory list */
    ml->ml_NumEntries      = 1;
    ml->ml_ME[0].me_Addr   = t;
    ml->ml_ME[0].me_Length = sizeof(struct Task);
    AddHead(&t->tc_MemEntry, &ml->ml_Node);

    /* Create a ETask structure and attach CPU context */
    if (!Exec_InitETask(t, SysBase))
    {
        bug("[KRN] Core %d FATAL : Failed to allocate memory for boostrap extended data!\n", (tmp & 0x3));
        goto cpu_registerfatal;
    }
    t->tc_UnionETask.tc_ETask->et_RegFrame = ctx;

    /* This Bootstrap task can run only on one of the available cores */
    IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuNumber = (tmp & 0x3);
    IntETask(t->tc_UnionETask.tc_ETask)->iet_CpuAffinity = 1 << (tmp & 0x3);

    __tls->ThisTask = t;

    if (__arm_arosintern.ARMI_InitCore)
        __arm_arosintern.ARMI_InitCore();

#endif

    bug("[KRN] Core %d operational\n", (tmp & 0x3));

//      amlock = KrnSpinLock(amlock, 0);    
    __arm_affinitymask |= (1 << (tmp & 0x3));
//      KrnSpinUnLock(amlock);

cpu_registerfatal:

    bug("[KRN] Core %d waiting for interrupts\n", (tmp & 0x3));

    for (;;) asm volatile("wfi");
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
    if ((tmp & 0xfff0) == 0xc070)
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

    core_SetupMMU(msg);

    if (msg)
    {
        /* Only boot processor calls cpu_Init with a valid msg */
        
    }

    /* Enable Vector Floating Point Calculations */
    asm volatile("mrc p15,0,%[fpuflags],c1,c0,2\n" : [fpuflags] "=r" (fpuflags));   // Read Access Control Register 
    fpuflags |= (VFPSingle | VFPDouble);                                            // Enable Single & Double Precision 
    asm volatile("mcr p15,0,%[fpuflags],c1,c0,2\n" : : [fpuflags] "r" (fpuflags)); // Set Access Control Register
    asm volatile(
        "       mov %[fpuflags],%[vfpenable]    \n"                                 // Enable VFP 
        "       fmxr fpexc,%[fpuflags]          \n"
         : [fpuflags] "=r" (fpuflags) : [vfpenable] "I" (VFPEnable));
}

#define ADDTIME(dest, src)			\
    (dest)->tv_micro += (src)->tv_micro;	\
    (dest)->tv_secs  += (src)->tv_secs;		\
    while((dest)->tv_micro > 999999)		\
    {						\
	(dest)->tv_secs++;			\
	(dest)->tv_micro -= 1000000;		\
    }

void cpu_Switch(regs_t *regs)
{
    struct Task *task;
    UQUAD timeCur;
    struct timeval timeVal;

    D(bug("[Kernel] cpu_Switch()\n"));

    task = GET_THIS_TASK;

    /* Copy current task's context into the ETask structure */
    /* Restore the task's state */
    STORE_TASKSTATE(task, regs)

    if (__arm_arosintern.ARMI_GetTime)
    {
        /* Update the taks CPU time .. */
        timeCur = __arm_arosintern.ARMI_GetTime() - GetIntETask(task)->iet_private1;
        timeVal.tv_secs = timeCur / 1000000;
        timeVal.tv_micro = timeCur % 1000000;

        ADDTIME(&GetIntETask(task)->iet_CpuTime, &timeVal);
    }

    core_Switch();
}

void cpu_Dispatch(regs_t *regs)
{
    struct Task *task;

    D(bug("[Kernel] cpu_Dispatch()\n"));

    /* Break Disable() if needed */
    if (SysBase->IDNestCnt >= 0) {
        SysBase->IDNestCnt = -1;
        ((uint32_t *)regs)[13] &= ~0x80;
    }

    if (!(task = core_Dispatch()))
        task = sysIdleTask;

    D(bug("[Kernel] cpu_Dispatch: Letting '%s' run for a bit..\n", task->tc_Node.ln_Name));

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    DREGS(cpu_DumpRegs(regs));

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

    if (__arm_arosintern.ARMI_GetTime)
    {
        /* Store the launch time */
        GetIntETask(task)->iet_private1 = __arm_arosintern.ARMI_GetTime();
        if (!GetIntETask(task)->iet_StartTime.tv_secs && !GetIntETask(task)->iet_StartTime.tv_micro)
        {
            GetIntETask(task)->iet_StartTime.tv_secs = GetIntETask(task)->iet_private1 / 1000000;
            GetIntETask(task)->iet_StartTime.tv_micro = GetIntETask(task)->iet_private1 % 1000000;
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
    int i;
    
    bug("[KRN] Register Dump:\n");
    for (i = 0; i < 12; i++)
    {
        bug("[KRN]      r%02d: 0x%08x\n", i, ((uint32_t *)regs)[i]);
    }
    bug("[KRN] (ip) r12: 0x%08x\n", ((uint32_t *)regs)[12]);
    bug("[KRN] (sp) r13: 0x%08x\n", ((uint32_t *)regs)[13]);
    bug("[KRN] (lr) r14: 0x%08x\n", ((uint32_t *)regs)[14]);
    bug("[KRN] (pc) r15: 0x%08x\n", ((uint32_t *)regs)[15]);
    bug("[KRN]     cpsr: 0x%08x\n", ((uint32_t *)regs)[16]);
}
