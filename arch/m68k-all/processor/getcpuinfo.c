/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
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
    struct M68KProcessorInformation * processor = NULL;
    struct SystemProcessors * sysprocs = (struct SystemProcessors *)ProcessorBase->Private1;
    struct Library *UtilityBase = TaggedOpenLibrary(TAGGEDOPEN_UTILITY);

    if (!UtilityBase)
        return;

    /* GCIT_SelectedProcessor is ignored for now. In future it might be used to
       distinguish between M68K processor and PowerPC turbo card processor (?)*/
    processor = &sysprocs->processor;
        
    while ((passedTag = NextTagItem(&tagList)) != NULL)
    {
        if ((passedTag->ti_Tag > GCIT_FeaturesBase) &&
            (passedTag->ti_Tag <= GCIT_FeaturesLast))
        {
            switch(passedTag->ti_Tag)
            {
            case(GCIT_SupportsFPU):
                *((BOOL *)passedTag->ti_Data) = (BOOL)(
                    (processor->FPUModel == FPUMODEL_68881) || 
                    (processor->FPUModel == FPUMODEL_68882) ||
                    (processor->FPUModel == FPUMODEL_INTERNAL)); 
                break;
            default: 
                *((BOOL *)passedTag->ti_Data) = FALSE; break;
            }
        }
        else
        {
        switch(passedTag->ti_Tag)
        {
        case(GCIT_NumberOfProcessors):
            *((ULONG *)passedTag->ti_Data) = 1;
            break;
        case(GCIT_ModelString):
            *((CONST_STRPTR *)passedTag->ti_Data) = processor->ModelString;
            break;
        case(GCIT_Family):
            *((ULONG *)passedTag->ti_Data) = CPUFAMILY_MOTOROLA_68000;
            break;
        case(GCIT_VectorUnit):
            *((ULONG *)passedTag->ti_Data) = VECTORTYPE_NONE;
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
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_L3CacheSize):
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_CacheLineSize):
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_Architecture):
            *((ULONG *)passedTag->ti_Data) = PROCESSORARCH_M68K;
            break;
        case(GCIT_Endianness):
            *((ULONG *)passedTag->ti_Data) = ENDIANNESS_BE;
            break;
        case(GCIT_ProcessorSpeed):
            *((UQUAD *)passedTag->ti_Data) = processor->CPUFrequency;
            break;
        case(GCIT_ProcessorLoad):
            *((UBYTE *)passedTag->ti_Data) = 0; /* TODO: IMPLEMENT */
            break;
        case(GCIT_FrontsideSpeed):
            *((UQUAD *)passedTag->ti_Data) = 0;
            break;
        }
        }
    }

    CloseLibrary(UtilityBase);

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

