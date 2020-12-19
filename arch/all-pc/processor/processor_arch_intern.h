/*
    Copyright © 2010-2020, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

struct X86ProcessorInformation
{
    TEXT    VendorID[13]; /* 12 + \0 */
    TEXT    HyperVID[13]; /* 12 + \0 */
    ULONG   Vendor;
    TEXT    BrandStringBuffer[48];
    STRPTR  BrandString;
    ULONG   Family;
    ULONG   Model;
    ULONG   VectorUnit;
    ULONG   Features1;  /* From EDX, function 00000001 */
    ULONG   Features2;  /* From ECX, function 00000001 */
    ULONG   Features3;  /* From EDX, function 80000001 */
    ULONG   Features4;  /* From ECX, function 80000001 */
    
    /* CPUID Information */
    ULONG   CPUIDHighestStandardFunction;
    ULONG   CPUIDHighestExtendedFunction;

    /* Processor cache */
    ULONG   L1DataCacheSize;
    ULONG   L1InstructionCacheSize;
    ULONG   L2CacheSize;
    ULONG   L3CacheSize;
    ULONG   CacheLineSize;  /* Min. of L1, L2, L3 */
    
    /* MSR Support */
    BOOL    APERFMPERF;
    
    /* Frequency information */
    UQUAD   MaxCPUFrequency;
    UQUAD   MaxFSBFrequency;
};

#define cpuid(num) \
    do { asm volatile("cpuid":"=a"(eax),"=b"(ebx),"=c"(ecx),"=d"(edx):"a"(num)); } while(0)

static inline void __attribute__((always_inline)) rdmsr(ULONG msr_no, ULONG *ret_lo, ULONG *ret_hi)
{
    ULONG ret1,ret2;
    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    *ret_lo=ret1;
    *ret_hi=ret2;
}

static inline ULONG __attribute__((always_inline)) rdmsri(ULONG msr_no)
{
    ULONG ret;

    asm volatile("rdmsr":"=a"(ret):"c"(msr_no));
    return ret;
}

VOID ReadProcessorInformation(struct ProcessorBase *ProcessorBase, struct X86ProcessorInformation * info);
UQUAD GetCurrentProcessorFrequency(struct ProcessorBase *ProcessorBase, struct X86ProcessorInformation * info);
VOID ReadMaxFrequencyInformation(struct X86ProcessorInformation * info);

/* EDX 00000001 Flags */
#define FEATB_FPU       0
#define FEATF_FPU       (1 << FEATB_FPU)
#define FEATB_VME       1
#define FEATF_VME       (1 << FEATB_VME)
#define FEATB_DBG       2
#define FEATB_PSE       3
#define FEATF_PSE       (1 << FEATB_PSE)
#define FEATB_TSC       4                                                               /* Time Stamp Counter */
#define FEATB_MSR       5
#define FEATF_MSR       (1 << FEATB_MSR)
#define FEATB_PAE       6
#define FEATF_PAE       (1 << FEATB_PAE)
#define FEATB_MCE       7
#define FEATB_CX8       8
#define FEATF_CX8       (1 << FEATB_CX8)
#define FEATB_APIC      9                                                               /* Onboard Advanced Programmable Interrupt Controller */
#define FEATF_APIC      (1 << FEATB_APIC)
#define FEATB_RSVD1     10
#define FEATB_SYSE      11
#define FEATB_MTRR      12
#define FEATB_PGE       13
#define FEATB_MCA       14
#define FEATB_CMOV      15
#define FEATF_CMOV      (1 << FEATB_CMOV)
#define FEATB_PAT       16
#define FEATB_PSE36     17
#define FEATF_PSE36     (1 << FEATB_PSE36)
#define FEATB_PSN       18
#define FEATB_CLFSH     19
#define FEATF_CLFSH     (1 << FEATB_CLFSH)
#define FEATB_RSVD2     20
#define FEATB_DS        21
#define FEATB_ACPI      22                                                              /* Onboard thermal control MSRs for ACPI        */
#define FEATF_ACPI      (1 << FEATB_ACPI)
#define FEATB_MMX       23
#define FEATF_MMX       (1 << FEATB_MMX)
#define FEATB_FXSR      24
#define FEATF_FXSR      (1 << FEATB_FXSR)
#define FEATB_SSE       25
#define FEATF_SSE       (1 << FEATB_SSE)
#define FEATB_SSE2      26
#define FEATF_SSE2      (1 << FEATB_SSE2)
#define FEATB_SS        27                                                              /* CPU cache implements self-snoop */
#define FEATB_HTT       28
#define FEATF_HTT       (1 << FEATB_HTT)
#define FEATB_TM        29                                                              /* Thermal monitor automatically limits temperature */
#define FEATB_IA6432    30                                                              /* IA64 processor emulating x86 */


/* ECX 00000001 Flags */
#define FEATB_SSE3      0
#define FEATF_SSE3      (1 << FEATB_SSE3)
#define FEATB_VMX       5
#define FEATF_VMX       (1 << FEATB_VMX)
#define FEATB_SMX       6
#define FEATB_TM2       8
#define FEATB_SSSE3     9
#define FEATF_SSSE3     (1 << FEATB_SSSE3)
#define FEATB_FMA       12
#define FEATB_CX16      13
#define FEATF_CX16      (1 << FEATB_CX16)
#define FEATB_SSE41     19
#define FEATF_SSE41     (1 << FEATB_SSE41)
#define FEATB_SSE42     20
#define FEATF_SSE42     (1 << FEATB_SSE42)
#define FEATB_X2APIC    21
#define FEATB_AES       25
#define FEATF_AES       (1 << FEATB_AES)
#define FEATB_XSAVE     26
#define FEATF_XSAVE     (1 << FEATB_XSAVE)
#define FEATB_OSXSAVE   27
#define FEATF_OSXSAVE   (1 << FEATB_OSXSAVE)
#define FEATB_AVX       28
#define FEATF_AVX       (1 << FEATB_AVX)
#define FEATB_F16C      29
#define FEATB_RDRND     30
#define FEATB_HYPERV    31
#define FEATF_HYPERV    (1 << FEATB_HYPERV)

/* EDX 80000001 AMD Flags */
#define FEATB_XDNX      20
#define FEATF_XDNX      (1 << FEATB_XDNX)
#define FEATB_MMXEXT    22
#define FEATF_MMXEXT    (1 << FEATB_MMXEXT)
#define FEATB_AMD64     29
#define FEATF_AMD64     (1 << FEATB_AMD64)
#define FEATB_3DNOWEXT  30
#define FEATF_3DNOWEXT  (1 << FEATB_3DNOWEXT)
#define FEATB_3DNOW     31
#define FEATF_3DNOW     (1 << FEATB_3DNOW)

/* ECX 80000001 AMD Flags */
#define FEATB_SVM       2
#define FEATF_SVM       (1 << FEATB_SVM)
#define FEATB_SSE4A     6
#define FEATF_SSE4A     (1 << FEATB_SSE4A)

/* Per manufacturer feature masks */
#define FEATURE_MASK_EDX_UNKNOWN    0
#define FEATURE_MASK_ECX_UNKNOWN    0

#define FEATURE_MASK_EDX_INTEL \
    ( FEATF_FPU | FEATF_VME | FEATF_PSE | FEATF_MSR | FEATF_PAE | FEATF_CX8 | FEATF_APIC | \
      FEATF_CMOV | FEATF_PSE36 | FEATF_CLFSH | FEATF_ACPI | FEATF_MMX | \
      FEATF_FXSR | FEATF_SSE | FEATF_SSE2 | FEATF_HTT )
#define FEATURE_MASK_ECX_INTEL \
    ( FEATF_SSE3 | FEATF_VMX | FEATF_SSSE3 | FEATF_CX16 | FEATF_SSE41 | \
      FEATF_SSE42 | FEATF_AES | FEATF_XSAVE | FEATF_OSXSAVE | FEATF_AVX | FEATF_HYPERV )
#define FEATURE_MASK_EDX_EXT_INTEL \
    ( FEATF_XDNX | FEATF_AMD64 )
#define FEATURE_MASK_ECX_EXT_INTEL  0

#define FEATURE_MASK_EDX_AMD \
    ( FEATF_FPU | FEATF_VME | FEATF_PSE | FEATF_MSR | FEATF_PAE | FEATF_CX8 | FEATF_APIC | \
      FEATF_CMOV | FEATF_PSE36 | FEATF_CLFSH | FEATF_MMX | FEATF_FXSR | \
      FEATF_SSE | FEATF_SSE2 | FEATF_HTT )
#define FEATURE_MASK_ECX_AMD \
    ( FEATF_SSE3 | FEATF_SSSE3 | FEATF_CX16 | FEATF_SSE41 | FEATF_AES | FEATF_HYPERV)
#define FEATURE_MASK_EDX_EXT_AMD \
    ( FEATF_XDNX | FEATF_MMXEXT | FEATF_AMD64 | FEATF_3DNOWEXT | FEATF_3DNOW )
#define FEATURE_MASK_ECX_EXT_AMD \
    ( FEATF_SVM | FEATF_SSE4A )

#endif /* PROCESSOR_ARCH_INTERN_H */
