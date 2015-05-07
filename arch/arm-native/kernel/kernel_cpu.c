/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>
#include <aros/arm/cpucontext.h>
#include <asm/arm/cpu.h>
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

#define D(x)
#define DREGS(x)

uint32_t        __arm_affinitymask __attribute__((section(".data"))) = 1;
spinlock_t      __arm_affinitymasklock;

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
    uint32_t tmp, ttmp;
#if defined(__AROSEXEC_SMP__)
    tls_t *__tls;
    struct ExecBase *SysBase;
    struct KernelBase *KernelBase;
#endif

    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);    /* I and D caches, branch prediction */
    tmp = (tmp & ~2) | (1 << 22);               /* Unaligned access enable */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));

    cpu_Init(&__arm_arosintern, NULL);

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));
    
#if defined(__AROSEXEC_SMP__)
    __tls = TLS_PTR_GET();

    /* Now we are ready to boostrap and launch the schedular */
    bug("[KRN] Core %d Boostrapping..\n", (tmp & 0x3));

    asm volatile ("mrs %0, cpsr" :"=r"(ttmp));
    bug("[KRN] Core %d CPSR=%08x\n", (tmp & 0x3), ttmp);
    ttmp &= ~(1 << 6);
    asm volatile ("msr cpsr_cxsf, %0" ::"r"(ttmp));
    bug("[KRN] Core %d CPSR=%08x\n", (tmp & 0x3), ttmp);

    bug("[KRN] Core %d TLS @ 0x%p\n", (tmp & 0x3), (__tls));
    KernelBase = (struct KernelBase *)__tls->KernelBase; // TLS_GET(KernelBase)
    SysBase = (struct ExecBase *)__tls->SysBase; // TLS_GET(SysBase)
    bug("[KRN] Core %d KernelBase @ 0x%p\n", (tmp & 0x3), KernelBase);
    bug("[KRN] Core %d SysBase @ 0x%p\n", (tmp & 0x3), SysBase);

    if ((__tls->ThisTask = cpu_InitBootStrap(SysBase)) == NULL)
        goto cpu_registerfatal;

    if (__arm_arosintern.ARMI_InitCore)
        __arm_arosintern.ARMI_InitCore(KernelBase, SysBase);

    cpu_BootStrap(__tls->ThisTask, SysBase);
#endif

    bug("[KRN] Core %d operational\n", (tmp & 0x3));

    KrnSpinLock(&__arm_affinitymasklock, SPINLOCK_MODE_WRITE);
    __arm_affinitymask |= (1 << (tmp & 0x3));
    KrnSpinUnLock(&__arm_affinitymasklock);

cpu_registerfatal:

    bug("[KRN] Core %d waiting for interrupts\n", (tmp & 0x3));

    KrnSpinUnLock(&startup_lock);

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

    __arm_affinitymasklock = (spinlock_t)SPINLOCK_INIT_UNLOCKED;
    __arm_affinitymask = 1;

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
            __arm_affinitymask = 1 << (tmp & 3);
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
#if defined(__AROSEXEC_SMP__) || defined(DEBUG)
    int cpunum = GetCPUNumber();
#endif

    struct Task *task;

    D(bug("[Kernel] cpu_Dispatch(%02d)\n", cpunum));

    /* Break Disable() if needed */
    if (SysBase->IDNestCnt >= 0) {
        SysBase->IDNestCnt = -1;
        ((uint32_t *)regs)[13] &= ~0x80;
    }

    if (!(task = core_Dispatch()))
    {
        task = TLS_GET(IdleTask);
    }

    D(bug("[Kernel] cpu_Dispatch[%02d]: 0x%p [R  ] '%s'\n", cpunum, task, task->tc_Node.ln_Name));

    /* Restore the task's state */
    RESTORE_TASKSTATE(task, regs)

    DREGS(cpu_DumpRegs(regs));

    /* Handle tasks's flags */
    if (task->tc_Flags & TF_EXCEPT)
        Exception();

#if defined(__AROSEXEC_SMP__)
    GetIntETask(task)->iet_CpuNumber = cpunum;
#endif

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
    int cpunum = GetCPUNumber();
    int i;
    
    bug("[KRN][%02d] Register Dump:\n", cpunum);
    for (i = 0; i < 12; i++)
    {
        bug("[KRN][%02d]      r%02d: 0x%08x\n", cpunum, i, ((uint32_t *)regs)[i]);
    }
    bug("[KRN][%02d] (ip) r12: 0x%08x\n", cpunum, ((uint32_t *)regs)[12]);
    bug("[KRN][%02d] (sp) r13: 0x%08x\n", cpunum, ((uint32_t *)regs)[13]);
    bug("[KRN][%02d] (lr) r14: 0x%08x\n", cpunum, ((uint32_t *)regs)[14]);
    bug("[KRN][%02d] (pc) r15: 0x%08x\n", cpunum, ((uint32_t *)regs)[15]);
    bug("[KRN][%02d]     cpsr: 0x%08x\n", cpunum, ((uint32_t *)regs)[16]);
}
