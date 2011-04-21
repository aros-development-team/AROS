/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Legacy IRQ class implementation on top of kernel.resource
    Lang: english
*/

#include <aros/debug.h>
#include <hidd/irq.h>
#include <proto/kernel.h>

#include "irq_intern.h"

#define KernelBase	isd->kernelBase

/*** HIDDIRQ::AddHandler() ***************************************/

BOOL Irq__Hidd_IRQ__AddHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_AddHandler *msg)
{
    UBYTE irqnum;
    struct irq_staticdata *isd = ISD(cl);

    D(bug("HIDDIRQ::AddHandler(): handler %s, irq %d\n", msg->handlerinfo->h_Node.ln_Name, msg->id));

    /*
     * Look up machine-specific numbers from a static table (if present).
     * TODO: kernel.resource can provide us with timer IRQ number sometimes.
     *       May be we should use it ?
     * Or may be we should query kernel.resource about this table ?
     * Or may be just declare irq.hidd obsolete ?
     */
    if (msg->id < 0)
    {
    	/* Negate the index. Now it starts from 1. */
    	irqnum = -msg->id;

	/* Outside of table? Error if so. */
    	if (irqnum > IRQ_Table[0])
    	{
    	    D(bug("[HIDDIRQ] IRQ ID %d is outside of table range\n", msg->id));
	    return FALSE;
	}

    	irqnum = IRQ_Table[irqnum];
    	if (irqnum == -1)
    	{
    	    D(bug("[HIDDIRQ] Table has no mapping for IRQ ID %d \n", msg->id));
    	    return FALSE;
    	}
    }
    else
    	irqnum = msg->id;

    D(bug("Translated IRQ number is %d\n", irqnum));

    /*
     * The interrupts are added through the kernel.resource now. I will store the Handle in 
     * Node structure of HIDDT_IRQ_Handler, which is not used anymore :)
     */
    msg->handlerinfo->h_Node.ln_Succ = KrnAddIRQHandler(irqnum, msg->handlerinfo->h_Code, msg->handlerinfo, &isd->hwinfo);

    return msg->handlerinfo->h_Node.ln_Succ ? TRUE : FALSE;
}

/*** HIDDIRQ::RemHandler() ***************************************/

VOID Irq__Hidd_IRQ__RemHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    struct irq_staticdata *isd = ISD(cl);

    D(bug("HIDDIRQ::RemHandler(): handler %s\n", msg->handlerinfo->h_Node.ln_Name));

    /* If ln_Succ is not empty then it surely points to the Handle returned from KrnAddIRQHandler().
     * Use it to remove the handler by kernel.resource now */
    if (msg->handlerinfo && msg->handlerinfo->h_Node.ln_Succ)
    {
        KrnRemIRQHandler(msg->handlerinfo->h_Node.ln_Succ);
	msg->handlerinfo->h_Node.ln_Succ = NULL;
    }

    ReturnVoid("HIDDIRQ::RemHandler");
}

/*** HIDDIRQ::CauseIRQ() *****************************************/

VOID Irq__Hidd_IRQ__CauseIRQ(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_CauseIRQ *msg)
{
    EnterFunc(bug("HIDDIRQ::CauseIRQ()\n"));

    /* TODO: Write CauseIRQ method (???) */

    ReturnVoid("HIDDIRQ::CauseIRQ");
}
