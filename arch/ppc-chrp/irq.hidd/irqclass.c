/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id: irqclass.c 28593 2008-05-01 20:19:36Z schulz $

    Desc: IRQ class
    Lang: english
*/

#include <hidd/irq.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/oop.h>
#include <proto/kernel.h>

#include <exec/alerts.h>
#include <exec/memory.h>
#include <aros/system.h>

#include "irq.h"

#define DEBUG 0
#include <aros/debug.h>

/* Don't initialize them with "= 0", otherwise they end up in the DATA segment! */

static OOP_AttrBase HiddIRQAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_IRQ,     &HiddIRQAttrBase        },
    { NULL, NULL }
};

/*** HIDDIRQ::AddHandler() ***************************************/

BOOL Irq__Hidd_IRQ__AddHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_AddHandler *msg)
{
    LONG irqnum = msg->id;
    void *KernelBase = OpenResource("kernel.resource");
    
    EnterFunc(bug("HIDDIRQ::AddHandler()\n"));
    D(bug("Adding handler %s, irq %d\n",msg->handlerinfo->h_Node.ln_Name, msg->id));

    if (msg->id < 0)
    {
        ReturnInt("HIDDIRQ::AddHandler", ULONG, FALSE);
    }
    else 
        irqnum = msg->id;

    D(bug("Translated IRQ number is %d\n", irqnum));

    if (irqnum >= 0 && irqnum <= 63)
    {
        static HIDDT_IRQ_HwInfo dummy;
        dummy.sysBase = SysBase;
        dummy.Error = 0;
        
        /*
         * The interrupts are added through the kernel.resource now. I will store the Handle in 
         * Node structure of HIDDT_IRQ_Handler, which is not used anymore :)
         */
        msg->handlerinfo->h_Node.ln_Succ = KrnAddIRQHandler(irqnum, msg->handlerinfo->h_Code, msg->handlerinfo, &dummy);

        ReturnInt("HIDDIRQ::AddHandler", ULONG, TRUE);
    }

    ReturnInt("HIDDIRQ::AddHandler", ULONG, FALSE);
}

/*** HIDDIRQ::RemHandler() ***************************************/

VOID Irq__Hidd_IRQ__RemHandler(OOP_Class *cl, OOP_Object *obj, struct pHidd_IRQ_RemHandler *msg)
{
    EnterFunc(bug("HIDDIRQ::RemHandler()\n"));
    D(bug("Removing handler %s\n",msg->handlerinfo->h_Node.ln_Name));

    void *KernelBase = OpenResource("kernel.resource");
    
    /* If ln_Succ is not empty then it surely points to the Handle returned from KrnAddIRQHandler().
     * Use it to remove the handler by kernel.resource now */
    
    if (msg->handlerinfo && msg->handlerinfo->h_Node.ln_Succ)
        KrnRemIRQHandler(msg->handlerinfo->h_Node.ln_Succ);
    
    msg->handlerinfo->h_Node.ln_Succ = NULL;
    
    ReturnVoid("HIDDIRQ::RemHandler");
}

/*** HIDDIRQ::CauseIRQ() *****************************************/

VOID Irq__Hidd_IRQ__CauseIRQ(OOP_Class *cl, OOP_Object *obj, struct pHidd_CauseIRQ *msg)
{
    EnterFunc(bug("HIDDIRQ::CauseIRQ()\n"));
#warning TODO: Write CauseIRQ method
    ReturnVoid("HIDDIRQ::CauseIRQ");
}
