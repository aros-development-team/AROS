/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: i386 compatable Processor information.
    Lang: english
*/

#ifndef __AROS_CPU_i386_H__
#define __AROS_CPU_i386_H__

#ifndef __AROS_CPU_H__
#   include <cpu/cpu.h>
#endif

struct i386_compat_intern       /* only applicable to i386 and compatible cpus.. one per cpu */
{
    UBYTE           x86;                                        /*                      */
    UBYTE           x86_vendor;                                 /*                      */
    UBYTE           x86_model;                                  /*                      */
    ULONG           x86_mask;                                   /*                      */
    UWORD           x86_hard_math;                              /*                      */
    ULONG           x86_cpuid;                                  /*                      */
    ULONG           x86_capability;                             /*                      */
    ULONG           x86_reserved;                               /*                      */
    ULONG           x86_vendor_id;                              /*                      */
    ULONG           x86_vendor_id2;
    ULONG           x86_vendor_id3;
};


#define X86_CR4_MCE         0x0040                              /* Machine check enable */
#define X86_CR4_PGE         0x0080                              /* enable global pages */
#define X86_CR4_PCE         0x0100                              /* enable performance counters at ipl 3 */

/* Intel-defined CPU features,                  CPUID level 0x00000001 (edx), word 0 */ 
#define X86_FEATURE_FPU         (0*32+ 0)                       /* Onboard FPU */ 
#define X86_FEATURE_VME         (0*32+ 1)                       /* Virtual Mode Extensions */ 
#define X86_FEATURE_DE          (0*32+ 2)                       /* Debugging Extensions */ 
#define X86_FEATURE_PSE         0x0008                          /* Page size extensions */
#define X86_FEATURE_TSC         0x0010                          /* Time stamp counter */
#define X86_FEATURE_MSR         0x0020                          /* RDMSR/WRMSR */
#define X86_FEATURE_PAE         0x0040                          /* Physical address extension */
#define X86_FEATURE_MCE         0x0080                          /* Machine check exception */
#define X86_FEATURE_CXS         0x0100                          /* cmpxchg8 available */
#define X86_FEATURE_APIC        0x0200                          /* internal APIC */
#define X86_FEATURE_10          0x0400
#define X86_FEATURE_11          0x0800
#define X86_FEATURE_MTRR        0x1000                          /* memory type registers */
#define X86_FEATURE_PGE         0x2000                          /* Global page */
#define X86_FEATURE_MCA         0x4000                          /* Machine Check Architecture */
#define X86_FEATURE_CMOV        0x8000                          /* Cmov/fcomi */

/* Intel defined CPU features,                  CPUID level 0x00000001 (ecx), word 4 */ 
#define X86_FEATURE_EST         (4*32+ 7)                           /* Enhanced SpeedStep */ 

/* AMD defined CPU features,                    CPUID level 0x80000001, word 1 */ 
/* Don't duplicate feature flags which are redundant with Intel! */ 
#define X86_FEATURE_SYSCALL     (1*32+11)                       /* SYSCALL/SYSRET */ 
#define X86_FEATURE_MP          (1*32+19)                       /* MP Capable. */ 
#define X86_FEATURE_MMXEXT      (1*32+22)                       /* AMD MMX extensions */ 
#define X86_FEATURE_LM          (1*32+29)                       /* Long Mode (x86-64) */ 
#define X86_FEATURE_3DNOWEXT    (1*32+30)                       /* AMD 3DNow! extensions */ 

#define X86_FEATURE_K6_MTRR     (3*32+ 1)                       /* AMD K6 nonstandard MTRRs */ 
#define X86_FEATURE_CYRIX_ARR   (3*32+ 2)                       /* Cyrix ARRs (= MTRRs) */ 
#define X86_FEATURE_CENTAUR_MCR (3*32+ 3)                       /* Centaur MCRs (= MTRRs) */ 

/* cpu specific tuning types */ 
#define X86_FEATURE_K8          (3*32+ 4)                       /* Opteron, Athlon64 */ 
#define X86_FEATURE_K7          (3*32+ 5)                       /* Athlon */ 
#define X86_FEATURE_P3          (3*32+ 6)                       /* P3 */ 
#define X86_FEATURE_P4          (3*32+ 7)                       /* P4 */ 

/* VIA/Cyrix/Centaur defined CPU features,      CPUID level 0xC0000001, word 5 */ 
#define X86_FEATURE_XSTORE  (5*32+ 2)                           /* on-CPU RNG present (xstore insn) */ 

/* symbolic names for some interesting MSRs */
/* Intel defined MSRs. */
#define MSR_IA32_P5_MC_ADDR         0
#define MSR_IA32_P5_MC_TYPE		    1
#define MSR_IA32_PLATFORM_ID		0x17
#define MSR_IA32_EBL_CR_POWERON		0x2a

#define MSR_IA32_APICBASE		    0x1b
#define MSR_IA32_APICBASE_BSP		(1<<8)
#define MSR_IA32_APICBASE_ENABLE	(1<<11)
#define MSR_IA32_APICBASE_BASE		(0xfffff<<12)

#define MSR_IA32_UCODE_WRITE		0x79
#define MSR_IA32_UCODE_REV		    0x8b

#define MSR_IA32_PERFCTR0		    0xc1
#define MSR_IA32_PERFCTR1		    0xc2

#define MSR_IA32_BBL_CR_CTL		    0x119

#define MSR_IA32_MCG_CAP		    0x179
#define MSR_IA32_MCG_STATUS		    0x17a
#define MSR_IA32_MCG_CTL		    0x17b

#define MSR_IA32_EVNTSEL0		    0x186
#define MSR_IA32_EVNTSEL1		    0x187

#define MSR_IA32_DEBUGCTLMSR		0x1d9
#define MSR_IA32_LASTBRANCHFROMIP	0x1db
#define MSR_IA32_LASTBRANCHTOIP		0x1dc
#define MSR_IA32_LASTINTFROMIP		0x1dd
#define MSR_IA32_LASTINTTOIP		0x1de

#define MSR_IA32_MC0_CTL		    0x400
#define MSR_IA32_MC0_STATUS		    0x401
#define MSR_IA32_MC0_ADDR		    0x402
#define MSR_IA32_MC0_MISC		    0x403

/* AMD Defined MSRs */
#define MSR_K6_EFER			        0xC0000080
#define MSR_K6_STAR			        0xC0000081
#define MSR_K6_WHCR			        0xC0000082
#define MSR_K6_UWCCR			    0xC0000085
#define MSR_K6_PSOR			        0xC0000087
#define MSR_K6_PFIR			        0xC0000088

#define MSR_K7_EVNTSEL0			    0xC0010000
#define MSR_K7_PERFCTR0			    0xC0010004
//                                  0xC0010010
#define MSR_K7_HWCR			        0xC0010015
//                                  0XC001001B

/* Centaur-Hauls/IDT defined MSRs. */
#define MSR_IDT_FCR1			    0x107
#define MSR_IDT_FCR2			    0x108
#define MSR_IDT_FCR3			    0x109
#define MSR_IDT_FCR4			    0x10a

#define MSR_IDT_MCR0			    0x110
#define MSR_IDT_MCR1			    0x111
#define MSR_IDT_MCR2			    0x112
#define MSR_IDT_MCR3			    0x113
#define MSR_IDT_MCR4			    0x114
#define MSR_IDT_MCR5			    0x115
#define MSR_IDT_MCR6			    0x116
#define MSR_IDT_MCR7			    0x117
#define MSR_IDT_MCR_CTRL		    0x120

/* VIA Cyrix defined MSRs*/
#define MSR_VIA_FCR                 0x1107 
#define MSR_VIA_LONGHAUL            0x110a 
#define MSR_VIA_RNG                 0x110b 
#define MSR_VIA_BCR2                0x1147 

#endif /* __AROS_CPU_i386_H__ */
