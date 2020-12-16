#ifndef KERNEL_IOAPIC_H
#define KERNEL_IOAPIC_H
/*
    Copyright © 2017-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS IOAPIC definitions.
    Lang: english
*/

#include <asm/cpu.h>

#include "apic.h"


#define IOREGSEL                0
#define IOREGWIN                0x10
#define IOREGEOI                0x40

#define IOAPICREG_ID            0x0
#define IOAPICREG_VER           0x1
#define IOAPICREG_ARB           0x2
#define IOAPICREG_REDTBLBASE    0x10

#define IOAPICVER_MASKVER       0xFF
#define IOAPICVER_CNTSHIFT      16
#define IOAPICVER_MASKCNT       (0xFF << IOAPICVER_CNTSHIFT)

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

#define IOAPICB_ENABLED         1
#define IOAPICF_ENABLED         (1 << IOAPICB_ENABLED)
#define IOAPICB_EOI             2
#define IOAPICF_EOI             (1 << IOAPICB_EOI)

struct IOAPICData
{
    ULONG	                ioapic_count;
    struct IOAPICCfgData        ioapics[0];	/* Per-IOAPIC data					*/
};

extern struct IntrController IOAPICInt_IntrController;

#endif /* !KERNEL_IOAPIC_H */