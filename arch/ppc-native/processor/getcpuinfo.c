/*
    Copyright © 2010-2031, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCPUInfo() - Provides information about installed CPUs
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include "defaults.h"
#include "processor_intern_arch.h"

#include <proto/processor.h>

/* See rom/processor/getcpuinfo.c for documentation */

AROS_LH1(void, GetCPUInfo,
    AROS_LHA(struct TagItem *, tagList, A0),
    struct ProcessorBase *, ProcessorBase, 1, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem * passedTag = NULL;
    struct SystemProcessors *sp = ProcessorBase->Private1;

    while ((passedTag = LibNextTagItem(&tagList)) != NULL)
    {
        if ((passedTag->ti_Tag > GCIT_FeaturesBase) &&
            (passedTag->ti_Tag <= GCIT_FeaturesLast))
        {
            *((BOOL *)passedTag->ti_Data) = FALSE;
            continue;
        }
        else
        {
        switch(passedTag->ti_Tag)
        {
        case(GCIT_NumberOfProcessors):
            *((ULONG *)passedTag->ti_Data) = ProcessorBase->cpucount;
            break;
        case(GCIT_Model):
            *((ULONG *)passedTag->ti_Data) = sp->sp_PVR;
            break;
        case(GCIT_ModelString):
            *((CONST_STRPTR *)passedTag->ti_Data) = "PowerPC";
            break;
        case(GCIT_Family):
            *((ULONG *)passedTag->ti_Data) = CPUFAMILY_UNKNOWN;
            break;
        case(GCIT_VectorUnit):
            *((ULONG *)passedTag->ti_Data) = VECTORTYPE_NONE;
            break;
        case(GCIT_L1CacheSize):
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_L1DataCacheSize):
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_L1InstructionCacheSize):
            *((ULONG *)passedTag->ti_Data) = 0;
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
            *((ULONG *)passedTag->ti_Data) = PROCESSORARCH_DEF;
            break;
        case(GCIT_Endianness):
            *((ULONG *)passedTag->ti_Data) = ENDIANNESS_DEF;
            break;
        case(GCIT_ProcessorSpeed):
            *((UQUAD *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_ProcessorLoad):
            *((UBYTE *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_FrontsideSpeed):
            *((UQUAD *)passedTag->ti_Data) = 0;
            break;
        }
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

