/*
    Copyright � 2011-2019, The AROS Development Team. All rights reserved.
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
    /*
     * The inline assembler doesn't like the reference to the 68060 specific
     * PCR register.  So here we put the manually assembled code into ReadPCR[]
     * and later we'll call it from Supervisor() to retrieve that PCR register
     * contents.  The assembled code is:
     *
     *     movec PCR,d0      ; 4E7A0808
     *     rte               ; 4E73
     */

    ULONG ReadPCR[2] = { 0x4E7A0808, 0x4E730000 };
    register ULONG pcr asm("d0");

    __sprintf(info->ModelStringBuffer, "%s", "MC68000");
    info->ModelString = info->ModelStringBuffer;

    info->CPUModel = CPUMODEL_68000;
    info->FPUModel = FPUMODEL_UNKNOWN;
    info->L1InstructionCacheSize = 0;
    info->L1DataCacheSize = 0;

    info->CPUFrequency = 0; /* TODO: Implement */
    pcr = 0;

    if (SysBase->AttnFlags & (AFF_68060 | AFF_68080)) {
	pcr = Supervisor(ReadPCR);
    }

    if ((SysBase->AttnFlags & AFF_68080) && ((pcr & 0xFFF00000) == 0x04400000))
    {
        UWORD vampID = *(volatile UWORD *)0xDFF3FC;      /* VREG_BOARD */
        UBYTE vampRev = ((pcr & 0x000F0000) >> 16);

        info->CPUModel = CPUMODEL_68080;
        // info->FPUModel = FPUMODEL_UNKNOWN;
        __sprintf(info->ModelStringBuffer, "Apollo Core 68080 (x%d)", (vampID & 0xFF));
        info->CPUFrequency = (vampID & 0xFF) * SysBase->PowerSupplyFrequency;

        if ((vampID >> 8) == 5)
        {
            /* v4 */
            info->L1InstructionCacheSize = (1024 << 5); /* 32 KB */
            info->L1DataCacheSize = (1024 << 6); /* 64 KB */
            //info->L1InstructionCacheSize = (1024 << 5); /* 32 KB */
            //info->L1DataCacheSize = (1024 << 7); /* 128 KB */
        }
        else
        {
            /* v1 is unsupported so we use v2 values */
            info->L1InstructionCacheSize = (1024 << 4); /* 16 KB */
            info->L1DataCacheSize = (1024 << 5); /* 32 KB */
        }
    }
    else if (SysBase->AttnFlags & AFF_68060)
    {
        int revoffs = 7;
        info->CPUModel = CPUMODEL_68060;
        if (SysBase->AttnFlags & AFF_FPU40) {
            info->FPUModel = FPUMODEL_INTERNAL;
            __sprintf(info->ModelStringBuffer, "%s", "MC68060");
        } else {
            revoffs += 2;
            if (SysBase->AttnFlags  & AFB_PRIVATEB)
            {
                __sprintf(info->ModelStringBuffer, "%s", "MC68LC060");
            }
            else
            {
                __sprintf(info->ModelStringBuffer, "%s", "MC68EC060");
            }
        }
        __sprintf(&info->ModelStringBuffer[revoffs], " (Rev. #%d)", (pcr >> 8) & 0xFF);
        info->L1InstructionCacheSize = (1024 << 3);
        info->L1DataCacheSize = (1024 << 3);
    }
    else if (SysBase->AttnFlags & AFF_68040)
    {
        info->CPUModel = CPUMODEL_68040;
        if (SysBase->AttnFlags & AFF_FPU40) {
            info->FPUModel = FPUMODEL_INTERNAL;
            __sprintf(info->ModelStringBuffer, "%s", "MC68040");
        } else {
            if (SysBase->AttnFlags  & AFB_PRIVATEB)
            {
                __sprintf(info->ModelStringBuffer, "%s", "MC68LC040");
            }
            else
            {
                __sprintf(info->ModelStringBuffer, "%s", "MC68EC040");
            }
        }
        info->L1InstructionCacheSize = (1024 << 2);
        info->L1DataCacheSize = (1024 << 2);
    }
    else if (SysBase->AttnFlags & AFF_68030)
    {
        info->CPUModel = CPUMODEL_68030;
        if (SysBase->AttnFlags  & AFB_PRIVATEB)
        {
            __sprintf(info->ModelStringBuffer, "%s", "MC68030");
        }
        else
        {
            __sprintf(info->ModelStringBuffer, "%s", "MC68EC030");
        }
        info->L1InstructionCacheSize = 256;
        info->L1DataCacheSize = 256;
    }
    else if (SysBase->AttnFlags & AFF_68020)
    {
        info->CPUModel = CPUMODEL_68020;
        if (SysBase->AttnFlags & AFF_ADDR32)
        {
            __sprintf(info->ModelStringBuffer, "%s", "MC68020");
        }
        else
        {
            __sprintf(info->ModelStringBuffer, "%s", "MC68EC020");
        }
        info->L1InstructionCacheSize = 256;
    }
    else if (SysBase->AttnFlags & AFF_68010)
    {
        info->CPUModel = CPUMODEL_68010;
        __sprintf(info->ModelStringBuffer, "%s", "MC68010");
    }

    if (info->FPUModel != FPUMODEL_INTERNAL) {
        UBYTE *s = info->ModelStringBuffer;
        while (*s)
            s++;
        if (SysBase->AttnFlags & AFF_68882)
        {
            info->FPUModel = FPUMODEL_68882;
            __sprintf(s, "%s", "/MC68882");
        }
        else if (SysBase->AttnFlags & AFF_68881)
        {
            info->FPUModel = FPUMODEL_68881;
            __sprintf(s, "%s", "/MC68881");
        }
        else
            info->FPUModel = FPUMODEL_NONE;
    }
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
