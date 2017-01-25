#ifndef KERNEL_APIC_H
#define KERNEL_APIC_H
/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

#include <asm/cpu.h>

#include "kernel_interrupts.h"

typedef  UBYTE apicid_t;

/*
 * Per-CPU data
 * Even old IntelMP spec say that we should be prepared to handle different CPUs.
 * This is why we have timer frequency here, not globally.
 */
struct CPUData
{
    ULONG       cpu_TimerFreq;	/* Timer clock frequency			        */
    apicid_t    cpu_LocalID;	/* Local APIC ID				        */
    apicid_t    cpu_PrivateID;  /* Sub-system private (ACPI, whatever) ID -  can differ */
    icintrid_t  cpu_ICID;       /* NB - this is icintrid_t not icid_t                   */
};

struct APICData
{
    IPTR	   lapicBase; 	/* Local APIC base address			        */
    ULONG	   apic_count;	/* Total number of APICs in the system		        */
    UWORD	   flags;	/* See below					        */
    struct CPUData cores[0];	/* Per-CPU data					        */
};

#define APF_8259 0x0001	        /* Legacy PIC present				        */

ULONG core_APIC_Wake(APTR start_addr, apicid_t id, IPTR base);
UBYTE core_APIC_GetID(IPTR base);
void  core_APIC_Init(struct APICData *data, apicid_t cpuNum);
void  core_APIC_AckIntr(void);

#ifdef __x86_64__
#define APIC_BASE_MASK 0x000FFFFFFFFFF000
#else
#define APIC_BASE_MASK 0xFFFFF000
#endif

/* This is callable in supervisor only */
static inline IPTR core_APIC_GetBase(void)
{
    return rdmsri(0x1B) & APIC_BASE_MASK;
}

struct APICData *core_APIC_Probe(void);
apicid_t core_APIC_GetNumber(struct APICData *);

extern struct IntrController APICInt_IntrController;

#endif /* KERNEL_APIC_H */
