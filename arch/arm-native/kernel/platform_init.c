/*
    Copyright (C) 2013-2015, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <asm/io.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <strings.h>

#include <defines/exec_LVO.h>

#include "etask.h"

#include "kernel_intern.h"
#include "kernel_arch.h"
#include "kernel_romtags.h"

THIS_PROGRAM_HANDLES_SYMBOLSET(ARMPLATFORMS);
DECLARESET(ARMPLATFORMS);

void platform_Init(struct ARM_Implementation *krnARMImpl, struct TagItem *msg)
{
    IPTR (*platprobe) (struct ARM_Implementation *, struct TagItem *);
    int cur;

    for ( cur = 1; (void *)(SETNAME(ARMPLATFORMS))[cur] != NULL; cur++)
    {
        platprobe = (void *)(SETNAME(ARMPLATFORMS))[cur];
        if (platprobe(krnARMImpl, msg))
        {
            if (krnARMImpl->ARMI_LED_Toggle)
                krnARMImpl->ARMI_LED_Toggle(ARM_LED_POWER, ARM_LED_ON);

            break;
        }
    }

    return;
}

void *(*__AllocMem)();

#define ExecAllocMem(bytesize, requirements)        \
AROS_CALL2(void *, __AllocMem,                      \
                AROS_LCA(ULONG, byteSize,     D0),                      \
                AROS_LCA(ULONG, requirements, D1),                      \
                struct ExecBase *, SysBase)

AROS_LH2(APTR, AllocMem,
        AROS_LHA(ULONG, byteSize,     D0),
        AROS_LHA(ULONG, requirements, D1),
        struct ExecBase *, SysBase, 33, Kernel)
{
    AROS_LIBFUNC_INIT

    if (requirements & MEMF_CHIP)
    {
        DALLOCMEM(bug("[Kernel] AllocMem: Ignoring MEMF_CHIP flag\n"));
        requirements &= ~MEMF_CHIP;
    }
    return ExecAllocMem(bytesize, requirements);

    AROS_LIBFUNC_EXIT
}

static int platform_PostInit(struct KernelBase *KernelBase)
{
    D(bug("[Kernel] platform_PostInit()\n"));

    D(bug("[Kernel] platform_PostInit: Patching in our AllocMem to ignore MEMF_CHIP..\n"));
    
    __AllocMem = SetFunction((struct Library *)SysBase, -LVOAllocMem*LIB_VECTSIZE, AROS_SLIB_ENTRY(AllocMem, Kernel, LVOAllocMem));

    D(bug("[Kernel] platform_PostInit: Performing Post Init..\n"));

#if defined(__AROSEXEC_SMP__)
    /* Pin the boot task to CPU 0 before ARMI_Init wakes the secondaries.
     * Exec_ARMCPUSMPInit does this too, but runs after us - until then
     * affinity is NULL ("run anywhere") and a secondary could dispatch
     * the boot task mid-coldstart, which is not migration-safe. */
    {
        struct Task *bootTask = FindTask(NULL);
        struct IntETask *iet = bootTask ? (struct IntETask *)GetETask(bootTask) : NULL;

        if (iet && !iet->iet_CpuAffinity)
        {
            void *bootAff = KrnAllocCPUMask();
            if (bootAff)
            {
                KrnGetCPUMask(0, bootAff);
                iet->iet_CpuAffinity = bootAff;
                iet->iet_CpuNumber = 0;
            }
        }
    }
#endif

    if (__arm_arosintern.ARMI_Init)
        __arm_arosintern.ARMI_Init(KernelBase, SysBase);
    
    D(bug("[Kernel] platform_PostInit: Registering Heartbeat timer..\n"));

    KrnAddSysTimerHandler(KernelBase);

#if defined(__AROSEXEC_SMP__)
    /* Every core expires its quantum from its own CNTP heartbeat, not
     * the VBlank. Arm the boot CPU here; secondaries do it in
     * cpu_Register. */
    if (__arm_arosintern.ARMI_InitTimerCore)
        __arm_arosintern.ARMI_InitTimerCore();
#endif

    D(bug("[Kernel] platform_PostInit: Done..\n"));

    return TRUE;
}

ADD2INITLIB(platform_PostInit, 10)

struct KernelBase *getKernelBase()
{
    return (struct KernelBase *)KernelBase;
}
