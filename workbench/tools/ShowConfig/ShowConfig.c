#include <aros/inquire.h>
#include <aros/kernel.h>
#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/date.h>
#include <resources/hpet.h>
#include <resources/processor.h>

#include <proto/aros.h>
#include <proto/clocksource.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/processor.h>

#include <stdio.h>
#include <string.h>

#include "cpuspecific.h"
#include "storage.h"

#define APPNAME "ShowConfig"
#define VERSION "ShowConfig 0.8"

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

APTR ProcessorBase = NULL;
char execextra[100];

ULONG ExtUDivMod32(ULONG a, ULONG b, ULONG *mod)
{
    *mod = a % b;

    return a/b;
}

void PrintNum(ULONG num)
{
    /* MiB ? */
    if(num > 1023) 
    {
	ULONG  x, xx;
	char* fmt = "MiB";
	
	/* GiB ? */
	if(num > 0xfffff)
	{ 
	    num >>= 10; 
	    fmt = "GiB";
	}
	
	num = ExtUDivMod32(UMult32(num, 100) >> 10, 100, &x);
	
	/* round */
	x = ExtUDivMod32(x, 10, &xx);
	
	if(xx > 4)
	{
	    if(++x > 9)
	    {
		x = 0;
		num++;
	    }
	}

        printf("%d.%d %s", (int)num, (int)x, fmt);
    }
    else 
    {
        printf("%d KiB", (int)num);
    }
}

ULONG ComputeKBytes(APTR a, APTR b)
{
    IPTR result = b - a;

    return (ULONG)(result >> 10);
}

void PrintCPUFeatures(const char * const *features, const BOOL *flags, ULONG count)
{
    ULONG i;
    ULONG column = 19;
    BOOL found = FALSE;

    printf("    Features       ");

    for (i = 0; i < count; i++)
    {
        if (flags[i])
        {
            /* Feature names come from static NUL-terminated string tables. */
            ULONG length = (ULONG)strlen(features[i]); /* Flawfinder: ignore */

            if (found)
            {
                if (column + 1 + length > 78)
                {
                    printf("\n                   ");
                    column = 19;
                }
                else
                {
                    printf(" ");
                    column++;
                }
            }

            printf("%s", features[i]);
            column += length;
            found = TRUE;
        }
    }

    if (!found)
        printf("None");

    printf("\n");
}

static VOID PrintSectionHeader(const char *name, BOOL separator)
{
    if (separator)
        printf("\n---\n\n");

    printf("[%s]\n\n", name);
}

static VOID PrintMemoryInformation()
{
    IPTR total = AvailMem(MEMF_TOTAL);
    IPTR free = AvailMem(MEMF_ANY);
    IPTR largest = AvailMem(MEMF_LARGEST);

    printf("  Total            ");
    PrintNum((ULONG)(total >> 10));
    printf("\n  Free             ");
    PrintNum((ULONG)(free >> 10));
    printf("\n  Largest block    ");
    PrintNum((ULONG)(largest >> 10));
    printf("\n");
}

static VOID PrintSystemInformation()
{
    IPTR release_major = 0;
    IPTR release_minor = 0;
    IPTR release_date = 0;
    IPTR abi = (IPTR)-1;
    STRPTR builddate = NULL;
    STRPTR variant = NULL;
    STRPTR architecture = NULL;
    struct ClockData release_clock;

    ArosInquire(AI_ArosReleaseMajor, (IPTR)&release_major,
                AI_ArosReleaseMinor, (IPTR)&release_minor,
                AI_ArosReleaseDate, (IPTR)&release_date,
                AI_ArosBuildDate, (IPTR)&builddate,
                AI_ArosVariant, (IPTR)&variant,
                AI_ArosArchitecture, (IPTR)&architecture,
                AI_ArosABIMajor, (IPTR)&abi,
                TAG_DONE);

    if (release_date)
    {
        Amiga2Date((ULONG)release_date * 86400UL, &release_clock);
        printf("  Release          AROS %lu.%lu (%04u-%02u-%02u)\n",
               (unsigned long)release_major,
               (unsigned long)release_minor,
               (unsigned int)release_clock.year,
               (unsigned int)release_clock.month,
               (unsigned int)release_clock.mday);
    }
    else
    {
        printf("  Release          AROS %lu.%lu\n",
               (unsigned long)release_major,
               (unsigned long)release_minor);
    }

    if (builddate)
        printf("  Build            %s\n", builddate);

    if (architecture)
        printf("  Architecture     %s\n", architecture);

    if (abi == (IPTR)-1)
        printf("  ABI              v1 (development)\n");
    else
        printf("  ABI              %lu\n", (unsigned long)abi);

    if (variant && *variant)
        printf("  Variant          %s\n", variant);
}

static ULONG GetProcessorsCount()
{
    ULONG count = 0;
    struct TagItem tags [] = 
    {
        {GCIT_NumberOfProcessors, (IPTR)&count},
        {TAG_DONE, TAG_DONE}
    };

    GetCPUInfo(tags);

    return count;
}

struct
{
    ULONG Architecture;
    STRPTR Description;
} ProcessorArchitecture [] =
{
    { PROCESSORARCH_UNKNOWN, "Unknown" },
    { PROCESSORARCH_M68K, "M68K" },
    { PROCESSORARCH_PPC, "PowerPC" },
    { PROCESSORARCH_X86, "X86" },
    { PROCESSORARCH_ARM, "ARM" },
    { PROCESSORARCH_RISCV, "RISC-V" },
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

static VOID PrintTopologyInformation()
{
    const struct ProcessorTopology *topo = GetCPUTopology();

    if (!topo)
        return;

    /* One line only when there is a structure worth describing */
    if (topo->pt_Packages > 1 || topo->pt_Clusters > 1 ||
        topo->pt_ThreadsPerCore > 1 || topo->pt_Cores != topo->pt_Count)
    {
        printf("  Topology         %u package(s), %u cluster(s), %u core(s), %u thread(s) per core\n",
               (unsigned int)topo->pt_Packages,
               (unsigned int)topo->pt_Clusters,
               (unsigned int)topo->pt_Cores,
               (unsigned int)topo->pt_ThreadsPerCore);
    }
}

static VOID PrintProcessorInformation()
{
    ULONG count = GetProcessorsCount();
    ULONG i, j;
    CONST_STRPTR modelstring;
    ULONG architecture, endianness;
    CONST_STRPTR architecturestring = "", endiannessstring = "";
    UQUAD cpuspeed;

    PrintTopologyInformation();

    for (i = 0; i < count; i++)
    {
        struct TagItem tags [] =
        {
            {GCIT_ModelString, (IPTR)&modelstring},
            {GCIT_Architecture, (IPTR)&architecture},
            {GCIT_Endianness, (IPTR)&endianness},
            {GCIT_ProcessorSpeed, (IPTR)&cpuspeed},
            {TAG_DONE, TAG_DONE}
        };

        GetCoreInfo(i, tags);

        j = 0;
        while(ProcessorArchitecture[j].Description != NULL)
        {
            if (ProcessorArchitecture[j].Architecture == architecture)
            {
                architecturestring = ProcessorArchitecture[j].Description;
                break;
            }
            j++;
        }

        j = 0;
        while(CurrentEndianness[j].Description != NULL)
        {
            if (CurrentEndianness[j].Endianness == endianness)
            {
                endiannessstring = CurrentEndianness[j].Description;
                break;
            }
            j++;
        }       

	if (!modelstring)
	    modelstring = "Unknown";

        printf("  Processor %-7u[%s/%s] %s", (unsigned int)(i + 1), architecturestring, endiannessstring, modelstring);
        if (cpuspeed)
            printf(" (%llu MHz)", (unsigned long long)(cpuspeed / 1000000));
        printf("\n");

        PrintCPUSpecificInfo(i, ProcessorBase);
    }
}

int __nocommandline;
char __stdiowin[]="CON://800/400/ShowConfig/AUTO/CLOSE/WAIT";

int main()
{
    struct MemHeader *mh;
    APTR KernelBase;
    APTR CSBase;
    ULONG memoryRegion = 0;
    int offset = 0;

#if (__WORDSIZE==64)
    sprintf(&execextra[1], "64bit/");
    offset = 6;
#endif
    if (OpenResource("execlock.resource"))
    {
        sprintf(&execextra[1 + offset], "SMP Enabled ");
        offset += 12;
    }

    if (offset == 0)
        execextra[0]            = '\0';
    else
    {
        execextra[0]            = '[';
        execextra[offset]       = ']';
    }

    PrintSectionHeader("SYSTEM", FALSE);
    printf("  Version          AROS version %d.%d, Exec version %d.%d %s\n",
           ArosBase->lib_Version, ArosBase->lib_Revision,
           SysBase->LibNode.lib_Version, SysBase->LibNode.lib_Revision,
           execextra);

    PrintSystemInformation();

    PrintSectionHeader("PROCESSOR", TRUE);
    ProcessorBase = OpenResource(PROCESSORNAME);
    if (ProcessorBase)
        PrintProcessorInformation();

    PrintSectionHeader("TIMERS", TRUE);
    KernelBase = OpenResource("kernel.resource");
    if (KernelBase)
    {
        CSBase = (APTR)KrnGetSystemAttr(KATTR_ClockSource);
        if (CSBase != (APTR)-1)
            printf("  Kernel clock     %s\n", ((struct Node *)CSBase)->ln_Name);
    }

    CSBase = OpenResource("hpet.resource");
    if (CSBase)
    {
        const struct Node *owner;
        struct Node unusedtsunit =
        {
            .ln_Name = "Available for use"
        };
        ULONG i = 0;

        while (GetCSUnitAttrs(i, CLOCKSOURCE_UNIT_OWNER, &owner, TAG_DONE))
        {
            if (!owner)
                owner = &unusedtsunit;

            printf("  HPET %02u          %s\n",
                   (unsigned)(++i), owner->ln_Name);
        }
    }

    PrintSectionHeader("MEMORY", TRUE);
    PrintMemoryInformation();

    for (mh = (struct MemHeader *)SysBase->MemList.lh_Head;
         mh->mh_Node.ln_Succ;
         mh = (struct MemHeader *)mh->mh_Node.ln_Succ)
    {
        char *memtype = "ROM";

        if (mh->mh_Attributes & MEMF_CHIP)
            memtype = "CHIP";
        if (mh->mh_Attributes & MEMF_FAST)
            memtype = "FAST";

        printf("\n  Region %-10u%s\n",
               (unsigned int)(++memoryRegion), memtype);
        printf("    Size           ");
        PrintNum(ComputeKBytes(mh->mh_Lower, mh->mh_Upper));
        printf("\n");
        printf("    Address        $%p-$%p\n",
               mh->mh_Lower, mh->mh_Upper - 1);
        printf("    Node type      0x%X\n", mh->mh_Node.ln_Type);
        printf("    Attributes     0x%X\n", mh->mh_Attributes);
    }

    PrintSectionHeader("STORAGE", TRUE);
    PrintStorageInformation();

    PrintSectionHeader("BOOT", TRUE);
    if (KernelBase)
    {
        struct TagItem *bootinfo = KrnGetBootInfo();
        struct TagItem *tag;

        tag = FindTagItem(KRN_BootLoader, bootinfo);
        if (tag)
            printf("  Loader           %s\n", (char *)tag->ti_Data);

        tag = FindTagItem(KRN_CmdLine, bootinfo);
        if (tag)
            printf("  Arguments        %s\n", (char *)tag->ti_Data);
    }

    return 0;
}
