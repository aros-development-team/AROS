/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/kernel.h>
#include <aros/symbolsets.h>

#include <resources/processor.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

#define DUMPINFO(a) a

#if defined(__AROSEXEC_SMP__)
static void ProbeCPUAndSignal(struct ARMProcessorInformation *info,
                              struct Task *parent, ULONG sigmask)
{
    ReadProcessorInformation(info);
    Signal(parent, sigmask);
}
#endif

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct ARMProcessorInformation **sysprocs;
    unsigned int i;
#if defined(__AROSEXEC_SMP__)
    struct Task *me = FindTask(NULL);
    ULONG waitmask = 0;
    BYTE sigbits[ProcessorBase->cpucount];

    for (i = 0; i < ProcessorBase->cpucount; i++)
        sigbits[i] = -1;
#endif

    D(bug("[processor.AArch64] :%s()\n", __PRETTY_FUNCTION__));

    sysprocs = AllocVec(ProcessorBase->cpucount * sizeof(APTR), MEMF_ANY | MEMF_CLEAR);
    if ((ProcessorBase->Private1 = sysprocs) == NULL)
        return FALSE;

    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
        sysprocs[i] = AllocMem(sizeof(struct ARMProcessorInformation), MEMF_CLEAR);
        if (!sysprocs[i])
            return FALSE;
#if defined(__AROSEXEC_SMP__)
        if (i > 0)
        {
            void *cpuMask = KrnAllocCPUMask();
            if (cpuMask)
                KrnGetCPUMask(i, cpuMask);

            sigbits[i] = AllocSignal(-1);
            if (sigbits[i] >= 0)
            {
                ULONG mask = 1UL << sigbits[i];
                waitmask |= mask;
                NewCreateTask(TASKTAG_AFFINITY      , cpuMask,
                              TASKTAG_PRI           , -127,
                              TASKTAG_PC            , ProbeCPUAndSignal,
                              TASKTAG_ARG1          , sysprocs[i],
                              TASKTAG_ARG2          , me,
                              TASKTAG_ARG3          , mask,
                              TAG_DONE);
            }
            else
            {
                /* Out of signals: probe on the current CPU. */
                ReadProcessorInformation(sysprocs[i]);
            }
        }
        else
#else
        if (i == 0)
#endif
            ReadProcessorInformation(sysprocs[i]);
    }

#if defined(__AROSEXEC_SMP__)
    if (waitmask)
        Wait(waitmask);
    for (i = 0; i < ProcessorBase->cpucount; i++)
    {
        if (sigbits[i] >= 0)
            FreeSignal(sigbits[i]);
    }
#endif

    Processor_FillTopology(ProcessorBase);

    DUMPINFO(
    bug("[processor.AArch64] Processor Details -:\n");
        for (i = 0; i < ProcessorBase->cpucount; i++)
        {
            bug("[processor.AArch64] ");
            if (ProcessorBase->cpucount > 1)
                bug("#%d ", i);
            bug("%s %s (ARM%s)\n", sysprocs[i]->Vendor,
                sysprocs[i]->BrandString ? sysprocs[i]->BrandString : (STRPTR)"Processor Core",
                sysprocs[i]->FamilyString ? sysprocs[i]->FamilyString : (CONST_STRPTR)"");

            if (sysprocs[i]->Features1 & FEATF_FPU)
                bug("[processor.AArch64]   FP%s\n",
                    (sysprocs[i]->Features1 & FEATF_FPU_HALF) ? " (half precision)" : "");
            if (sysprocs[i]->Features1 & FEATF_NEON)
                bug("[processor.AArch64]   AdvSIMD\n");
            if (sysprocs[i]->Features1 & FEATF_AES)
                bug("[processor.AArch64]   Crypto extensions\n");
            if (sysprocs[i]->Features1 & FEATF_CRC32)
                bug("[processor.AArch64]   CRC32\n");

            bug("[processor.AArch64] Cache Info:\n");
            bug("[processor.AArch64]   L1 Data   : %dKb\n", sysprocs[i]->L1DataCacheSize);
            bug("[processor.AArch64]   L1 Instr. : %dKb\n", sysprocs[i]->L1InstructionCacheSize);
            bug("[processor.AArch64]   L2        : %dKb\n", sysprocs[i]->L2CacheSize);
            bug("[processor.AArch64]   Line size : %d bytes\n", sysprocs[i]->CacheLineSize);
        }
    )

    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
