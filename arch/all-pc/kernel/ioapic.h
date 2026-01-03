#ifndef KERNEL_IOAPIC_H
#define KERNEL_IOAPIC_H
/*
    Copyright © 2017-2026, The AROS Development Team. All rights reserved.
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

#define IOAPICDM_

#define IOAPIC_INTPOL_HIGH      0x0
#define IOAPIC_INTPOL_LOW       0x1

#define IOAPIC_DESTMOD_PHYS     0x0
#define IOAPIC_DESTMOD_LOG      0x1

#define IOAPIC_DELMOD_FIXED     0x0      /*                                          */
#define IOAPIC_DELMOD_LOPRI     0x1      /*       lowest priority                    */
#define IOAPIC_DELMOD_SMI       0x2      /*       System Management INT              */
#define IOAPIC_DELMOD_RSVD1     0x3      /*       reserved                           */
#define IOAPIC_DELMOD_NMI       0x4      /*       NMI signal                         */
#define IOAPIC_DELMOD_INIT      0x5      /*       INIT signal                        */
#define IOAPIC_DELMOD_RSVD2     0x6      /*       reserved                           */
#define IOAPIC_DELMOD_EXTINT    0x7      /*       External INTe                      */

struct IOAPICCfgData
{
    APTR        ioapic_Base;
    ULONG       ioapic_Flags;
    apicid_t    ioapic_ID;
    UBYTE       ioapic_Ver;
    UBYTE       ioapic_IRQCount;
    UBYTE       ioapic_SCI;
    ULONG       ioapic_GSI;
    UQUAD       *ioapic_RouteTable;
};

#define IOAPICB_ENABLED         1
#define IOAPICF_ENABLED         (1 << IOAPICB_ENABLED)
#define IOAPICB_EOI             2
#define IOAPICF_EOI             (1 << IOAPICB_EOI)
#define IOAPICB_NORD            30
#define IOAPICF_NORD            (1 << IOAPICB_NORD)
#define IOAPICB_DUMP            31
#define IOAPICF_DUMP            (1 << IOAPICB_DUMP)

struct IOAPICData
{
    ULONG	                ioapic_count;
    struct IOAPICCfgData        ioapics[0];	/* Per-IOAPIC data					*/
};

extern struct IntrController IOAPICInt_IntrController;

#endif /* !KERNEL_IOAPIC_H */