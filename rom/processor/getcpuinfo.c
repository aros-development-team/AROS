/*
    Copyright (C) 2010-2011, The AROS Development Team. All rights reserved.

    Desc: GetCPUInfo() - Provides information about installed CPUs
*/

#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <aros/libcall.h>
#include <proto/utility.h>
#include <resources/processor.h>

#include "defaults.h"
#include "processor_intern.h"

/*****************************************************************************

    NAME */
#include <proto/processor.h>

        AROS_LH1(void, GetCPUInfo,

/*  SYNOPSIS */
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct ProcessorBase *, ProcessorBase, 1, Processor)

/*  FUNCTION

        Provides information about selected processor in the system
    
    INPUTS

        Function takes an array of tags. Data is returned for each tag. See
        specific tag description. There is a control tag CGIT_SelectedProcessor.

    TAGS

        GCIT_SelectedProcessor - (ULONG) When this tag is set correctly, information
                                 about choosen processor is provided. If this
                                 tag is missing or this tag has invalid value,
                                 information about first processor is returned.
                                 New code should use GetCoreInfo() instead of
                                 this tag.

        GCIT_NumberOfProcessors - (ULONG *) Provides the number of logical
                                 processors present in the system.

        GCIT_NumberOfPackages - (ULONG *) Provides the number of physical
                                 processor packages present in the system.

        GCIT_NumberOfClusters - (ULONG *) Provides the number of core clusters
                                 present in the system.

        GCIT_NumberOfCores - (ULONG *) Provides the number of physical cores
                                 present in the system.

        GCIT_ThreadsPerCore - (ULONG *) Provides the maximum number of SMT
                                 threads per physical core.

        GCIT_PackageID, GCIT_ClusterID, GCIT_CoreID, GCIT_ThreadID,
        GCIT_PhysicalID - (ULONG *) The position of the selected processor
                                 in the system topology, and its hardware ID.
                                 See GetCoreInfo().

        GCIT_ISAString - (CONST_STRPTR *) Provides a textual description of
                                 the instruction set architecture of the
                                 selected processor. The string is considered
                                 read-only.

        GCIT_ModelString - (CONST_STRPTR *) Provides the name of the model of the
                            processor. The string is considered read-only.

        GCIT_Family - (ULONG *) Provides designation of processor family using
                      one of the CPUFAMILY_XXX values.

        GCIT_VectorUnit - (ULONG *) Provides designation of available vectory
                          unit using one of the VECTORTYPE_XXX values.

        GCIT_Architecture - (ULONG *) Provides designation of processor
                            architecture using one of the PROCESSORARCH_XXX
                            values.

        GCIT_Endianness - (ULONG *) Provides designation of current processor
                          endianness using one of the ENDIANNESS_XXX values.

        GCIT_ProcessorSpeed - (UQUAD *) Provides the current CPU speed in Hz
        
        GCIT_FrontsideSpeed - (UQUAD *) Provides the current FSB speed in Hz

        GCIT_ProcessorLoad - (ULONG *) Provides the current CPU load (0-0xffffffff)

        Cache sizes - (ULONG *) Following tags are used to retrieve size of
                      specified caches.
                      
                      GCIT_L1CacheSize
                      GCIT_L1DataCacheSize
                      GCIT_L1InstructionCacheSize
                      GCIT_L2CacheSize
                      GCIT_L3CacheSize
                      
                      Size is returned in kB.

        GCIT_CacheLineSize - (ULONG *) Provides the size of cache line in bytes.
                             In case these sizes differ per cache level, the
                             smallest size if provided.

        Features - (BOOL *) Following tags are used to check availability of
                   certain features. The result is always a boolean.
                   
                   GCIT_SupportsFPU
                   GCIT_SupportsAltiVec
                   GCIT_SupportsVMX
                   GCIT_SupportsMMX
                   GCIT_SupportsMMXEXT
                   GCIT_Supports3DNOW
                   GCIT_Supports3DNOWEXT
                   GCIT_SupportsSSE
                   GCIT_SupportsSSE2
                   GCIT_SupportsSSE3
                   GCIT_SupportsSSSE3
                   GCIT_SupportsSSE41
                   GCIT_SupportsSSE42
                   GCIT_SupportsSSE4A
                   GCIT_SupportsVME
                   GCIT_SupportsPSE
                   GCIT_SupportsPAE
                   GCIT_SupportsCX8
                   GCIT_SupportsAPIC
                   GCIT_SupportsCMOV
                   GCIT_SupportsPSE36
                   GCIT_SupportsCLFSH
                   GCIT_SupportsACPI
                   GCIT_SupportsFXSR
                   GCIT_SupportsHTT
                   GCIT_SupportsCX16
                   GCIT_SupportsVirtualization
                   GCIT_SupportsNoExecutionBit
                   GCIT_Supports64BitMode
                   GCIT_SupportsMSR

    RESULT

        None

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        GetCoreInfo(), GetCPUTopology()

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct TagItem * passedTag = NULL;
    const struct ProcessorTopology *topo = ProcessorBase->Topology;
    const struct ProcessorTopologyEntry *entry = NULL;
    ULONG selected;

    /* This is the default implementation */

    selected = (ULONG)GetTagData(GCIT_SelectedProcessor, 0, tagList);
    if (selected >= ProcessorBase->cpucount)
        selected = 0;
    if (topo && selected < topo->pt_Count)
        entry = &topo->pt_Entries[selected];

    while ((passedTag = NextTagItem(&tagList)) != NULL)
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
        case(GCIT_NumberOfPackages):
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Packages : 1;
            break;
        case(GCIT_NumberOfClusters):
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Clusters : 1;
            break;
        case(GCIT_NumberOfCores):
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_Cores : ProcessorBase->cpucount;
            break;
        case(GCIT_ThreadsPerCore):
            *((ULONG *)passedTag->ti_Data) = topo ? topo->pt_ThreadsPerCore : 1;
            break;
        case(GCIT_PackageID):
            *((ULONG *)passedTag->ti_Data) = entry ? entry->pte_PackageID : 0;
            break;
        case(GCIT_ClusterID):
            *((ULONG *)passedTag->ti_Data) = entry ? entry->pte_ClusterID : 0;
            break;
        case(GCIT_CoreID):
            *((ULONG *)passedTag->ti_Data) = entry ? entry->pte_CoreID : selected;
            break;
        case(GCIT_ThreadID):
            *((ULONG *)passedTag->ti_Data) = entry ? entry->pte_ThreadID : 0;
            break;
        case(GCIT_PhysicalID):
            *((ULONG *)passedTag->ti_Data) = entry ? entry->pte_PhysicalID : selected;
            break;
        case(GCIT_ModelString):
            *((CONST_STRPTR *)passedTag->ti_Data) = "Unknown";
            break;
        case(GCIT_ISAString):
            *((CONST_STRPTR *)passedTag->ti_Data) = "Unknown";
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
            *((ULONG *)passedTag->ti_Data) = 0;
            break;
        case(GCIT_FrontsideSpeed):
            *((UQUAD *)passedTag->ti_Data) = 0;
            break;
        }
        }
    }

    AROS_LIBFUNC_EXIT
} /* GetCPUInfo() */

