/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

#include <resources/acpi.h>

struct APICData
{
    IPTR   lapicBase; 	/* Base address					*/
    ULONG  count;	/* Total number of APICs in the system		*/
    UWORD  flags;	/* See below					*/
    UWORD  IDMap[0];	/* ACPI_ID << 8 | LOGICAL_ID	      		*/
};

#define APF_8259 0x0001	/* Legacy PIC present				*/

ULONG core_APIC_Wake(APTR start_addr, UBYTE id, IPTR base);
IPTR  core_APIC_GetBase(void);
UBYTE core_APIC_GetID(IPTR base);
BOOL  core_APIC_Init(IPTR base);
void  core_APIC_AckIntr(void);

struct APICData *acpi_APIC_Init(struct ACPIBase *ACPIBase);
struct APICData *core_APIC_Probe(void);
UBYTE core_APIC_GetNumber(struct APICData *data);
