#ifndef RESOURCES_PROCESSOR_H
#define RESOURCES_PROCESSOR_H

/*
    Copyright � 2010-2020, The AROS Development Team. All rights reserved.
        
    Tags and defines for processors information queries
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define PROCESSORNAME "processor.resource"

/*
 * Tags.
 *
 * System-wide tags (GetCPUInfo): GCIT_NumberOfProcessors,
 * GCIT_NumberOfPackages, GCIT_NumberOfClusters, GCIT_NumberOfCores,
 * GCIT_ThreadsPerCore.
 *
 * Per-core tags (GetCoreInfo): everything else - identification,
 * caches, speeds, load, topology position and the boolean feature tags.
 * Cores of a heterogeneous system may answer differently.
 *
 * For compatibility GetCPUInfo() also answers every per-core tag, for
 * the core named by the GCIT_SelectedProcessor control tag (first core
 * when absent); new code should use GetCoreInfo() instead.
 *
 * ARM-specific implementation:
 * 1. GCIT_Model   manufacturer-specific part number
 * 2. GCIT_Version manufacturer-specific revision and variant numbers (see macros below)
 * 3. GCIT_Vendor  implementer ID as defined by ARM Ltd.
 *
 * RISC-V-specific implementation:
 * 1. GCIT_Model   low 32 bits of marchid
 * 2. GCIT_Version low 32 bits of mimpid
 * 3. GCIT_Vendor  mvendorid (JEDEC manufacturer ID)
 */
#define GCIT_NumberOfProcessors     (TAG_USER +   1)
#define GCIT_NumberOfCPUs           GCIT_NumberOfProcessors
#define GCIT_Family                 (TAG_USER +   2)
#define GCIT_Model                  (TAG_USER +   3)
#define GCIT_ModelString            (TAG_USER +   4)
#define GCIT_Version                (TAG_USER +   5)
/* #define GCIT_VersionString      (TAG_USER +   6) */
#define GCIT_FrontsideSpeed         (TAG_USER +   7)
#define GCIT_ProcessorSpeed         (TAG_USER +   8)
#define GCIT_L1CacheSize            (TAG_USER +   9)
#define GCIT_L2CacheSize            (TAG_USER +  10)
#define GCIT_L3CacheSize            (TAG_USER +  11)
#define GCIT_VectorUnit             (TAG_USER +  12)
/* #define GCIT_Extensions         (TAG_USER +  13) */
#define GCIT_CacheLineSize          (TAG_USER +  14)
/* #define GCIT_CPUPageSize        (TAG_USER +  15) */
/* #define GCIT_ExecPageSize       (TAG_USER +  16) */
/* #define GCIT_TimeBaseSpeed      (TAG_USER +  17) */
#define GCIT_SelectedProcessor      (TAG_USER + 100)
#define GCIT_L1DataCacheSize        (TAG_USER + 102)
#define GCIT_L1InstructionCacheSize (TAG_USER + 103)
#define GCIT_Architecture           (TAG_USER + 104)
#define GCIT_Endianness             (TAG_USER + 105)
#define GCIT_ProcessorLoad          (TAG_USER + 106)
#define GCIT_Vendor                 (TAG_USER + 107)
#define GCIT_ISAString              (TAG_USER + 108)

/*
 * Topology. A "processor" as counted by GCIT_NumberOfProcessors is a
 * logical processor (hardware thread). Logical processors group into
 * cores, cores into clusters, clusters into packages. On hardware with
 * no such level the counts report 1 and the IDs 0.
 */
#define GCIT_NumberOfPackages       (TAG_USER + 109)
#define GCIT_NumberOfClusters       (TAG_USER + 110)
#define GCIT_NumberOfCores          (TAG_USER + 111)
#define GCIT_ThreadsPerCore         (TAG_USER + 112)
/* Position of the selected processor within the topology */
#define GCIT_PackageID              (TAG_USER + 113)
#define GCIT_ClusterID              (TAG_USER + 114)
#define GCIT_CoreID                 (TAG_USER + 115)
#define GCIT_ThreadID               (TAG_USER + 116)
/* Hardware ID of the selected processor (APIC ID / MPIDR / hart id) */
#define GCIT_PhysicalID             (TAG_USER + 117)

/* Space [TAG_USER + 200, TAG_USER + 499] is reserved for boolean feature
   tags. Do no introduce non boolean, non feature tags in this space. */
#define GCIT_FeaturesBase           (TAG_USER + 200)
#define GCIT_SupportsFPU            (GCIT_FeaturesBase +   1)
#define GCIT_SupportsAltiVec        (GCIT_FeaturesBase +   2)
#define GCIT_SupportsVMX            (GCIT_FeaturesBase +   3)
#define GCIT_SupportsMMX            (GCIT_FeaturesBase +   4)
#define GCIT_SupportsMMXEXT         (GCIT_FeaturesBase +   5)
#define GCIT_Supports3DNOW          (GCIT_FeaturesBase +   6)
#define GCIT_Supports3DNOWEXT       (GCIT_FeaturesBase +   7)
#define GCIT_SupportsSSE            (GCIT_FeaturesBase +   8)
#define GCIT_SupportsSSE2           (GCIT_FeaturesBase +   9)
#define GCIT_SupportsSSE3           (GCIT_FeaturesBase +  10)
#define GCIT_SupportsSSSE3          (GCIT_FeaturesBase +  11)
#define GCIT_SupportsSSE41          (GCIT_FeaturesBase +  12)
#define GCIT_SupportsSSE42          (GCIT_FeaturesBase +  13)
#define GCIT_SupportsSSE4A          (GCIT_FeaturesBase +  14)
#define GCIT_SupportsVME            (GCIT_FeaturesBase +  15)
#define GCIT_SupportsPSE            (GCIT_FeaturesBase +  16)
#define GCIT_SupportsPAE            (GCIT_FeaturesBase +  17)
#define GCIT_SupportsCX8            (GCIT_FeaturesBase +  18)
#define GCIT_SupportsAPIC           (GCIT_FeaturesBase +  19)
#define GCIT_SupportsCMOV           (GCIT_FeaturesBase +  20)
#define GCIT_SupportsPSE36          (GCIT_FeaturesBase +  21)
#define GCIT_SupportsCLFSH          (GCIT_FeaturesBase +  22)
#define GCIT_SupportsACPI           (GCIT_FeaturesBase +  23)
#define GCIT_SupportsFXSR           (GCIT_FeaturesBase +  24)
#define GCIT_SupportsHTT            (GCIT_FeaturesBase +  25)
#define GCIT_SupportsCX16           (GCIT_FeaturesBase +  26)
#define GCIT_SupportsVirtualization (GCIT_FeaturesBase +  27)
#define GCIT_SupportsNoExecutionBit (GCIT_FeaturesBase +  28)
#define GCIT_Supports64BitMode      (GCIT_FeaturesBase +  29)
#define GCIT_SupportsMSR            (GCIT_FeaturesBase +  30)
#define GCIT_SupportsVFP            (GCIT_FeaturesBase +  31)
#define GCIT_SupportsVFPv3          (GCIT_FeaturesBase +  32)
#define GCIT_SupportsVFPv3D16       (GCIT_FeaturesBase +  33)
#define GCIT_SupportsNeon           (GCIT_FeaturesBase +  34)
#define GCIT_SupportsVFPv4          (GCIT_FeaturesBase +  35)
#define GCIT_SupportsThumb          (GCIT_FeaturesBase +  36)
#define GCIT_SupportsThumbEE        (GCIT_FeaturesBase +  37)
#define GCIT_SupportsBranchPred     (GCIT_FeaturesBase +  38)
#define GCIT_SupportsSecurityExt    (GCIT_FeaturesBase +  39)
#define GCIT_SupportsAMMX           (GCIT_FeaturesBase +  40)
#define GCIT_SupportsAVX            (GCIT_FeaturesBase +  41)
#define GCIT_SupportsAES            (GCIT_FeaturesBase +  42)
#define GCIT_Virtualized            (GCIT_FeaturesBase +  100)
/* RISC-V ISA extensions. The full ISA string is available via
   GCIT_ISAString; these cover the extensions of general interest. */
#define GCIT_SupportsRVM            (GCIT_FeaturesBase + 120)
#define GCIT_SupportsRVA            (GCIT_FeaturesBase + 121)
#define GCIT_SupportsRVF            (GCIT_FeaturesBase + 122)
#define GCIT_SupportsRVD            (GCIT_FeaturesBase + 123)
#define GCIT_SupportsRVC            (GCIT_FeaturesBase + 124)
#define GCIT_SupportsRVV            (GCIT_FeaturesBase + 125)
#define GCIT_SupportsZba            (GCIT_FeaturesBase + 126)
#define GCIT_SupportsZbb            (GCIT_FeaturesBase + 127)
#define GCIT_SupportsZbc            (GCIT_FeaturesBase + 128)
#define GCIT_SupportsZbs            (GCIT_FeaturesBase + 129)
#define GCIT_SupportsZfh            (GCIT_FeaturesBase + 130)
#define GCIT_SupportsZicbom         (GCIT_FeaturesBase + 131)
#define GCIT_SupportsZicboz         (GCIT_FeaturesBase + 132)
#define GCIT_SupportsSstc           (GCIT_FeaturesBase + 133)
#define GCIT_SupportsSvpbmt         (GCIT_FeaturesBase + 134)
#define GCIT_FeaturesLast           (TAG_USER + 499)

/* Processor family defines */
#define CPUFAMILY_UNKNOWN             0
#define CPUFAMILY_60X                 1
#define CPUFAMILY_7X0                 2
#define CPUFAMILY_74XX                3
#define CPUFAMILY_4XX                 4
#define CPUFAMILY_AMD_K5            100
#define CPUFAMILY_AMD_K6            101
#define CPUFAMILY_AMD_K7            102
#define CPUFAMILY_AMD_K8            103
#define CPUFAMILY_AMD_K9            104
#define CPUFAMILY_AMD_K10           105
#define CPUFAMILY_INTEL_486         106
#define CPUFAMILY_INTEL_PENTIUM     107
#define CPUFAMILY_INTEL_PENTIUM_PRO 108
#define CPUFAMILY_INTEL_PENTIUM4    109
#define CPUFAMILY_MOTOROLA_68000    110
#define CPUFAMILY_ARM_3             120    /* ARM family is architecture designation */
#define CPUFAMILY_ARM_4             121
#define CPUFAMILY_ARM_4T            122
#define CPUFAMILY_ARM_5             123
#define CPUFAMILY_ARM_5T            124
#define CPUFAMILY_ARM_5TE           125
#define CPUFAMILY_ARM_5TEJ          126
#define CPUFAMILY_ARM_6             127
#define CPUFAMILY_ARM_7             128
#define CPUFAMILY_RISCV             130    /* Unknown/other RISC-V */
#define CPUFAMILY_RISCV_RV32        131
#define CPUFAMILY_RISCV_RV64        132

/* Vector unit type */
#define VECTORTYPE_NONE               0
#define VECTORTYPE_ALTIVEC            1
#define VECTORTYPE_VMX                2
#define VECTORTYPE_MMX              100
#define VECTORTYPE_MMXEXT           101
#define VECTORTYPE_3DNOW            102
#define VECTORTYPE_3DNOWEXT         103
#define VECTORTYPE_SSE              104
#define VECTORTYPE_SSE2             105
#define VECTORTYPE_SSE3             106
#define VECTORTYPE_SSSE3            107
#define VECTORTYPE_SSE41            108
#define VECTORTYPE_SSE42            109
#define VECTORTYPE_SSE4A            110
#define VECTORTYPE_AVX              111
#define VECTORTYPE_VFP              120
#define VECTORTYPE_VFPv3            121
#define VECTORTYPE_NEON             122
#define VECTORTYPE_RVV              130
#define VECTORTYPE_AMMX             200

/* Processor architecture defines */
#define PROCESSORARCH_UNKNOWN         0
#define PROCESSORARCH_M68K            1
#define PROCESSORARCH_PPC             2
#define PROCESSORARCH_X86             3
#define PROCESSORARCH_ARM             4
#define PROCESSORARCH_RISCV           5

/* Endianness */
#define ENDIANNESS_UNKNOWN            0
#define ENDIANNESS_LE                 1
#define ENDIANNESS_BE                 2

/* Some generic vendor IDs */
#define VENDOR_UNKNOWN                0
#define VENDOR_AMD                    1
#define VENDOR_INTEL                  2
#define VENDOR_CYRIX                  3
#define VENDOR_UMC                    4
#define VENDOR_NEXGEN                 5
#define VENDOR_CENTAUR                6
#define VENDOR_RISE                   7
#define VENDOR_SIS                    8
#define VENDOR_TRANSMETA              9
#define VENDOR_NSC                   10 /* National Semiconductor */
#define VENDOR_MOTOROLA             256

/* ARM implementer IDs */
#define ARM_VENDOR_ARMLTD           0x41 /* ARM Ltd                 */
#define ARM_VENDOR_DEC              0x44 /* Digital Equipment Corp  */
#define ARM_VENDOR_MOTOROLA         0x4D /* Motorola/Freescale      */
#define ARM_VENDOR_QUALCOMM         0x51 /* Qualcomm                */
#define ARM_VENDOR_MARVELL          0x56 /* Marvell Semi Inc        */
#define ARM_VENDOR_INTEL            0x69 /* Intel corp              */

/* For ARM Version tag encodes Revision and Variant numbers */
#define ARM_REVISION(x)     ((x >> 16) & 0xFFFF)
#define ARM_VARIANT(x)      (x & 0xFFFF)

/*
 * RISC-V vendor IDs (JEDEC manufacturer ID as reported in mvendorid).
 * GCIT_Vendor returns the raw mvendorid value on RISC-V.
 */
#define RISCV_VENDOR_ANDES          0x31E
#define RISCV_VENDOR_SIFIVE         0x489
#define RISCV_VENDOR_THEAD          0x5B7

/*
 * System processor topology, as returned by GetCPUTopology(). The table
 * is owned by processor.resource and is read-only; it is valid for the
 * lifetime of the system. Entries are indexed by logical processor and
 * pte_LogicalID may be used with GCIT_SelectedProcessor.
 */
struct ProcessorTopologyEntry
{
    ULONG pte_LogicalID;    /* Index of the logical processor           */
    ULONG pte_PhysicalID;   /* Hardware ID (APIC ID / MPIDR / hart id)  */
    UWORD pte_PackageID;
    UWORD pte_ClusterID;
    UWORD pte_CoreID;       /* Within the package                       */
    UWORD pte_ThreadID;     /* SMT thread within the core               */
    UWORD pte_Pad;
};

struct ProcessorTopology
{
    ULONG pt_Count;         /* Logical processors described             */
    ULONG pt_Packages;
    ULONG pt_Clusters;
    ULONG pt_Cores;         /* Physical cores in the system             */
    ULONG pt_ThreadsPerCore;/* Maximum SMT threads per core             */
    const struct ProcessorTopologyEntry *pt_Entries;
};

#endif /* RESOURCES_PROCESSOR_H */
