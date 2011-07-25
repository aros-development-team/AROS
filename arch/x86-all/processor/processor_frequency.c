/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <aros/config.h>
#include <aros/debug.h>
#include <proto/exec.h>
#include <resources/processor.h>

#include "processor_arch_intern.h"

/*
 * This code can't work on hosted because rdmsr instruction requires supervisor privilege.
 * Hosted ports could have host-specific version of this code which would utilize host OS services.
 */
#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)

#define MSR_IA32_MPERF              0xE7
#define MSR_IA32_APERF              0xE8
#define MSR_P4_EBC_FREQUENCY_ID     0x2C
#define MSR_CORE_FSB_FREQ           0xCD
#define MSR_CORE_EBL_CR_POWERON     0x2A
#define MSR_NAHALEM_PLATFORM_INFO   0xCE
#define MSR_K10_PSTATE_P0           0xC0010064
#define MSR_K8_FIDVID_STATUS        0xC0010042

#define FSB_100 100000000ULL
#define FSB_133 133330000ULL
#define FSB_166 166670000ULL
#define FSB_200 200000000ULL
#define FSB_266 266670000ULL
#define FSB_333 333330000ULL
#define FSB_400 400000000ULL

static VOID ReadIntelMaxFrequencyInformation(struct X86ProcessorInformation * info)
{
    APTR ssp;

    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    ssp = SuperState();

    /* Procedure for Pentium 4 family (NetBurst) */
    if (info->Family == CPUFAMILY_INTEL_PENTIUM4)
    {
        ULONG eax, edx;
        UQUAD fsb = 0;
        ULONG mult = 0;

        /* Works only on model >= 2 */
        if (info->Model < 2)
            return;

        /* This procesure calculates the maximum frequency */

        rdmsr(MSR_P4_EBC_FREQUENCY_ID, &eax, &edx);

        /*Intel 64 and IA-32 Architectures
          Software Developer's Manual
          Volume 3B:System Programming Guide, Part 2
          Table B-12 */
        switch((eax >> 16) & 0x07)
        {
            case(0):
                if (info->Model == 2)
                    fsb = FSB_100;
                else
                    fsb = FSB_266;
                break;
            case(1): fsb = FSB_133; break;
            case(2): fsb = FSB_200; break;
            case(3): fsb = FSB_166; break;
            case(4): fsb = FSB_333; break;
            default: fsb = 0; break;
        }
        
        /* Getting multiplier at reset time */
        mult = eax >> 24;
        
        info->MaxCPUFrequency = fsb * mult;
        info->MaxFSBFrequency = fsb * 4;
    }

    /* Procedure for Pentium M (part of Pentium Pro) family */
    if ((info->Family == CPUFAMILY_INTEL_PENTIUM_PRO) &&
        (
        (info->Model == 0x09) | /* Pentium M */ 
        (info->Model == 0x0D)   /* Pentium M */
        )
        )
    {
        ULONG eax, edx;
        ULONG mult = 0;

        /*Intel 64 and IA-32 Architectures
          Software Developer's Manual
          Volume 3B:System Programming Guide, Part 2
          Table B-16 */

        rdmsr(MSR_CORE_EBL_CR_POWERON, &eax, &edx);
        
        mult = (eax >> 22) & 0x1F;

        info->MaxCPUFrequency = FSB_100 * mult;
        info->MaxFSBFrequency = FSB_100 * 4;        
    }
    
    /* Procedure for Core (part of Pentium Pro) family (ATOM, Core, Core Duo) */
    if ((info->Family == CPUFAMILY_INTEL_PENTIUM_PRO) &&
        (
        (info->Model == 0x0E) | /* Core Duo */ 
        (info->Model == 0x0F) | /* Core 2 Duo */
        (info->Model == 0x16) | /* Core Celeron */
        (info->Model == 0x17) | /* Core 2 Extreeme */
        (info->Model == 0x1C)   /* ATOM */
        )
        )
    {
        ULONG eax, edx;
        UQUAD fsb = 0;
        ULONG mult = 0;

        /* This procesure calculates the maximum frequency */

        rdmsr(MSR_CORE_FSB_FREQ, &eax, &edx);

        /*Intel 64 and IA-32 Architectures
          Software Developer's Manual
          Volume 3B:System Programming Guide, Part 2
          Table B-3 */
        switch(eax & 0x07)
        {
            case(5): fsb = FSB_100; break;
            case(1): fsb = FSB_133; break;
            case(3): fsb = FSB_166; break;
            case(2): fsb = FSB_200; break;
            case(0): fsb = FSB_266; break;
            case(4): fsb = FSB_333; break;
            case(6): fsb = FSB_400; break;
            default: fsb = 0; break;
        }
        
        rdmsr(MSR_CORE_EBL_CR_POWERON, &eax, &edx);
        /* Getting multiplier at reset time */
        mult = (eax >> 22) & 0x1F;

        info->MaxCPUFrequency = fsb * mult;
        info->MaxFSBFrequency = fsb * 4;
    }
    
    /* Procedure for Nahalem (part of Pentium Pro) family (i7, i5, i3) */
    if ((info->Family == CPUFAMILY_INTEL_PENTIUM_PRO) &&
        (
        (info->Model == 0x1A) | /* Core i7 */ 
        (info->Model == 0x1E) | /* ? */
        (info->Model == 0x1F) | /* ? */
        (info->Model == 0x2E)   /* ? */
        )
        )
    {
        ULONG eax, edx;
        ULONG mult = 0;

        /* This procesure calculates the maximum frequency */

        rdmsr(MSR_NAHALEM_PLATFORM_INFO, &eax, &edx);

        /*Intel 64 and IA-32 Architectures
          Software Developer's Manual
          Volume 3B:System Programming Guide, Part 2
          Table B-5 */
        mult = (eax >> 8) & 0xFF;
        
        info->MaxCPUFrequency = FSB_133 * mult;
        /* Note: FSB is not a valid concept with iX (Nahalem) processors */
        info->MaxFSBFrequency = 0;
    }

    UserState(ssp);
}

static VOID ReadAMDMaxFrequencyInformation(struct X86ProcessorInformation * info)
{
    APTR ssp;

    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    ssp = SuperState();

    if (info->Family == CPUFAMILY_AMD_K10)
    {
        ULONG eax, edx;
        ULONG cpufid = 0, cpudid = 0;

        /* AMD Family 10h Processor BKDG, page 425 */
        rdmsr(MSR_K10_PSTATE_P0, &eax, &edx);
        
        cpufid = eax & 0x1F;
        cpudid = (eax >> 6) & 0x07;
        
        info->MaxCPUFrequency = FSB_100 * (cpufid + 0x10) / (1 << cpudid);
        /* Note: FSB is not a valid concept with K10 processors */
        info->MaxFSBFrequency = 0;
    }
    
    if ((info->Family == CPUFAMILY_AMD_K9) || (info->Family == CPUFAMILY_AMD_K8))
    {
        ULONG eax, ebx, ecx, edx;
        ULONG cpufid = 0;
        
        /* BIOS and Kernel Developer's Guide for the AMD AthlonTM 64 and
            AMD OpteronTM Processors, page 382 */

        /* Check for power management features */
        if (info->CPUIDHighestExtendedFunction < 0x80000007)
            return;

        cpuid(0x80000007);
        
        /* Check avalability of FID */
        if ((edx & 0x02) == 0)
            return;

        /* It should be safe to read MSR_K8_FIDVID_STATUS */
        rdmsr(MSR_K8_FIDVID_STATUS, &eax, &edx);

        cpufid = (eax >> 16) & 0x3F;

        /* Note: K8 has only even multipliers, but they match K9 ones */
        info->MaxCPUFrequency = FSB_200 * (4 * 2 + cpufid) / 2;

        /* Note: FSB is not a valid concept with K8, K9 processors */
        info->MaxFSBFrequency = 0;        
    }

    UserState(ssp);
}

#endif /* AROS_FLAVOUR_STANDALONE */

VOID ReadMaxFrequencyInformation(struct X86ProcessorInformation * info)
{
    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    info->MaxCPUFrequency = 0;
    info->MaxFSBFrequency = 0;

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)

    switch(info->Vendor)
    {
    case(VENDOR_INTEL): 
        ReadIntelMaxFrequencyInformation(info);
        break;
    case(VENDOR_AMD):
        ReadAMDMaxFrequencyInformation(info);
        break;
    default:
        break;
    };

#endif
}

/* Currently the only method of calculating CPU Frequency is based on
   using performance counter. Other methods might include reading power state
   and checking with ACPI power tables or hardcoded power tables */

UQUAD GetCurrentProcessorFrequency(struct X86ProcessorInformation * info)
{
    UQUAD retFreq = info->MaxCPUFrequency;

    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

#if (AROS_FLAVOUR & AROS_FLAVOUR_STANDALONE)

    /*
	Check if APERF/MPERF is available
	Notes: K10, K9 - rdmsr can be used to read current PState/cpufid/cpudid
    */
    if (info->APERFMPERF)
    {
	APTR ssp;
        ULONG eax, edx;
        UQUAD startaperf, endaperf, diffaperf = 0;
        UQUAD startmperf, endmperf, diffmperf = 0;
        LONG i;

	ssp = SuperState();

        for(i = 0; i < 10; i++)
        {
            rdmsr(MSR_IA32_MPERF, &eax, &edx);
            startmperf = (UQUAD)edx << 32 | eax;
            rdmsr(MSR_IA32_APERF, &eax, &edx);
            startaperf = (UQUAD)edx << 32 | eax;

            rdmsr(MSR_IA32_MPERF, &eax, &edx);
            endmperf = (UQUAD)edx << 32 | eax;
            rdmsr(MSR_IA32_APERF, &eax, &edx);
            endaperf = (UQUAD)edx << 32 | eax;

            if ((startmperf > endmperf) || (startaperf > endaperf))
                continue; /* Overflow error. Skip */

            diffmperf += endmperf - startmperf;
            diffaperf += endaperf - startaperf;
        }

	UserState(ssp);

	D(bug("[processor.x86] %s: max: %x, diffa: %x, diffm %x\n", __PRETTY_FUNCTION__, info->MaxCPUFrequency, diffaperf, diffmperf));

        /* Use ratio between MPERF and APERF */
	if (diffmperf)
	    retFreq = info->MaxCPUFrequency * diffaperf / diffmperf;
	else
	    retFreq = info->MaxCPUFrequency;
    }
    else
    {
	/* use PStates ?*/
    }

#endif

    return retFreq;
}
