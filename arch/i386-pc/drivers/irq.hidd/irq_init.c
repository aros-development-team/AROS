/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ system for standalone i386 AROS
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <oop/oop.h>
#include <utility/utility.h>
#include <aros/symbolsets.h>
#include <exec/interrupts.h>
#include <exec/execbase.h>
#include <hardware/intbits.h>

#include "irq.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

static int PCIrq_Init(LIBBASETYPEPTR LIBBASE)
{
    int i;
    struct irq_staticdata *isd = &LIBBASE->isd;
    
    D(bug("IRQ: Initializing\n"));

    /* Initialize IRQ lists */
    for (i = 0; i < 16; i++)
    {
	NEWLIST(&isd->irqlist[i]);
    }
		
    Disable();
    init_Servers(isd);	/* Initialize all known IRQ servers */
    Enable();		/* Turn interrupts on */

    D(bug("     Init OK\n"));
    return TRUE;
}

ADD2INITLIB(PCIrq_Init, 0)
