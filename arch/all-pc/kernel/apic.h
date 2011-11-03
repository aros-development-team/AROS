/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

#include <asm/cpu.h>
#include <resources/acpi.h>

/*
 * Per-CPU data
 * Even old IntelMP spec say that we should be prepared to handle different CPUs.
 * This is why we have timer frequency here, not globally.
 */
struct CPUData
{
    ULONG timerFreq;	/* Timer clock frequency			*/
    UBYTE lapicID;	/* Local APIC ID				*/
    UBYTE sysID;	/* System (ACPI, whatever) ID, can differ	*/
};

struct APICData
{
    IPTR	   lapicBase; 	/* Local APIC base address			*/
    IPTR	   ioapicBase;	/* IOAPIC base address				*/
    APTR	   acpiBase;	/* acpi.resource base (cached)			*/
    APTR	   acpi_madt;	/* ACPI MADT pointer (cached)			*/
    ULONG	   count;	/* Total number of APICs in the system		*/
    UWORD	   flags;	/* See below					*/
    struct CPUData cores[0];	/* Per-CPU data					*/
};

#define APF_8259 0x0001	/* Legacy PIC present				*/

ULONG core_APIC_Wake(APTR start_addr, UBYTE id, IPTR base);
UBYTE core_APIC_GetID(IPTR base);
void  core_APIC_Init(struct APICData *data, ULONG cpuNum);
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

struct APICData *acpi_APIC_Init(struct ACPIBase *ACPIBase);
struct APICData *core_APIC_Probe(void);
UBYTE core_APIC_GetNumber(struct APICData *data);
