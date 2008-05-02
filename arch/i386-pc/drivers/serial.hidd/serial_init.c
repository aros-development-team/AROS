 /*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial hidd initialization code.
    Lang: English.
*/
#include <stddef.h>
#include <exec/types.h>
#include <exec/alerts.h>

#include <aros/symbolsets.h>

#include <hidd/irq.h>

#include <proto/oop.h>
#include <proto/exec.h>

#include "serial_intern.h"

#include LC_LIBDEFS_FILE

#undef  SDEBUG
#undef  DEBUG
#define DEBUG 0
#include <aros/debug.h>

void serial_int_13(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);
void serial_int_24(HIDDT_IRQ_Handler *, HIDDT_IRQ_HwInfo *);

static int PCSer_Init(LIBBASETYPEPTR LIBBASE)
{
    struct class_static_data *csd = &LIBBASE->hdg_csd; /* SerialHidd static data */

    EnterFunc(bug("SerialHIDD_Init()\n"));

    csd->irqhidd = OOP_NewObject(NULL, CLID_Hidd_IRQ, NULL);
    if (csd->irqhidd)
    {
	HIDDT_IRQ_Handler *irq;
			
	/* Install COM1 and COM3 interrupt */
	irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
	if(!irq)
	{
	    kprintf("  ERROR: Cannot install Serial\n");
	    Alert( AT_DeadEnd | AN_IntrMem );
	}
	irq->h_Node.ln_Pri=127;		/* Set the highest pri */
	irq->h_Code = serial_int_13;
	irq->h_Data = (APTR)csd;
	HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Serial1);

	/* Install COM2 and COM4 interrupt */
	irq = AllocMem(sizeof(HIDDT_IRQ_Handler), MEMF_CLEAR|MEMF_PUBLIC);
	if(!irq)
	{
	    kprintf("  ERROR: Cannot install Serial\n");
	    Alert( AT_DeadEnd | AN_IntrMem );
	}
	irq->h_Node.ln_Pri=127;		/* Set the highest pri */
	irq->h_Code = serial_int_24;
	irq->h_Data = (APTR)csd;
	HIDD_IRQ_AddHandler(csd->irqhidd, irq, vHidd_IRQ_Serial2);

	D(bug("  Got Interrupts\n"));
	ReturnInt("SerialHIDD_Init", ULONG, TRUE);
    }

    ReturnInt("SerialHIDD_Init", ULONG, FALSE);
}

ADD2INITLIB(PCSer_Init, 0)
ADD2LIBS("irq.hidd", 0, static struct Library *, __irqhidd)
