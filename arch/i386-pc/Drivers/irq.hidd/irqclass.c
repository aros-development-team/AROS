/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ class
    Lang: english
*/

#include <hidd/irq.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>

#include <exec/alerts.h>
#include <exec/memory.h>
#include <aros/system.h>

#include "irq.h"

#define DEBUG 1
#include <aros/debug.h>

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddIRQAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_IRQ,	&HiddIRQAttrBase	},
    { NULL, NULL }
};

/*** HIDDIRQ::AddHandler() ***************************************/

BOOL PCIrq__Hidd_IRQ__AddHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_AddHandler *msg)
{
    LONG irqnum = msg->id;
    
    EnterFunc(bug("HIDDIRQ::AddHandler()\n"));
    D(bug("Adding handler %s, irq %d\n",msg->handlerinfo->h_Node.ln_Name, msg->id));
    
    if (msg->id < 0)
    {
	switch (msg->id)
	{
	    case vHidd_IRQ_Timer:
		irqnum = 0;
		break;

	    case vHidd_IRQ_Keyboard:
		irqnum = 1;
		break;

	    case vHidd_IRQ_Serial2:
		irqnum = 3;
		break;

	    case vHidd_IRQ_Serial1:
		irqnum = 4;
		break;

	    case vHidd_IRQ_Floppy:
		irqnum = 6;
		break;

	    case vHidd_IRQ_RTC:
		irqnum = 8;
		break;

	    case vHidd_IRQ_Mouse:
		irqnum = 12;
		break;

	    case vHidd_IRQ_HDD1:
		irqnum = 14;
		break;

	    case vHidd_IRQ_HDD2:
		irqnum = 15;
		break;
	}
    }

    D(bug("Translated IRQ number is %d\n", irqnum));

    if (irqnum >= 0 && irqnum < 16)
    {
        /* Adding an IRQ handler has to be atomic */
        Disable();
        Enqueue((struct List *)&ISD(cl)->irqlist[irqnum],(struct Node *)msg->handlerinfo);
        Enable();

        ReturnInt("HIDDIRQ::AddHandler", ULONG, TRUE);
    }
    
    ReturnInt("HIDDIRQ::AddHandler", ULONG, FALSE);
}

/*** HIDDIRQ::RemHandler() ***************************************/

VOID PCIrq__Hidd_IRQ__RemHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    EnterFunc(bug("HIDDIRQ::RemHandler()\n"));
    D(bug("Removing handler %s\n",msg->handlerinfo->h_Node.ln_Name));

    Disable();
    Remove((struct Node *)msg->handlerinfo);
    Enable();

    ReturnVoid("HIDDIRQ::RemHandler");
}

/*** HIDDIRQ::CauseIRQ() *****************************************/

VOID PCIrq__Hidd_IRQ__CauseIRQ(OOP_Class *cl, OOP_Object *obj, struct pHidd_CauseIRQ *msg)
{
    EnterFunc(bug("HIDDIRQ::CauseIRQ()\n"));
#warning TODO: Write CauseIRQ method
    ReturnVoid("HIDDIRQ::CauseIRQ");
}
