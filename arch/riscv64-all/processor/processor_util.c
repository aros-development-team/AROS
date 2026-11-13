/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Answering the per-core queries.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <aros/kernel.h>
#include <proto/kernel.h>
#include <resources/processor.h>

#include "processor_intern.h"
#include "processor_arch_intern.h"

/*
 * Weak default for machines with no way to ask: the IDs stay unknown.
 * The OpenSBI platform code overrides this with the SBI queries.
 */
void __attribute__((weak)) Processor_PlatformReadIDs(UQUAD *vendor,
                                                     UQUAD *archid,
                                                     UQUAD *impl)
{
    *vendor = 0;
    *archid = 0;
    *impl = 0;
}

static BOOL FeatureBit(struct RiscVProcessorInformation *info, ULONG bit)
{
    return (info->Features & RVFEATF(bit)) ? TRUE : FALSE;
}

VOID Processor_AnswerTag(struct ProcessorBase *ProcessorBase, ULONG coreNo,
                         struct TagItem *tag)
{
    struct RiscVProcessorInformation *info =
        Processor_GetInfo(ProcessorBase, coreNo);
    const struct ProcessorTopology *topo = ProcessorBase->Topology;
    const struct ProcessorTopologyEntry *entry = NULL;

    if (topo && coreNo < topo->pt_Count)
        entry = &topo->pt_Entries[coreNo];

    if ((tag->ti_Tag > GCIT_FeaturesBase) && (tag->ti_Tag <= GCIT_FeaturesLast))
    {
        BOOL supported = FALSE;

        if (info) switch (tag->ti_Tag)
        {
        case GCIT_SupportsFPU:
            supported = FeatureBit(info, RVFEATB_F);
            break;
        case GCIT_Supports64BitMode:
            supported = (info->Family == CPUFAMILY_RISCV_RV64);
            break;
        case GCIT_SupportsVirtualization:
            supported = FeatureBit(info, RVFEATB_H);
            break;
        case GCIT_SupportsRVM:
            supported = FeatureBit(info, RVFEATB_M);
            break;
        case GCIT_SupportsRVA:
            supported = FeatureBit(info, RVFEATB_A);
            break;
        case GCIT_SupportsRVF:
            supported = FeatureBit(info, RVFEATB_F);
            break;
        case GCIT_SupportsRVD:
            supported = FeatureBit(info, RVFEATB_D);
            break;
        case GCIT_SupportsRVC:
            supported = FeatureBit(info, RVFEATB_C);
            break;
        case GCIT_SupportsRVV:
            supported = FeatureBit(info, RVFEATB_V);
            break;
        case GCIT_SupportsZba:
            supported = FeatureBit(info, RVFEATB_ZBA);
            break;
        case GCIT_SupportsZbb:
            supported = FeatureBit(info, RVFEATB_ZBB);
            break;
        case GCIT_SupportsZbc:
            supported = FeatureBit(info, RVFEATB_ZBC);
            break;
        case GCIT_SupportsZbs:
            supported = FeatureBit(info, RVFEATB_ZBS);
            break;
        case GCIT_SupportsZfh:
            supported = FeatureBit(info, RVFEATB_ZFH);
            break;
        case GCIT_SupportsZicbom:
            supported = FeatureBit(info, RVFEATB_ZICBOM);
            break;
        case GCIT_SupportsZicboz:
            supported = FeatureBit(info, RVFEATB_ZICBOZ);
            break;
        case GCIT_SupportsSstc:
            supported = FeatureBit(info, RVFEATB_SSTC);
            break;
        case GCIT_SupportsSvpbmt:
            supported = FeatureBit(info, RVFEATB_SVPBMT);
            break;
        }

        *((BOOL *)tag->ti_Data) = supported;
        return;
    }

    switch (tag->ti_Tag)
    {
    case GCIT_ModelString:
        *((CONST_STRPTR *)tag->ti_Data) = (info && info->ModelString)
            ? info->ModelString : (CONST_STRPTR)"RISC-V";
        break;
    case GCIT_ISAString:
        *((CONST_STRPTR *)tag->ti_Data) = (info && info->ISAString)
            ? info->ISAString : (CONST_STRPTR)"Unknown";
        break;
    case GCIT_Family:
        *((ULONG *)tag->ti_Data) = info ? info->Family : CPUFAMILY_RISCV;
        break;
    case GCIT_Model:
        *((ULONG *)tag->ti_Data) = info ? (ULONG)info->ArchID : 0;
        break;
    case GCIT_Version:
        *((ULONG *)tag->ti_Data) = info ? (ULONG)info->ImplID : 0;
        break;
    case GCIT_Vendor:
        *((ULONG *)tag->ti_Data) = info ? (ULONG)info->VendorID : 0;
        break;
    case GCIT_VectorUnit:
        *((ULONG *)tag->ti_Data) = info ? info->VectorUnit : VECTORTYPE_NONE;
        break;
    case GCIT_L1CacheSize:
        *((ULONG *)tag->ti_Data) = info ?
            info->L1DataCacheSize + info->L1InstructionCacheSize : 0;
        break;
    case GCIT_L1DataCacheSize:
        *((ULONG *)tag->ti_Data) = info ? info->L1DataCacheSize : 0;
        break;
    case GCIT_L1InstructionCacheSize:
        *((ULONG *)tag->ti_Data) = info ? info->L1InstructionCacheSize : 0;
        break;
    case GCIT_L2CacheSize:
        *((ULONG *)tag->ti_Data) = info ? info->L2CacheSize : 0;
        break;
    case GCIT_L3CacheSize:
        *((ULONG *)tag->ti_Data) = info ? info->L3CacheSize : 0;
        break;
    case GCIT_CacheLineSize:
        *((ULONG *)tag->ti_Data) = info ? info->CacheLineSize : 0;
        break;
    case GCIT_Architecture:
        *((ULONG *)tag->ti_Data) = PROCESSORARCH_RISCV;
        break;
    case GCIT_Endianness:
        *((ULONG *)tag->ti_Data) = ENDIANNESS_LE;
        break;
    case GCIT_ProcessorSpeed:
        *((UQUAD *)tag->ti_Data) = info ? info->ClockFrequency : 0;
        break;
    case GCIT_FrontsideSpeed:
        *((UQUAD *)tag->ti_Data) = 0;
        break;
    case GCIT_ProcessorLoad:
        {
            intptr_t load = KrnGetCPUAttr(KATTR_CPULoad, coreNo);

            *((ULONG *)tag->ti_Data) = (load == -1) ? 0 : (ULONG)load;
        }
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
        *((ULONG *)tag->ti_Data) = entry ? entry->pte_PhysicalID
                                         : (info ? info->HartID : coreNo);
        break;
    }
}
