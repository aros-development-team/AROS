/*
    Copyright © 2013-2015, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/kernel.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include <proto/kernel.h>

#include <strings.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_debug.h"
#include "kernel_cpu.h"
#include "kernel_syscall.h"
#include "kernel_scheduler.h"
#include "kernel_intr.h"

#define D(x)
#define DREGS(x)

extern struct Task *sysIdleTask;
uint32_t __arm_affinitymask __attribute__((section(".data"))) = 1;

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
"               ldr     sp, mpcore_data         \n"
"               ldr     pc, mpcore_code         \n"

"mpcore_pde:    .word   0                       \n"
"mpcore_code:   .word   0                       \n"
"mpcore_data:   .word   0                       \n"
"mpcore_end:  "
);

extern mpcore_trampoline();
extern uint32_t mpcore_end;
extern uint32_t mpcore_pde;

void cpu_Register()
{
    uint32_t tmp;

    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r"(tmp));
    tmp |= (1 << 2) | (1 << 12) | (1 << 11);    /* I and D caches, branch prediction */
    tmp = (tmp & ~2) | (1 << 22);               /* Unaligned access enable */
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(tmp));

    cpu_Init(&__arm_arosintern, NULL);

    asm volatile (" mrc p15, 0, %0, c0, c0, 5 " : "=r" (tmp));

    __arm_affinitymask |= (1 << (tmp & 0x3));

    bug("[KRN] Core %d up and waiting for interrupts\n", tmp & 0x3);

    for (;;) asm volatile("wfi");
}

void cpu_Delay(int usecs)
{
    unsigned int delay;
    for (delay = 0; delay < usecs; delay++) asm volatile ("mov r0, r0\n");
}

void arm_flush_cache(uint32_t addr, uint32_t length);

uint32_t tmp_stacks_smp[4*1024];

void cpu_Probe(struct ARM_Implementation *krnARMImpl)
{
    uint32_t tmp;

    asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (tmp));
    if ((tmp & 0xfff0) == 0xc070)
    {
        krnARMImpl->ARMI_Family = 7;

        if (krnARMImpl->ARMI_Delay)
        {
        // Read the Multiprocessor Affinity Register (MPIDR)
        asm volatile ("mrc p15, 0, %0, c0, c0, 5" : "=r" (tmp));

        if (tmp & (2 << 30))
        {
            void *trampoline_src = mpcore_trampoline;
            void *trampoline_dst = (void *)0x2000;
            uint32_t trampoline_length = (uintptr_t)&mpcore_end - (uintptr_t)mpcore_trampoline;
            uint32_t trampoline_data_offset = (uintptr_t)&mpcore_pde - (uintptr_t)mpcore_trampoline;

            bug("[KRN] Multicore system\n");

            bug("[KRN] Copy SMP trampoline from %p to %p (%d bytes)\n", trampoline_src, trampoline_dst, trampoline_length);
            bcopy(trampoline_src, trampoline_dst, trampoline_length);

            bug("[KRN] Patching data for trampoline at offset %d\n", trampoline_data_offset);
            asm volatile ("mrc p15, 0, %0, c2, c0, 0":"=r"(tmp));
            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[0] = tmp; // pde
            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[1] = (uint32_t)cpu_Register;

            bug("[KRN] Waking up cores\n");

            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = &tmp_stacks_smp[4*1024-16];
            arm_flush_cache((uint32_t)trampoline_dst, 512);
            *((uint32_t *)(0x4000008c + 0x10)) = trampoline_dst;
            cpu_Delay(10000000);

            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = &tmp_stacks_smp[3*1024-16];
            arm_flush_cache((uint32_t)trampoline_dst, 512);
            *((uint32_t *)(0x4000008c + 0x20)) = trampoline_dst;

            cpu_Delay(10000000);

            ((uint32_t *)(trampoline_dst + trampoline_data_offset))[2] = &tmp_stacks_smp[2*1024-16];
            arm_flush_cache((uint32_t)trampoline_dst, 512);
            *((uint32_t *)(0x4000008c + 0x30)) = trampoline_dst;
            cpu_Delay(10000000);

        }
        }
    }
    else
        krnARMImpl->ARMI_Family = 6;

    krnARMImpl->ARMI_Delay = &cpu_Delay;
}

void cpu_Init(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    register unsigned int fpuflags;

    core_SetupMMU(msg);

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

    D(bug("[Kernel] cpu_Switch()\n"));

    task = SysBase->ThisTask;
        
    /* Copy current task's context into the ETask structure */
    /* Restore the task's state */
    STORE_TASKSTATE(task, regs)

    if (__arm_arosintern.ARMI_GetTime)
    {
        /* Update the taks CPU time .. */
        GetIntETask(task)->iet_CpuTime += __arm_arosintern.ARMI_GetTime() - GetIntETask(task)->iet_private1;
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
