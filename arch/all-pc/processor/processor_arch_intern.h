/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef PROCESSOR_ARCH_INTERN_H
#define PROCESSOR_ARCH_INTERN_H

#include <exec/types.h>

struct X86ProcessorInformation
{
    TEXT    VendorID[13]; /* 12 + \0 */
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

static inline void __attribute__((always_inline)) rdmsr(LONG msr_no, LONG *ret_lo, LONG *ret_hi)
{
    LONG ret1,ret2;
    asm volatile("rdmsr":"=a"(ret1),"=d"(ret2):"c"(msr_no));
    *ret_lo=ret1;
    *ret_hi=ret2;
}


VOID ReadProcessorInformation(struct X86ProcessorInformation * info);
VOID ReadMaxFrequencyInformation(struct X86ProcessorInformation * info);
UQUAD GetCurrentProcessorFrequency(struct X86ProcessorInformation * info);

/* EDX 00000001 Flags */
#define FEATB_FPU   0
#define FEATB_VME   1
#define FEATB_PSE   3
#define FEATB_MSR   5
#define FEATB_PAE   6
#define FEATB_CX8   8
#define FEATB_APIC  9
#define FEATB_CMOV  15
#define FEATB_PSE36 17
#define FEATB_CLFSH 19
#define FEATB_ACPI  22
#define FEATB_MMX   23
#define FEATB_FXSR  24
#define FEATB_SSE   25
#define FEATB_SSE2  26
#define FEATB_HTT   28

#define FEATF_FPU   (1 << FEATB_FPU)
#define FEATF_VME   (1 << FEATB_VME)
#define FEATF_PSE   (1 << FEATB_PSE)
#define FEATF_MSR   (1 << FEATB_MSR)
#define FEATF_PAE   (1 << FEATB_PAE)
#define FEATF_CX8   (1 << FEATB_CX8)
#define FEATF_APIC  (1 << FEATB_APIC)
#define FEATF_CMOV  (1 << FEATB_CMOV)
#define FEATF_PSE36 (1 << FEATB_PSE36)
#define FEATF_CLFSH (1 << FEATB_CLFSH)
#define FEATF_ACPI  (1 << FEATB_ACPI)
#define FEATF_MMX   (1 << FEATB_MMX)
#define FEATF_FXSR  (1 << FEATB_FXSR)
#define FEATF_SSE   (1 << FEATB_SSE)
#define FEATF_SSE2  (1 << FEATB_SSE2)
#define FEATF_HTT   (1 << FEATB_HTT)

/* ECX 00000001 Flags */
#define FEATB_SSE3  0
#define FEATB_VMX   5
#define FEATB_SSSE3 9
#define FEATB_CX16  13
#define FEATB_SSE41 19
#define FEATB_SSE42 20

#define FEATF_SSE3  (1 << FEATB_SSE3)
#define FEATF_VMX   (1 << FEATB_VMX)
#define FEATF_SSSE3 (1 << FEATB_SSSE3)
#define FEATF_CX16  (1 << FEATB_CX16)
#define FEATF_SSE41 (1 << FEATB_SSE41)
#define FEATF_SSE42 (1 << FEATB_SSE42)

/* EDX 80000001 AMD Flags */
#define FEATB_XDNX      20
#define FEATB_MMXEXT    22
#define FEATB_AMD64     29
#define FEATB_3DNOWEXT  30
#define FEATB_3DNOW     31

#define FEATF_XDNX      (1 << FEATB_XDNX)
#define FEATF_MMXEXT    (1 << FEATB_MMXEXT)
#define FEATF_AMD64     (1 << FEATB_AMD64)
#define FEATF_3DNOWEXT  (1 << FEATB_3DNOWEXT)
#define FEATF_3DNOW     (1 << FEATB_3DNOW)

/* ECX 80000001 AMD Flags */
#define FEATB_SVM       2
#define FEATB_SSE4A     6

#define FEATF_SVM       (1 << FEATB_SVM)
#define FEATF_SSE4A     (1 << FEATB_SSE4A)

/* Per manufacturer feature masks */
#define FEATURE_MASK_EDX_UNKNOWN    0
#define FEATURE_MASK_ECX_UNKNOWN    0

#define FEATURE_MASK_EDX_INTEL \
    (FEATF_FPU | FEATF_VME | FEATF_PSE | FEATF_MSR | FEATF_PAE | FEATF_CX8 | FEATF_APIC | \
    FEATF_CMOV | FEATF_PSE36 | FEATF_CLFSH | FEATF_ACPI | FEATF_MMX | \
    FEATF_FXSR | FEATF_SSE | FEATF_SSE2 | FEATF_HTT)
#define FEATURE_MASK_ECX_INTEL \
    (FEATF_SSE3 | FEATF_VMX | FEATF_SSSE3 | FEATF_CX16 | FEATF_SSE41 | \
    FEATF_SSE42)
#define FEATURE_MASK_EDX_EXT_INTEL \
    (FEATF_XDNX | FEATF_AMD64)
#define FEATURE_MASK_ECX_EXT_INTEL  0

#define FEATURE_MASK_EDX_AMD \
    (FEATF_FPU | FEATF_VME | FEATF_PSE | FEATF_MSR | FEATF_PAE | FEATF_CX8 | FEATF_APIC | \
    FEATF_CMOV | FEATF_PSE36 | FEATF_CLFSH | FEATF_MMX | FEATF_FXSR | \
    FEATF_SSE | FEATF_SSE2 | FEATF_HTT)
#define FEATURE_MASK_ECX_AMD \
    (FEATF_SSE3 | FEATF_SSSE3 | FEATF_CX16 | FEATF_SSE41)
#define FEATURE_MASK_EDX_EXT_AMD \
    (FEATF_XDNX | FEATF_MMXEXT | FEATF_AMD64 | FEATF_3DNOWEXT | FEATF_3DNOW)
#define FEATURE_MASK_ECX_EXT_AMD \
    (FEATF_SVM | FEATF_SSE4A)

#endif /* PROCESSOR_ARCH_INTERN_H */
