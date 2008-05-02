/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/system.h>
#include <aros/symbolsets.h>

#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "parallel_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

#if 0		    
	/* Code that was commented out in the init function before converting to
	 * use of genmodule
	 */
		    csd->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
		    
		    if (csd->irqhidd)
		    {
			HIDDT_IRQ_Handler *irq;
			
			/* Install COM1 and COM3 interrupt */
			irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
			if(!irq)
			{
			    kprintf("  ERROR: Cannot install Parallel\n");
			    Alert( AT_DeadEnd | AN_IntrMem );
			}
			irq->h_Node.ln_Pri=127;		/* Set the highest pri */
			irq->h_Code = parallel_int_7;
			irq->h_Data = (APTR)csd;
			HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Parallel1);

			D(bug("  Got Interrupts\n"));
#endif

