/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.

    Desc: GetCPUInfo() - Provides information about installed CPUs
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/libcall.h>
#include <resources/processor.h>
#include <aros/kernel.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

static void ProcessFeaturesTag(struct ARMProcessorInformation * info, struct TagItem * tag);

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

    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    /* If no processor is specified, query the BP */
    selectedprocessor = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);

    /* If selectedprocessor not in line with number of processors, report on
    first available processor */
    if (selectedprocessor >= ProcessorBase->cpucount)
        selectedprocessor = 0;

    /* Go over each passed tag and fill appropriate data */
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
            ARM_AnswerTag(ProcessorBase, selectedprocessor, passedTag);
            break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

VOID ARM_AnswerTag(struct ProcessorBase * ProcessorBase, ULONG coreNo, struct TagItem * tag)
{
    struct ARMProcessorInformation **sysprocs = ProcessorBase->Private1;
    struct ARMProcessorInformation *processor = sysprocs[coreNo];
    const struct ProcessorTopology *topo = ProcessorBase->Topology;
    const struct ProcessorTopologyEntry *entry = NULL;

    if (topo && coreNo < topo->pt_Count)
        entry = &topo->pt_Entries[coreNo];

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
        *((CONST_STRPTR *)tag->ti_Data) = processor->FamilyString
            ? processor->FamilyString : (CONST_STRPTR)"Unknown";
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
    case(GCIT_CacheLineSize):
        *((ULONG *)tag->ti_Data) = processor->CacheLineSize;
        break;
    case(GCIT_Architecture):
        *((ULONG *)tag->ti_Data) = PROCESSORARCH_ARM;
        break;
    case(GCIT_Endianness):
        if (processor->Features1 & FEATF_BIGEND)
            *((ULONG *)tag->ti_Data) = ENDIANNESS_BE;
        else
            *((ULONG *)tag->ti_Data) = ENDIANNESS_LE;
        break;
    case(GCIT_ProcessorSpeed):
        *((UQUAD *)tag->ti_Data) = GetCurrentProcessorFrequency(ProcessorBase, processor);
        break;
    case(GCIT_ProcessorLoad):
#if defined(__AROSEXEC_SMP__)
        *((ULONG *)tag->ti_Data) = KrnGetSystemAttr(KATTR_CPULoad + coreNo);
#else
        *((ULONG *)tag->ti_Data) = 0; /* TODO: IMPLEMENT */
#endif
        break;
    case GCIT_Vendor:
        *((CONST_STRPTR *)tag->ti_Data) = processor->Vendor;
        break;
    case GCIT_PackageID:
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_PackageID : 0;
        break;
    case GCIT_ClusterID:
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_ClusterID : 0;
        break;
    case GCIT_CoreID:
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_CoreID : coreNo;
        break;
    case GCIT_ThreadID:
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_ThreadID : 0;
        break;
    case GCIT_PhysicalID:
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_PhysicalID : coreNo;
        break;
    }
}

static void ProcessFeaturesTag(struct ARMProcessorInformation * info, struct TagItem * tag)
{
D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    switch(tag->ti_Tag)
    {
    case(GCIT_SupportsVFP):
    case(GCIT_SupportsFPU):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU) >> FEATB_FPU); break;
    case(GCIT_SupportsVFPv3):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU_VFP3) >> FEATB_FPU_VFP3); break;
    case(GCIT_SupportsVFPv3D16):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU_VFP3_16) >> FEATB_FPU_VFP3_16); break;
    case(GCIT_SupportsNeon):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_NEON) >> FEATB_NEON); break;
    case(GCIT_SupportsVFPv4):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU_VFP4) >> FEATB_FPU_VFP4); break;
    case(GCIT_SupportsSecurityExt):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_SECURE) >> FEATB_SECURE); break;
    case(GCIT_SupportsBranchPred):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_BRANCHP) >> FEATB_BRANCHP); break;
    case(GCIT_SupportsThumbEE):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_THUMBEX) >> FEATB_THUMBEX); break;
    default:
        *((BOOL *)tag->ti_Data) = FALSE; break;
    }
}
