#include <proto/exec.h>
#include <resources/processor.h>
#include <stdio.h>
#include <proto/processor.h>

APTR ProcessorBase = NULL;

static ULONG getcpucount()
{
    ULONG cpucount = 0;
    struct TagItem tags [] = 
    {
        {GCIT_NumberOfProcessors, (IPTR)&cpucount},
        {TAG_DONE, TAG_DONE}
    };

    GetCPUInfo(tags);

    printf("CPU Count: %d\n", cpucount);
    
    return cpucount;
}

struct
{
    ULONG VectorUnitType;
    STRPTR Description;
} VectorUnit [] = 
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

struct
{
    ULONG Feature;
    STRPTR Description;
} ProcessorFeatures [] =
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
    { 0, NULL }
};

struct
{
    ULONG Architecture;
    STRPTR Description;
} ProcessorArchitecture [] =
{
    { PROCESSORARCH_UNKNOWN, "Unknown" },
    { PROCESSORARCH_M68K, "Motorola 68K" },
    { PROCESSORARCH_PPC, "PowerPC" },
    { PROCESSORARCH_X86, "X86" },
    { PROCESSORARCH_ARM, "ARM" },
    { 0, NULL }   
};

struct
{
    ULONG Endianness;
    STRPTR Description;
} CurrentEndianness [] =
{
    { ENDIANNESS_UNKNOWN, "Unknown" },
    { ENDIANNESS_LE, "LE" },
    { ENDIANNESS_BE, "BE" },
    { 0, NULL}
};

static void printcpuinformation(ULONG index)
{
    TEXT modelstring[128] = {0};
    TEXT familystring[64] = {0};
    ULONG family, vectorunit;
    ULONG i = 0;
    ULONG l1size, l1datasize, l1instrsize, l2size, l3size, cachelinesize;
    ULONG architecture, endianness;
    
    struct TagItem tags [] =
    {
        {GCIT_SelectedProcessor, index},
        {GCIT_ModelString, (IPTR)modelstring},
        {GCIT_Family, (IPTR)&family},
        {GCIT_FamilyString, (IPTR)familystring},
        {GCIT_VectorUnit, (IPTR)&vectorunit},
        {GCIT_L1CacheSize, (IPTR)&l1size},
        {GCIT_L1DataCacheSize, (IPTR)&l1datasize},
        {GCIT_L1InstructionCacheSize, (IPTR)&l1instrsize},
        {GCIT_L2CacheSize, (IPTR)&l2size},
        {GCIT_L3CacheSize, (IPTR)&l3size},
        {GCIT_CacheLineSize, (IPTR)&cachelinesize},
        {GCIT_Architecture, (IPTR)&architecture},
        {GCIT_Endianness, (IPTR)&endianness},
        {TAG_DONE, TAG_DONE}
    };
    
    GetCPUInfo(tags);
    
    printf("CPU: %d\n", index);
    printf("Family: %d\n", family);
    printf("FamilyString: %s\n", familystring);
    printf("ModelString: %s\n", modelstring);
    
    while(VectorUnit[i].Description != NULL)
    {
        if (VectorUnit[i].VectorUnitType == vectorunit)
        {
            printf("Vector Unit: %s\n", VectorUnit[i].Description);
            break;
        }
        i++;
    }
    
    i = 0;
    while(ProcessorArchitecture[i].Description != NULL)
    {
        if (ProcessorArchitecture[i].Architecture == architecture)
        {
            printf("Architecture: %s\n", ProcessorArchitecture[i].Description);
            break;
        }
        i++;
    }

    i = 0;
    while(CurrentEndianness[i].Description != NULL)
    {
        if (CurrentEndianness[i].Endianness == endianness)
        {
            printf("Endianness: %s\n", CurrentEndianness[i].Description);
            break;
        }
        i++;
    }
    
    printf("L1CacheSize: %d kB\n", l1size);
    printf("L1DataCacheSize: %d kB\n", l1datasize);
    printf("L1InstructionCacheSize: %d kB\n", l1instrsize);
    printf("L2CacheSize: %d kB\n", l2size);
    printf("L3CacheSize: %d kB\n", l3size);
    printf("CacheLineSize: %d B\n", cachelinesize);
    
    printf("Features:\n");
    i = 0;
    while(ProcessorFeatures[i].Description != NULL)
    {
        BOOL check = FALSE;
        struct TagItem ftags [] =
        {
            { GCIT_SelectedProcessor, index },
            { ProcessorFeatures[i].Feature, (IPTR)&check },
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

    if ((ProcessorBase = OpenResource(PROCESSORNAME)) == NULL)
    {
        printf("Not able to open %s\n", PROCESSORNAME);
        return 1;
    }
    
    cpus = getcpucount();

    for(index = 0; index < cpus; index++)
        printcpuinformation(index);

    return 0;
}
