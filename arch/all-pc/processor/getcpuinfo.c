/*
    Copyright © 2010-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCPUInfo() - Provides information about installed CPUs
    Lang: english
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <resources/processor.h>
#include <proto/utility.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

static void ProcessFeaturesTag(struct X86ProcessorInformation * info, struct TagItem * tag);

#include <proto/processor.h>

/* See rom/processor/getcpuinfo.c for documentation */

AROS_LH1(void, GetCPUInfo,
    AROS_LHA(struct TagItem *, tagList, A0),
    struct ProcessorBase *, ProcessorBase, 1, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem * passedTag = NULL;
    struct X86ProcessorInformation * processor = NULL;
    struct X86ProcessorInformation **sysprocs = ProcessorBase->Private1;
    ULONG selectedprocessor = 0;

    D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    /* If processor was not selected, fall back to legacy mode and report on
    first available processor */
    selectedprocessor = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);

    /* If selectedprocessor not in line with number of processors, report on 
    first available processor */
    if (selectedprocessor >= ProcessorBase->cpucount)
        selectedprocessor = 0;

    processor = sysprocs[selectedprocessor];

    /* Go over each passed tag and fill apprioprate data */
    while ((passedTag = NextTagItem(&tagList)) != NULL)
    {
        if ((passedTag->ti_Tag > GCIT_FeaturesBase) &&
            (passedTag->ti_Tag <= GCIT_FeaturesLast))
        {
            ProcessFeaturesTag(processor, passedTag);
        }
        else
        {
        switch(passedTag->ti_Tag)
        {
        case(GCIT_NumberOfProcessors):
            *((ULONG *)passedTag->ti_Data) = ProcessorBase->cpucount;
            break;
        case(GCIT_ModelString):
            *((CONST_STRPTR *)passedTag->ti_Data) = processor->BrandString;
            break;
        case(GCIT_Family):
            *((ULONG *)passedTag->ti_Data) = processor->Family;
            break;
        case(GCIT_VectorUnit):
            *((ULONG *)passedTag->ti_Data) = processor->VectorUnit;
            break;
        case(GCIT_L1CacheSize):
            *((ULONG *)passedTag->ti_Data) = 
                (processor->L1DataCacheSize + processor->L1InstructionCacheSize);
            break;
        case(GCIT_L1DataCacheSize):
            *((ULONG *)passedTag->ti_Data) = processor->L1DataCacheSize;
            break;
        case(GCIT_L1InstructionCacheSize):
            *((ULONG *)passedTag->ti_Data) = processor->L1InstructionCacheSize;
            break;
        case(GCIT_L2CacheSize):
            *((ULONG *)passedTag->ti_Data) = processor->L2CacheSize;
            break;
        case(GCIT_L3CacheSize):
            *((ULONG *)passedTag->ti_Data) = processor->L3CacheSize;
            break;
        case(GCIT_CacheLineSize):
            *((ULONG *)passedTag->ti_Data) = processor->CacheLineSize;
            break;
        case(GCIT_Architecture):
            *((ULONG *)passedTag->ti_Data) = PROCESSORARCH_X86;
            break;
        case(GCIT_Endianness):
            *((ULONG *)passedTag->ti_Data) = ENDIANNESS_LE;
            break;
        case(GCIT_ProcessorSpeed):
            *((UQUAD *)passedTag->ti_Data) = GetCurrentProcessorFrequency(processor);
            break;
        case(GCIT_ProcessorLoad):
            *((UBYTE *)passedTag->ti_Data) = 0; /* TODO: IMPLEMENT */
            break;
        case(GCIT_FrontsideSpeed):
            *((UQUAD *)passedTag->ti_Data) = processor->MaxFSBFrequency;
            break;

        case GCIT_Vendor:
            *((ULONG *)passedTag->ti_Data) = processor->Vendor;
            break;
        }
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

static void ProcessFeaturesTag(struct X86ProcessorInformation * info, struct TagItem * tag)
{
D(bug("[processor.x86] :%s()\n", __PRETTY_FUNCTION__));

    switch(tag->ti_Tag)
    {
    case(GCIT_SupportsFPU):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU) >> FEATB_FPU); break;
    case(GCIT_SupportsMMX):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_MMX) >> FEATB_MMX); break;
    case(GCIT_SupportsSSE):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_SSE) >> FEATB_SSE); break;
    case(GCIT_SupportsSSE2):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_SSE2) >> FEATB_SSE2); break;
    case(GCIT_SupportsSSE3):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE3) >> FEATB_SSE3); break;
    case(GCIT_SupportsSSSE3):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSSE3) >> FEATB_SSSE3); break;
    case(GCIT_SupportsSSE41):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE41) >> FEATB_SSE41); break;
    case(GCIT_SupportsSSE42):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE42) >> FEATB_SSE42); break;
    case(GCIT_SupportsMMXEXT):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_MMXEXT) >> FEATB_MMXEXT); break;
    case(GCIT_Supports3DNOW):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_3DNOW) >> FEATB_3DNOW); break;
    case(GCIT_Supports3DNOWEXT):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_3DNOWEXT) >> FEATB_3DNOWEXT); break;
    case(GCIT_SupportsSSE4A):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features4 & FEATF_SSE4A) >> FEATB_SSE4A); break;
    case(GCIT_SupportsVME):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_VME) >> FEATB_VME); break;
    case(GCIT_SupportsPSE):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_PSE) >> FEATB_PSE); break;
    case(GCIT_SupportsPAE):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_PAE) >> FEATB_PAE); break;
    case(GCIT_SupportsCX8):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_CX8) >> FEATB_CX8); break;
    case(GCIT_SupportsAPIC):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_APIC) >> FEATB_APIC); break;
    case(GCIT_SupportsCMOV):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_CMOV) >> FEATB_CMOV); break;
    case(GCIT_SupportsPSE36):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_PSE36) >> FEATB_PSE36); break;
    case(GCIT_SupportsCLFSH):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_CLFSH) >> FEATB_CLFSH); break;
    case(GCIT_SupportsACPI):
        if (info->Vendor == VENDOR_INTEL)
            *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_ACPI) >> FEATB_ACPI);
        else 
            *((BOOL *)tag->ti_Data) = FALSE; /* TODO: IMPLEMENT FOR AMD */
        break;
    case(GCIT_SupportsFXSR):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FXSR) >> FEATB_FXSR); break;
    case(GCIT_SupportsHTT):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_HTT) >> FEATB_HTT); break;
    case(GCIT_SupportsCX16):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_CX16) >> FEATB_CX16); break;
    case(GCIT_SupportsVirtualization):
        switch(info->Vendor)
        {
        case(VENDOR_INTEL):
            *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_VMX) >> FEATB_VMX); break;
        case(VENDOR_AMD):
            *((BOOL *)tag->ti_Data) = (BOOL)((info->Features4 & FEATF_SVM) >> FEATB_SVM); break;
        default:
            *((BOOL *)tag->ti_Data) = FALSE; break;
        };
        break;
    case(GCIT_SupportsNoExecutionBit):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_XDNX) >> FEATB_XDNX); break;
    case(GCIT_Supports64BitMode):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_AMD64) >> FEATB_AMD64); break;
    case(GCIT_SupportsMSR):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_MSR) >> FEATB_MSR); break;
    default: 
        *((BOOL *)tag->ti_Data) = FALSE; break;
    }
}
