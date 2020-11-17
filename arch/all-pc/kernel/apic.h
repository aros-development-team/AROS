#ifndef KERNEL_APIC_H
#define KERNEL_APIC_H
/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

#include <asm/cpu.h>

#define APIC_MSI_BASE   0x7FFFFFFF

/*
 * Per-CPU data
 * Even old IntelMP spec say that we should be prepared to handle different CPUs.
 * This is why we have timer frequency here, not globally.
 */
 
struct CPUMMUConfig
{
    void                        *mmu_PML4;
    void                        *mmu_PTE;
    void                        *mmu_PDP;
    void                        *mmu_PDE;
    int                         mmu_PDEPageCount;
    int                         mmu_PDEPageUsed;
};

struct CPUData
{
    ULONG                       cpu_TimerFreq;	/* Timer clock frequency                                */
    UQUAD                       cpu_TSCFreq;    /* Timestamp counter frequency                          */
    apicid_t                    cpu_LocalID;	/* Local APIC ID                                        */
    apicid_t                    cpu_PrivateID;  /* Sub-system private (ACPI, whatever) ID -  can differ */
    icintrid_t                  cpu_ICID;       /* NB - this is icintrid_t not icid_t                   */
    void                        *cpu_TLS;
    void                        *cpu_GDT;
    void                        *cpu_IDT;
    struct CPUMMUConfig         *cpu_MMU;
    UQUAD                       cpu_LAPICTick;
    UQUAD                       cpu_LastCPULoadTime;
    UQUAD                       cpu_SleepTime;
    ULONG                       cpu_Load;
};

struct APICData
{
    IPTR	                lapicBase; 	/* Local APIC base address			        */
    ULONG	                apic_count;	/* Total number of APICs in the system		        */
    UWORD	                flags;	        /* See below					        */
    struct CPUData          cores[0];	/* Per-CPU data					        */
};

#define APF_8259                (1 << 0)	/* Legacy PIC present				        */
#define APF_TIMER               (1 << 1)	/* APIC uses its own heartbeat timer	                */

#ifdef __x86_64__
#define APIC_BASE_MASK 0x000FFFFFFFFFF000
#else
#define APIC_BASE_MASK 0xFFFFF000
#endif

struct APICCPUWake_Data
{
    APTR        cpuw_apicstartrip;
    IPTR        cpuw_apicbase;
    apicid_t    cpuw_apicid;
};


ULONG core_APIC_Wake(APTR start_addr, apicid_t id, IPTR base);
UBYTE core_APIC_GetID(IPTR base);
void  core_APIC_Init(struct APICData *data, apicid_t cpuNum);
void  core_APIC_AckIntr(void);

/* This is callable in supervisor only */
static inline IPTR core_APIC_GetBase(void)
{
    return rdmsri(0x1B) & APIC_BASE_MASK;
}

struct APICData *core_APIC_Probe(void);

apicid_t core_APIC_GetNumberFromLocal(struct APICData *, apicid_t);
apicid_t core_APIC_GetNumber(struct APICData *);
void core_APIC_GetMask(struct APICData *, apicid_t, cpumask_t *);
BOOL core_APIC_CPUInMask(apicid_t, cpumask_t *);

void core_SetupIDT(apicid_t, apicidt_t *);
BOOL core_SetIDTGate(apicidt_t *, int, uintptr_t, BOOL, BOOL);
BOOL core_SetIRQGate(void *, int, uintptr_t);
void core_DefaultIRETQ();

ULONG core_APIC_AllocMSI(ULONG);
void core_APIC_RegisterMSI(void *);

extern struct IntrController APICInt_IntrController;

#endif /* !KERNEL_APIC_H */
