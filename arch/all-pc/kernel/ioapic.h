#ifndef KERNEL_IOAPIC_H
#define KERNEL_IOAPIC_H
/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

#include <asm/cpu.h>

#include "kernel_interrupts.h"

#include "apic.h"

#define IOAPICREG_ID            0
#define IOAPICREG_VER           1
#define IOAPICREG_ARB           2
#define IOAPICREG_REDTBLBASE    10

struct IOAPICCfgData
{
    APTR ioapicBase;
    apicid_t ioapicID;
    UBYTE ioapicVer;
    UBYTE ioapicIRQs;
    UBYTE ioapicGSI;
};

struct IOAPICData
{
    ULONG	                ioapic_count;
    struct IOAPICCfgData        ioapics[0];	/* Per-IOAPIC data					*/
};

extern struct IntrController IOAPICInt_IntrController;

#endif /* !KERNEL_IOAPIC_H */