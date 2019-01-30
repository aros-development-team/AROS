/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
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

static void ProcessFeaturesTag(struct ARMProcessorInformation * info, struct TagItem * tag);

#include <proto/processor.h>

/* See rom/processor/getcpuinfo.c for documentation */

AROS_LH1(void, GetCPUInfo,
    AROS_LHA(struct TagItem *, tagList, A0),
    struct ProcessorBase *, ProcessorBase, 1, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem * passedTag = NULL;
    struct ARMProcessorInformation *processor = NULL;
    struct ARMProcessorInformation **sysprocs = ProcessorBase->Private1;
    ULONG selectedprocessor = 0;

    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    /* If no processor is specified, query the BP */
    selectedprocessor = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);

    /* If selectedprocessor not in line with number of processors, report on
    first available processor */
    if (selectedprocessor >= ProcessorBase->cpucount)
        selectedprocessor = 0;

    processor = sysprocs[selectedprocessor];

    /* Go over each passed tag and fill appropriate data */
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
        case(GCIT_CacheLineSize):
            *((ULONG *)passedTag->ti_Data) = processor->CacheLineSize;
            break;
        case(GCIT_Architecture):
            *((ULONG *)passedTag->ti_Data) = PROCESSORARCH_ARM;
            break;
        case(GCIT_Endianness):
            if (processor->Features1 & FEATF_BIGEND)
                *((ULONG *)passedTag->ti_Data) = ENDIANNESS_BE;
            else
                *((ULONG *)passedTag->ti_Data) = ENDIANNESS_LE;
            break;
        case(GCIT_ProcessorSpeed):
            *((UQUAD *)passedTag->ti_Data) = GetCurrentProcessorFrequency(processor);
            break;
        case(GCIT_ProcessorLoad):
            *((ULONG *)passedTag->ti_Data) = 0; /* TODO: IMPLEMENT */
            break;
        case GCIT_Vendor:
            *((CONST_STRPTR *)passedTag->ti_Data) = processor->Vendor;
            break;
        }
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

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
