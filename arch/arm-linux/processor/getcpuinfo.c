/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: GetCPUInfo() - Provides information about installed CPUs (ARM version)
    Lang: english
*/


#include <aros/libcall.h>
#include <resources/processor.h>
#include <proto/utility.h>

#include "processor_intern.h"
#include "arch_intern.h"

AROS_LH1(void, GetCPUInfo,
	 AROS_LHA(struct TagItem *, tagList, A0),
	 struct ProcessorBase *, ProcessorBase, 1, Processor)
{
    AROS_LIBFUNC_INIT

    struct TagItem *passedTag = NULL;
    struct LinuxArmProcessor *data = ProcessorBase->Private1;

    /* Go over each passed tag and fill apprioprate data */
    while ((passedTag = NextTagItem((const struct TagItem **)&tagList)) != NULL)
    {
        ULONG val;

        switch (passedTag->ti_Tag)
        {
        case GCIT_NumberOfProcessors:
            *((ULONG *)passedTag->ti_Data) = 1;
            break;

        case GCIT_Family:
            *((ULONG *)passedTag->ti_Data) = data->Arch;
            break;

	case GCIT_Model:
	    *((ULONG *)passedTag->ti_Data) = data->Part;		/* ARM Model is vendor-specific part number */
	    break;

        case GCIT_ModelString:
            *((CONST_STRPTR *)passedTag->ti_Data) = data->Model;	/* String composed by Linux kernel */
            break;

	case GCIT_Version:
	    *((ULONG *)passedTag->ti_Data) = data->Version;
	    break;

        case GCIT_VectorUnit:
            /* These values are mutually exclusive, report the best from feature flags */
            if (data->Features & FF_NEON)
            	val = VECTORTYPE_NEON;
            else if (data->Features & FF_VFPv3)
            	val = VECTORTYPE_VFPv3;
            else if (data->Features & FF_VFP)
            	val = VECTORTYPE_VFP;
            else
            	val = 0;

            *((ULONG *)passedTag->ti_Data) = val;
            break;

        case GCIT_Architecture:
            *((ULONG *)passedTag->ti_Data) = PROCESSORARCH_ARM;
            break;

        case GCIT_Endianness:
            *((ULONG *)passedTag->ti_Data) = ENDIANNESS_LE;	/* We are little-endian only */
            break;

	case GCIT_Vendor:
	    *((ULONG *)passedTag->ti_Data) = data->Implementer;
	    break;

	default:
	    if ((passedTag->ti_Tag >= GCIT_SupportsVFP) && (passedTag->ti_Tag <= GCIT_SupportsThumbEE))
	    {
	    	/* Each feature tag has own bit in Features field */
	    	ULONG val = data->Features >> (passedTag->ti_Tag - GCIT_SupportsVFP);

	    	*((BOOL *)passedTag->ti_Data) = val & 0x01;
	    }
	    break;
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */
