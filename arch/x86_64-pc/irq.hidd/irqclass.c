/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: IRQ class
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <hidd/irq.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/kernel.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <aros/system.h>

#include "irq.h"

#define HiddIRQAttrBase isd->irqAttrBase
#define KernelBase	isd->kernelBase

/*** HIDDIRQ::AddHandler() ***************************************/

BOOL Irq__Hidd_IRQ__AddHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_AddHandler *msg)
{
    ULONG irqnum;
    struct irq_staticdata *isd = ISD(cl);

    EnterFunc(bug("HIDDIRQ::AddHandler()\n"));
    D(bug("Adding handler %s, irq %d\n",msg->handlerinfo->h_Node.ln_Name, msg->id));

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

    default:
    	irqnum = msg->id;
    	break;
    }

    D(bug("Translated IRQ number is %d\n", irqnum));

    if (irqnum <= 0xDF)
    {
        /*
         * The interrupts are added through the kernel.resource now. I will store the Handle in 
         * Node structure of HIDDT_IRQ_Handler, which is not used anymore :)
         */
        msg->handlerinfo->h_Node.ln_Succ = KrnAddIRQHandler(irqnum, msg->handlerinfo->h_Code, msg->handlerinfo, &isd->hwinfo);

	if (msg->handlerinfo->h_Node.ln_Succ)
	    ReturnInt("HIDDIRQ::AddHandler", ULONG, TRUE);
    }

    ReturnInt("HIDDIRQ::AddHandler", ULONG, FALSE);
}

/*** HIDDIRQ::RemHandler() ***************************************/

VOID Irq__Hidd_IRQ__RemHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    struct irq_staticdata *isd = ISD(cl);

    EnterFunc(bug("HIDDIRQ::RemHandler()\n"));

    D(bug("Removing handler %s\n", msg->handlerinfo->h_Node.ln_Name));

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

VOID Irq__Hidd_IRQ__CauseIRQ(OOP_Class *cl, OOP_Object *obj, struct pHidd_CauseIRQ *msg)
{
    EnterFunc(bug("HIDDIRQ::CauseIRQ()\n"));

    /* TODO: Write CauseIRQ method (???) */

    ReturnVoid("HIDDIRQ::CauseIRQ");
}
