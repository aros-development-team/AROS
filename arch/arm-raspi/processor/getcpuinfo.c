/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
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

/*****************************************************************************

    NAME */
#include <proto/processor.h>

	AROS_LH1(void, GetCPUInfo,

/*  SYNOPSIS */
	AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
	struct ProcessorBase *, ProcessorBase, 1, Processor)

/*  FUNCTION

    INPUTS

    TAGS

    RESULT

    NOTES
    
    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem * passedTag = NULL;
    struct ARMProcessorInformation *processor = NULL;
    struct ARMProcessorInformation **sysprocs = ProcessorBase->Private1;
    ULONG selectedprocessor = 0;

    D(bug("[processor.ARM] :%s()\n", __PRETTY_FUNCTION__));

    /* If processor was not selected, fall back to legacy mode and report on
    first available processor */
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
            *((ULONG *)passedTag->ti_Data) = ENDIANNESS_LE;
            break;
        case(GCIT_ProcessorSpeed):
            *((UQUAD *)passedTag->ti_Data) = GetCurrentProcessorFrequency(processor);
            break;
        case(GCIT_ProcessorLoad):
            *((UBYTE *)passedTag->ti_Data) = 0; /* TODO: IMPLEMENT */
            break;
        case GCIT_Vendor:
            *((ULONG *)passedTag->ti_Data) = processor->Vendor;
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
    case(GCIT_SupportsFPU):
        *((BOOL *)tag->ti_Data) = (BOOL)((info->Features1 & FEATF_FPU) >> FEATB_FPU); break;
    default: 
        *((BOOL *)tag->ti_Data) = FALSE; break;
    }
}
