/*
    Copyright � 2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/rawfmt.h>
#include <proto/exec.h>
#include <stdarg.h>
#include <aros/symbolsets.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

static VOID  __sprintf(UBYTE *buffer, const UBYTE *format, ...)
{
    va_list args;

    va_start(args, format);
    VNewRawDoFmt(format, RAWFMTFUNC_STRING, buffer, args);
    va_end(args);
}

static VOID ReadProcessorInformation(struct M68KProcessorInformation * info)
{
   
    __sprintf(info->ModelStringBuffer, "%s", "Unknown");
    info->ModelString = info->ModelStringBuffer;
    
    info->CPUModel = CPUMODEL_UNKNOWN;
    info->FPUModel = FPUMODEL_UNKNOWN;
    info->L1InstructionCacheSize = 0;
    info->L1DataCacheSize = 0;

    if (SysBase->AttnFlags & AFF_68060)
    {
        info->CPUModel = CPUMODEL_68060;
        __sprintf(info->ModelStringBuffer, "%s", "68060");
        info->L1InstructionCacheSize = 8192;
        info->L1DataCacheSize = 8192;
    }
    else if (SysBase->AttnFlags & AFF_68040)
    {
        info->CPUModel = CPUMODEL_68040;
        __sprintf(info->ModelStringBuffer, "%s", "68040");
        info->L1InstructionCacheSize = 4096;
        info->L1DataCacheSize = 4096;
    }
    else if (SysBase->AttnFlags & AFF_68030)
    {
        info->CPUModel = CPUMODEL_68030;
        __sprintf(info->ModelStringBuffer, "%s", "68030");
        info->L1InstructionCacheSize = 256;
        info->L1DataCacheSize = 256;
    }
    else if (SysBase->AttnFlags & AFF_68020)
    {
        info->CPUModel = CPUMODEL_68020;
        __sprintf(info->ModelStringBuffer, "%s", "68020");
        info->L1InstructionCacheSize = 256;
    }
    else if (SysBase->AttnFlags & AFF_68010)
    {
        info->CPUModel = CPUMODEL_68010;
        __sprintf(info->ModelStringBuffer, "%s", "68010");
    }
    else
    {
        info->CPUModel = CPUMODEL_68000;
        __sprintf(info->ModelStringBuffer, "%s", "68000");
    }

    if (SysBase->AttnFlags & AFF_FPU40) {
        if (SysBase->AttnFlags & AFF_68060)
        {
            info->FPUModel = FPUMODEL_INTERNAL;
            __sprintf(info->ModelStringBuffer + 5, "%s", " + FPU");
        }
        else if (SysBase->AttnFlags & AFF_68040)
        {
            info->FPUModel = FPUMODEL_INTERNAL;
            __sprintf(info->ModelStringBuffer + 5, "%s", " + FPU");
        }
        else
            info->FPUModel = FPUMODEL_NONE;
    } 
    else if (SysBase->AttnFlags & AFF_68882)
    {
        info->FPUModel = FPUMODEL_68882;
        __sprintf(info->ModelStringBuffer + 5, "%s", " + 68882");
    }
    else if (SysBase->AttnFlags & AFF_68881)
    {
        info->FPUModel = FPUMODEL_68881;
        __sprintf(info->ModelStringBuffer + 5, "%s", " + 68881");
    }
    else
        info->FPUModel = FPUMODEL_NONE;

    info->CPUFrequency = 0; /* TODO: Implement */
}

LONG Processor_Init(struct ProcessorBase * ProcessorBase)
{
    struct SystemProcessors * sysprocs = 
        AllocVec(sizeof(struct SystemProcessors), MEMF_ANY | MEMF_CLEAR);

    if (sysprocs == NULL)
        return FALSE;

    ProcessorBase->Private1 = sysprocs;

    ReadProcessorInformation(&sysprocs->processor);
    
    return TRUE;
}

ADD2INITLIB(Processor_Init, 1);
