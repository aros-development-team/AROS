#include <resources/processor.h>
#include <proto/processor.h>

#include <stdio.h>

#include "cpuspecific.h"

#ifdef __x86__

#define FLAGS_NUM 31

static const char *features[] =
{
    "FPU",
    "MMX",
    "MMXExt",
    "3DNow!",
    "3DNowExt!",
    "SSE",
    "SSE2",
    "SSE3",
    "SSSE3",
    "SSE4.1",
    "SSE4.2",
    "SSE4A",
    "AES",
    "AVX",
    "NoExecute",
    "64Bit",
    "Hyperthreading",
    "VME",
    "PSE",
    "PAE",
    "CX8",
    "APIC",
    "CMOV",
    "PSE36",
    "CLFSH",
    "ACPI",
    "FXSR",
    "CX16",
    "Virtualization",
    "MSR",
    "Virtualized"
};

void PrintCPUSpecificInfo(ULONG i, APTR ProcessorBase)
{
    BOOL flags[FLAGS_NUM];
    struct TagItem tags [FLAGS_NUM + 1] =
    {
        {GCIT_SupportsFPU	    , (IPTR)&flags[0 ]},
        {GCIT_SupportsMMX	    , (IPTR)&flags[1 ]},
        {GCIT_SupportsMMXEXT	    , (IPTR)&flags[2 ]},
        {GCIT_Supports3DNOW	    , (IPTR)&flags[3 ]},
        {GCIT_Supports3DNOWEXT	    , (IPTR)&flags[4 ]},
        {GCIT_SupportsSSE  	    , (IPTR)&flags[5 ]},
        {GCIT_SupportsSSE2 	    , (IPTR)&flags[6 ]},
        {GCIT_SupportsSSE3 	    , (IPTR)&flags[7 ]},
        {GCIT_SupportsSSSE3	    , (IPTR)&flags[8 ]},
        {GCIT_SupportsSSE41	    , (IPTR)&flags[9 ]},
        {GCIT_SupportsSSE42	    , (IPTR)&flags[10]},
        {GCIT_SupportsSSE4A	    , (IPTR)&flags[11]},
        {GCIT_SupportsAES	    , (IPTR)&flags[12]},
        {GCIT_SupportsAVX	    , (IPTR)&flags[13]},
        {GCIT_SupportsNoExecutionBit, (IPTR)&flags[14]},
        {GCIT_Supports64BitMode     , (IPTR)&flags[15]},
        {GCIT_SupportsHTT           , (IPTR)&flags[16]},
        {GCIT_SupportsVME           , (IPTR)&flags[17]},
        {GCIT_SupportsPSE           , (IPTR)&flags[18]},
        {GCIT_SupportsPAE           , (IPTR)&flags[19]},
        {GCIT_SupportsCX8           , (IPTR)&flags[20]},
        {GCIT_SupportsAPIC          , (IPTR)&flags[21]},
        {GCIT_SupportsCMOV          , (IPTR)&flags[22]},
        {GCIT_SupportsPSE36         , (IPTR)&flags[23]},
        {GCIT_SupportsCLFSH         , (IPTR)&flags[24]},
        {GCIT_SupportsACPI          , (IPTR)&flags[25]},
        {GCIT_SupportsFXSR          , (IPTR)&flags[26]},
        {GCIT_SupportsCX16          , (IPTR)&flags[27]},
        {GCIT_SupportsVirtualization, (IPTR)&flags[28]},
        {GCIT_SupportsMSR           , (IPTR)&flags[29]},
        {GCIT_Virtualized           , (IPTR)&flags[30]},
        {TAG_DONE                   , 0               }
    };

    if (!GetCoreInfo(i, tags))
        return;

    PrintCPUFeatures(features, flags, FLAGS_NUM);
}

#endif
