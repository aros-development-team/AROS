/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <resources/processor.h>
#include <stdio.h>
#if !defined(STATICBUILD)
#include <proto/processor.h>

APTR ProcessorBase = NULL;
#else
#include "processor_intern.h"
struct ProcessorBase ProcBase;
void Processor_GetCPUInfo(struct TagItem * tagList, struct ProcessorBase * ProcessorBase);
LONG Processor_Init(struct ProcessorBase * ProcessorBase);
#define GetCPUInfo(x) Processor_GetCPUInfo(x, &ProcBase)
#endif

static ULONG getcpucount()
{
    ULONG cpucount = 0;
    struct TagItem tags [] = 
    {
        {GCIT_NumberOfProcessors, (IPTR)&cpucount},
        {TAG_DONE, TAG_DONE}
    };

    GetCPUInfo(tags);

    printf("CPU Count: %d\n", (int)cpucount);
    
    return cpucount;
}

struct TagDescription
{
    ULONG Value;
    CONST_STRPTR Description;
};

struct TagDescription VectorUnit [] = 
{
    { VECTORTYPE_NONE, "None"},
    { VECTORTYPE_ALTIVEC, "AltiVec"},
    { VECTORTYPE_VMX, "VMX"},
    { VECTORTYPE_MMX, "MMX"},
    { VECTORTYPE_SSE, "SSE"},
    { VECTORTYPE_SSE2, "SSE2"},
    { VECTORTYPE_SSE3, "SSE3"},
    { VECTORTYPE_SSSE3, "SSSE3"},
    { VECTORTYPE_SSE41, "SSE41"},
    { VECTORTYPE_SSE42, "SSE42"},
    { VECTORTYPE_MMXEXT, "MMX Ext"},
    { VECTORTYPE_3DNOW, "3DNow"},
    { VECTORTYPE_3DNOWEXT, "3DNow Ext"},
    { VECTORTYPE_SSE4A, "SSE4A"},
    { 0, NULL }        
};

struct TagDescription ProcessorFamily [] =
{
    { CPUFAMILY_UNKNOWN, "Unknown" },
    { CPUFAMILY_MOTOROLA_68000, "Motorola MC680x0" },
    { CPUFAMILY_60X, "IBM PowerPC 60x" },
    { CPUFAMILY_7X0, "IBM PowerPC 7x0" },
    { CPUFAMILY_74XX, "IBM PowerPC 74xx" },
    { CPUFAMILY_4XX, "IBM PowerPC 4xx" },
    { CPUFAMILY_AMD_K5, "AMD K5" },
    { CPUFAMILY_AMD_K6, "AMD K6" },
    { CPUFAMILY_AMD_K7, "AMD K7" },
    { CPUFAMILY_AMD_K8, "AMD K8" },
    { CPUFAMILY_AMD_K9, "AMD K9" },
    { CPUFAMILY_AMD_K10, "AMD K10" },
    { CPUFAMILY_INTEL_486, "Intel 486" },
    { CPUFAMILY_INTEL_PENTIUM, "Intel Pentium" },
    { CPUFAMILY_INTEL_PENTIUM_PRO, "Intel Pentium Pro" },
    { CPUFAMILY_INTEL_PENTIUM4, "Intel Pentium 4"},
    { 0, NULL },
};

struct TagDescription ProcessorArchitecture [] =
{
    { PROCESSORARCH_UNKNOWN, "Unknown" },
    { PROCESSORARCH_M68K, "M68K" },
    { PROCESSORARCH_PPC, "PowerPC" },
    { PROCESSORARCH_X86, "X86" },
    { PROCESSORARCH_ARM, "ARM" },
    { 0, NULL }   
};

struct TagDescription CurrentEndianness [] =
{
    { ENDIANNESS_UNKNOWN, "Unknown" },
    { ENDIANNESS_LE, "LE" },
    { ENDIANNESS_BE, "BE" },
    { 0, NULL}
};

struct TagDescription ProcessorFeatures [] =
{
    { GCIT_SupportsFPU, "FPU" },
    { GCIT_SupportsAltiVec, "AltiVec" },
    { GCIT_SupportsVMX, "VMX" },
    { GCIT_SupportsMMX, "MMX" },
    { GCIT_SupportsMMXEXT, "AMD MMX Entensions" },
    { GCIT_Supports3DNOW, "3DNow!" },
    { GCIT_Supports3DNOWEXT, "AMD 3DNow! Extensions" },
    { GCIT_SupportsSSE, "SSE" },
    { GCIT_SupportsSSE2, "SSE2" },
    { GCIT_SupportsSSE3, "SSE3" },
    { GCIT_SupportsSSSE3, "SSSE3" },
    { GCIT_SupportsSSE41, "SSE4.1" },
    { GCIT_SupportsSSE42, "SSE4.2" },
    { GCIT_SupportsSSE4A, "SSE4a" },
    { GCIT_SupportsVME, "Virtual Mode Extension" },
    { GCIT_SupportsPSE, "Page Size Extension" },
    { GCIT_SupportsPAE, "Physical Address Extension" },
    { GCIT_SupportsCX8, "CMPXCHG8 Instruction" },
    { GCIT_SupportsAPIC, "APIC" },
    { GCIT_SupportsCMOV, "Conditional Move Instruction" },
    { GCIT_SupportsPSE36, "36-bit Page Size Extension" },
    { GCIT_SupportsCLFSH, "CLFLUSH Instruction" },
    { GCIT_SupportsACPI, "ACPI" },
    { GCIT_SupportsFXSR, "FXSAVE and FXSTOR Instructions" },
    { GCIT_SupportsHTT, "Hyper-Threading Technology" },
    { GCIT_SupportsCX16, "CMPXCHG16B Instruction" },
    { GCIT_SupportsVirtualization, "Virtualization Technology" },
    { GCIT_SupportsNoExecutionBit, "No-Execution Page Bit" },
    { GCIT_Supports64BitMode, "64-bit Capable (x86-64)" },
    { GCIT_SupportsMSR, "MSR (Model Specific Registers)" },
    { 0, NULL }
};

CONST_STRPTR GetDescription(struct TagDescription * table, ULONG value)
{
    static CONST_STRPTR undefined = "Undefined";
    
    LONG i = 0;
    while (table[i].Description != NULL)
    {
        if (table[i].Value == value)
            return table[i].Description;
        i++;
    }
    
    return undefined;
}

static void printcpuinformation(ULONG index)
{
    CONST_STRPTR modelstring;
    ULONG family, vectorunit;
    ULONG i = 0;
    ULONG l1size, l1datasize, l1instrsize, l2size, l3size, cachelinesize;
    ULONG architecture, endianness;
    UQUAD currentspeed, fsbspeed;
    
    struct TagItem tags [] =
    {
        {GCIT_SelectedProcessor, index},
        {GCIT_ModelString, (IPTR)&modelstring},
        {GCIT_Family, (IPTR)&family},
        {GCIT_VectorUnit, (IPTR)&vectorunit},
        {GCIT_L1CacheSize, (IPTR)&l1size},
        {GCIT_L1DataCacheSize, (IPTR)&l1datasize},
        {GCIT_L1InstructionCacheSize, (IPTR)&l1instrsize},
        {GCIT_L2CacheSize, (IPTR)&l2size},
        {GCIT_L3CacheSize, (IPTR)&l3size},
        {GCIT_CacheLineSize, (IPTR)&cachelinesize},
        {GCIT_Architecture, (IPTR)&architecture},
        {GCIT_Endianness, (IPTR)&endianness},
        {GCIT_ProcessorSpeed, (IPTR)&currentspeed},
        {GCIT_FrontsideSpeed, (IPTR)&fsbspeed},
        {TAG_DONE, TAG_DONE}
    };
    
    GetCPUInfo(tags);
    
    printf("CPU: %d\n", (int)index);
    printf("Family: %d\n", (int)family);
    printf("FamilyString: %s\n", GetDescription(ProcessorFamily, family));
    printf("ModelString: %s\n", modelstring);
    printf("Vector Unit: %s\n", GetDescription(VectorUnit, vectorunit));
    printf("Architecture: %s\n", GetDescription(ProcessorArchitecture, architecture));
    printf("Endianness: %s\n", GetDescription(CurrentEndianness, endianness));
    
    printf("Current Speed: %u Mhz\n", (unsigned)(currentspeed / 1000000));
    printf("Frontside Bus Speed: %u Mhz\n", (unsigned)(fsbspeed / 1000000));
    
    printf("L1CacheSize: %d kB\n", (int)l1size);
    printf("L1DataCacheSize: %d kB\n", (int)l1datasize);
    printf("L1InstructionCacheSize: %d kB\n", (int)l1instrsize);
    printf("L2CacheSize: %d kB\n", (int)l2size);
    printf("L3CacheSize: %d kB\n", (int)l3size);
    printf("CacheLineSize: %d B\n", (int)cachelinesize);
    
    printf("Features:\n");
    i = 0;
    while(ProcessorFeatures[i].Description != NULL)
    {
        BOOL check = FALSE;
        struct TagItem ftags [] =
        {
            { GCIT_SelectedProcessor, index },
            { ProcessorFeatures[i].Value, (IPTR)&check },
            { TAG_DONE, TAG_DONE }
        };
        
        GetCPUInfo(ftags);
        
        if (check)
            printf("    [%s]\n", ProcessorFeatures[i].Description);
        i++;
    }
}

int main(void)
{
    ULONG cpus = 0;
    ULONG index = 0;

#if !defined(STATICBUILD)
    if ((ProcessorBase = OpenResource(PROCESSORNAME)) == NULL)
    {
        printf("Not able to open %s\n", PROCESSORNAME);
        return 1;
    }
#else
    Processor_Init(&ProcBase);
#endif

    cpus = getcpucount();

    for(index = 0; index < cpus; index++)
        printcpuinformation(index);

    return 0;
}
