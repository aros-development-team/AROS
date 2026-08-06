/*
    Copyright (C) 2010-2026, The AROS Development Team. All rights reserved.

    Desc: GetCPUInfo() - Provides information about installed CPUs
*/

#include <aros/debug.h>

#include <aros/libcall.h>
#include <resources/processor.h>
#include <aros/kernel.h>
#include <proto/kernel.h>
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
    const struct ProcessorTopology *topo = ProcessorBase->Topology;
    ULONG selectedprocessor = 0;

    D(bug("[processor.x86] :%s()\n", __func__));

    /* If processor was not selected, fall back to legacy mode and report on
    first available processor */
    selectedprocessor = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);

    /* If selectedprocessor not in line with number of processors, report on
    first available processor */
    if (selectedprocessor >= ProcessorBase->cpucount)
        selectedprocessor = 0;

    /* Go over each passed tag and fill apprioprate data */
    while ((passedTag = NextTagItem(&tagList)) != NULL)
    {
        switch (passedTag->ti_Tag)
        {
        case GCIT_NumberOfProcessors:
            *((ULONG *)passedTag->ti_Data) = ProcessorBase->cpucount;
            break;
        case GCIT_NumberOfPackages:
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Packages : 1;
            break;
        case GCIT_NumberOfClusters:
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Clusters : 1;
            break;
        case GCIT_NumberOfCores:
            *((ULONG *)passedTag->ti_Data) =
                topo ? topo->pt_Cores : ProcessorBase->cpucount;
            break;
        case GCIT_ThreadsPerCore:
            *((ULONG *)passedTag->ti_Data) =
                topo ? topo->pt_ThreadsPerCore : 1;
            break;
        case GCIT_SelectedProcessor:
            break;
        default:
            X86_AnswerTag(ProcessorBase, selectedprocessor, passedTag);
            break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

VOID X86_AnswerTag(struct ProcessorBase * ProcessorBase, ULONG coreNo, struct TagItem * tag)
{
    struct X86ProcessorInformation **sysprocs = ProcessorBase->Private1;
    struct X86ProcessorInformation * processor = sysprocs[coreNo];

    if ((tag->ti_Tag > GCIT_FeaturesBase) &&
        (tag->ti_Tag <= GCIT_FeaturesLast))
    {
        ProcessFeaturesTag(processor, tag);
        return;
    }

    switch (tag->ti_Tag)
    {
    case(GCIT_ModelString):
        *((CONST_STRPTR *)tag->ti_Data) = processor->BrandString;
        break;
    case(GCIT_ISAString):
        *((CONST_STRPTR *)tag->ti_Data) = (processor->Features3 & FEATF_AMD64)
            ? (CONST_STRPTR)"x86-64" : (CONST_STRPTR)"x86";
        break;
    case(GCIT_Family):
        *((ULONG *)tag->ti_Data) = processor->Family;
        break;
    case(GCIT_Model):
        *((ULONG *)tag->ti_Data) = processor->Model;
        break;
    case(GCIT_VectorUnit):
        *((ULONG *)tag->ti_Data) = processor->VectorUnit;
        break;
    case(GCIT_L1CacheSize):
        *((ULONG *)tag->ti_Data) =
            (processor->L1DataCacheSize + processor->L1InstructionCacheSize);
        break;
    case(GCIT_L1DataCacheSize):
        *((ULONG *)tag->ti_Data) = processor->L1DataCacheSize;
        break;
    case(GCIT_L1InstructionCacheSize):
        *((ULONG *)tag->ti_Data) = processor->L1InstructionCacheSize;
        break;
    case(GCIT_L2CacheSize):
        *((ULONG *)tag->ti_Data) = processor->L2CacheSize;
        break;
    case(GCIT_L3CacheSize):
        *((ULONG *)tag->ti_Data) = processor->L3CacheSize;
        break;
    case(GCIT_CacheLineSize):
        *((ULONG *)tag->ti_Data) = processor->CacheLineSize;
        break;
    case(GCIT_Architecture):
        *((ULONG *)tag->ti_Data) = PROCESSORARCH_X86;
        break;
    case(GCIT_Endianness):
        *((ULONG *)tag->ti_Data) = ENDIANNESS_LE;
        break;
    case(GCIT_ProcessorSpeed):
        *((UQUAD *)tag->ti_Data) = GetCurrentProcessorFrequency(ProcessorBase, processor);
        break;
    case(GCIT_ProcessorLoad):
        {
            intptr_t load = KrnGetCPUAttr(KATTR_CPULoad, coreNo);

            *((ULONG *)tag->ti_Data) = (load == -1) ? 0 : (ULONG)load;
        }
        break;
    case(GCIT_FrontsideSpeed):
        *((UQUAD *)tag->ti_Data) = processor->MaxFSBFrequency;
        break;
    case GCIT_Vendor:
        *((ULONG *)tag->ti_Data) = processor->Vendor;
        break;
    case GCIT_PackageID:
        *((ULONG *)tag->ti_Data) = processor->PackageID;
        break;
    case GCIT_ClusterID:
        *((ULONG *)tag->ti_Data) = 0;
        break;
    case GCIT_CoreID:
        *((ULONG *)tag->ti_Data) = processor->CoreID;
        break;
    case GCIT_ThreadID:
        *((ULONG *)tag->ti_Data) = processor->ThreadID;
        break;
    case GCIT_PhysicalID:
        *((ULONG *)tag->ti_Data) = processor->APICID;
        break;
    }
}

static void ProcessFeaturesTag(struct X86ProcessorInformation * info, struct TagItem * tag)
{
D(bug("[processor.x86] :%s()\n", __func__));

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
    case(GCIT_SupportsMSR):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_MSR) >> FEATB_MSR); break;
    case(GCIT_SupportsCX16):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_CX16) >> FEATB_CX16); break;
    case(GCIT_SupportsSSE3):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE3) >> FEATB_SSE3); break;
    case(GCIT_SupportsSSSE3):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSSE3) >> FEATB_SSSE3); break;
    case(GCIT_SupportsSSE41):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE41) >> FEATB_SSE41); break;
    case(GCIT_SupportsSSE42):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_SSE42) >> FEATB_SSE42); break;
    case(GCIT_SupportsAES):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_AES) >> FEATB_AES); break;
    case(GCIT_Virtualized):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_HYPERV) >> FEATB_HYPERV); break;
    case(GCIT_SupportsAVX):
        {
            if (info->Features2 & FEATF_XSAVE)
                *((BOOL *)tag->ti_Data) = (BOOL)((info->Features2 & FEATF_AVX) >> FEATB_AVX);
            else
                *((BOOL *)tag->ti_Data) = (BOOL)FALSE;
            break;
        }
    case(GCIT_SupportsMMXEXT):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_MMXEXT) >> FEATB_MMXEXT); break;
    case(GCIT_Supports3DNOW):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_3DNOW) >> FEATB_3DNOW); break;
    case(GCIT_Supports3DNOWEXT):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_3DNOWEXT) >> FEATB_3DNOWEXT); break;
    case(GCIT_SupportsNoExecutionBit):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_XDNX) >> FEATB_XDNX); break;
    case(GCIT_Supports64BitMode):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features3 & FEATF_AMD64) >> FEATB_AMD64); break;
    case(GCIT_SupportsSSE4A):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features4 & FEATF_SSE4A) >> FEATB_SSE4A); break;
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

    default:
        *((BOOL *)tag->ti_Data) = FALSE; break;
    }
}
