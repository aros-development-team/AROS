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
    APTR        ioapic_Base;
    ULONG       ioapic_Flags;
    apicid_t    ioapic_ID;
    UBYTE       ioapic_Ver;
    UBYTE       ioapic_IRQCount;
    UBYTE       ioapic_GSI;
    UQUAD       *ioapic_RouteTable;
};

#define IOAPICF_ENABLED         (1 << 1)

struct IOAPICData
{
    ULONG	                ioapic_count;
    struct IOAPICCfgData        ioapics[0];	/* Per-IOAPIC data					*/
};

extern struct IntrController IOAPICInt_IntrController;

#endif /* !KERNEL_IOAPIC_H */